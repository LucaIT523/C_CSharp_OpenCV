#include "jetclass.h"
#include <codecvt>  // for codecvt_utf8

// Dosyalarý al
std::vector<std::wstring> ShapeDetector::getFilesInDirectory(const std::wstring& directory, const std::wstring& ext) 
{
	std::vector<std::wstring> files;
	WIN32_FIND_DATAW fileData;
	HANDLE hFind;
	std::wstring fullSearchPath = directory + L"/*" + ext;

	if ((hFind = FindFirstFileW(fullSearchPath.c_str(), &fileData)) != INVALID_HANDLE_VALUE) {
		do {
			files.push_back(directory + L"/" + fileData.cFileName);
		} while (FindNextFileW(hFind, &fileData));
		FindClose(hFind);
	}
	return files;
}
std::wstring ShapeDetector::extractFileName(const std::wstring& filepath) 
{
	size_t lastSlash = filepath.find_last_of(L"\\/");
	size_t lastDot = filepath.find_last_of(L".");
	if (lastDot == std::wstring::npos) {
		lastDot = filepath.size();
	}
	return filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
}

class ParallelTemplateMatching : public cv::ParallelLoopBody
{
private:
	cv::Mat& cropped;
	std::vector<cv::Mat>& temp;
	std::vector<cv::Mat>& tempFlip;
	std::vector<bool>& tempFlag;
	std::vector<double>& minCorrelation;
	std::vector<int>& bestMatchIndex;
	std::vector<int>& bestMatchAngle;
	std::vector<double>& bestMatchScale;
	std::vector <cv::Rect>& bestMatchLoc;
	int& numTemplateFound;
	int numTemp;
	int angleSearchRange;
	int angleStep;
	double scaleSearchMin;
	double scaleSearchMax;
	double scaleSearchStep;
	int maxNumMatches;
	mutable std::mutex mutex;

public:
	ParallelTemplateMatching(cv::Mat& _cropped, std::vector<cv::Mat>& _temp, std::vector<cv::Mat>& _tempFlip, std::vector<bool>& _tempFlag, std::vector<double>& _minCorrelation, std::vector<int>& _bestMatchIndex, std::vector<int>& _bestMatchAngle, std::vector<double>& _bestMatchScale, std::vector <cv::Rect>& _bestMatchLoc, int& _numTemplateFound, int _numTemp, int _angleSearchRange, int _angleStep, double _scaleSearchMin, double _scaleSearchMax, double _scaleSearchStep, int _maxNumMatches)
		: cropped(_cropped), temp(_temp), tempFlip(_tempFlip), tempFlag(_tempFlag), minCorrelation(_minCorrelation), bestMatchIndex(_bestMatchIndex), bestMatchAngle(_bestMatchAngle), bestMatchScale(_bestMatchScale), bestMatchLoc(_bestMatchLoc), numTemplateFound(_numTemplateFound), numTemp(_numTemp), angleSearchRange(_angleSearchRange), angleStep(_angleStep), scaleSearchMin(_scaleSearchMin), scaleSearchMax(_scaleSearchMax), scaleSearchStep(_scaleSearchStep), maxNumMatches(_maxNumMatches){}

	virtual void operator()(const cv::Range& range) const CV_OVERRIDE
	{
		for (int j = range.start; j < range.end; j++)
		{

			if (tempFlag[j] == false)
				continue;

			double currMinCorr = 1.0;
			double currMinScale = 0;
			int currMinAngle = 0;
			cv::Rect currMinLoc;
			bool matchFound = false;

			//bool currTemplateFound = false;
			for (int k = 0; k < 2; k++)
			{
				
				cv::Mat currTemp;
				if (k == 0)
				{
					currTemp = temp[j];
				}
				else
				{
					currTemp = tempFlip[j];
				}

				for (double currScale = scaleSearchMin; currScale <= scaleSearchMax; currScale += scaleSearchStep)
				{
					cv::Mat currTempScaled;

					// Resmin orijinal boyutlarýný al
					int original_width = currTemp.cols;
					int original_height = currTemp.rows;
					int new_width = original_width * currScale;
					int new_height = original_height * currScale;
					cv::resize(currTemp, currTempScaled, cv::Size(new_width, new_height));

					if (cropped.size().width <= currTempScaled.size().width || cropped.size().height <= currTempScaled.size().height)
					{
						//std::cout << "cropped object size is smaller than the template size for template: Skipping to the next template." << std::endl;
						continue;
					}
					//std::cout << "scale tested" << std::endl;
					for (int angle = -angleSearchRange; angle < angleSearchRange; angle += angleStep)
					{

						// Rotate template
						cv::Mat rotMat = getRotationMatrix2D(cv::Point2f(currTempScaled.cols / 2, currTempScaled.rows / 2), angle, 1);
						cv::Mat rotatedTemp;
						cv::warpAffine(currTempScaled, rotatedTemp, rotMat, currTempScaled.size());

						// Perform template matching
						cv::Mat result;
						cv::matchTemplate(cropped, rotatedTemp, result, cv::TM_SQDIFF_NORMED);

						// Find the best match
						double minVal, maxVal;
						cv::Point minLoc, maxLoc;
						cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

						// Save the best match found so far
						if (minVal<currMinCorr) {
							currMinCorr = minVal;
							currMinScale = currScale;
							currMinAngle = angle;
							currMinLoc= cv::Rect(minLoc.x, minLoc.y, rotatedTemp.cols, rotatedTemp.rows);
							matchFound = true;

						}
					}
				}
			}
			if(matchFound)
			{ 
				std::lock_guard<std::mutex> lock(mutex);  // Mutex ile kilitle
				minCorrelation.push_back(1 - currMinCorr);
				bestMatchIndex.push_back(j);
				bestMatchAngle.push_back(currMinAngle);
				bestMatchScale.push_back(currMinScale);
				bestMatchLoc.push_back(currMinLoc);
				numTemplateFound++;
			}
			
		}
	}
};

int DetectionResult::set_output_empty()
{
	numTemplateFound = 0;
	bestMatchIndex.clear();
	bestMatchName.clear();
	bestMatchAngle.clear();
	bestMatchScale.clear();
	bestMatchLoc.clear();
	bestMatchCorr.clear();

	return 0;
}

int ShapeDetector::read_shape_directory(const std::wstring Directory, const std::wstring Extension)
{
	TemplateFiles = getFilesInDirectory(Directory, Extension);
	NumTemplates = TemplateFiles.size();

	for (const auto& file : TemplateFiles)
	{
		//cv::Mat image = cv::imread(cv::String(file.begin(), file.end()), cv::IMREAD_GRAYSCALE);
		//files are read this way instead of imread to overcome Turkish character errors
		std::vector<unsigned char> fileContent;
		std::ifstream fileStream(file, std::ios::binary);
		fileContent.assign(std::istreambuf_iterator<char>(fileStream), std::istreambuf_iterator<char>());
		cv::Mat image = cv::imdecode(fileContent, cv::IMREAD_GRAYSCALE);

		cv::Mat imageFlip;
		if (image.empty()) {
			std::cout << "Could not open or find the template" << std::endl;
			return -1;
		}
		std::wstring wtext = extractFileName(file);

		// Original image size
		int original_width = image.cols;
		int original_height = image.rows;

		// New dimensions
		int new_width = original_width*tempReadScale;
		int new_height = original_height*tempReadScale;
		cv::resize(image, image, cv::Size(new_width, new_height));
		cv::bitwise_not(image, image);
		cv::threshold(image, image, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

		//find relevant area in the image and process only this area to save time
		cv::Mat labels, stats, centroids;
		int numOfLabels = cv::connectedComponentsWithStats(image, labels, stats, centroids);
		int maxArea = 0;
		cv::Rect maxRect;
		for (int i = 1; i < numOfLabels; i++) {
			int area = stats.at<int>(i, cv::CC_STAT_AREA);
			cv::Rect rect(stats.at<int>(i, cv::CC_STAT_LEFT),
				stats.at<int>(i, cv::CC_STAT_TOP),
				stats.at<int>(i, cv::CC_STAT_WIDTH),
				stats.at<int>(i, cv::CC_STAT_HEIGHT));

			if (area > maxArea) {
				maxArea = area;
				maxRect = rect;
			}
		}

		// draw a bbox around the largest CC
		cv::Mat cropped = image(maxRect);
		int cropPad = 20 * tempReadScale;
		cv::copyMakeBorder(cropped, cropped, cropPad, cropPad, cropPad, cropPad, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

		//save flipped template
		cv::flip(cropped, imageFlip, 1);
		Templates.push_back(cropped);
		TemplatesF.push_back(imageFlip);
		TemplateNames.push_back(wtext);
		TemplateFlags.push_back(true);
	}

	return 0;
}


int ShapeDetector::set_templates_to_detect(const std::vector<std::wstring> Filenames)
{
	// Dosya oluþturup yazma iþlemini yapalým.
	std::wofstream outfile("activeTemplates.txt");  // Çýktý dosyasýný açalým. (UTF-16 formatýnda karakterleri doðru yazdýrmak için wofstream kullanýlmýþtýr.)
	outfile.imbue(std::locale(outfile.getloc(), new std::codecvt_utf8<wchar_t>));  // UTF-8 encoding

	if (!outfile)
	{
		std::wcerr << L"output.txt dosyasý oluþturulamadý ya da açýlamadý." << std::endl;
		return -1;  // Bir hata kodu dön.
	}

	//TODO: set all flags to 0 first, then search for the file names in the main template file vector and set their flags to 1
	if (Filenames.size() == 1 && Filenames[0] == L"all")
	{
		for (int i = 0; i < NumTemplates; i++)
		{
			outfile << TemplateNames[i] << std::endl;  // Her dosya ismini satýr satýr yaz.
			TemplateFlags[i] = true;
		}
	}
	else
	{
		for (int i = 0; i < NumTemplates; i++)
		{
			TemplateFlags[i] = false;
		}
		for (int i = 0; i < Filenames.size(); i++)
		{
			auto it = std::find(TemplateNames.begin(), TemplateNames.end(), Filenames[i]);
			if (it != TemplateNames.end()) {
				int index = std::distance(TemplateNames.begin(), it);
				//std::wcout << L"Eleman " << Filenames[i] << L" index " << index << L" de bulundu." << std::endl;
				outfile << Filenames[i] << std::endl;  // Her dosya ismini satýr satýr yaz.
				TemplateFlags[index] = true;
			}
			else {
				//std::wcout << L"Eleman bulunamadý." << std::endl;
				continue;
			}
		}
	}
	outfile.close();  // Dosyayý kapatalým.
	return 0;
}

int ShapeDetector::set_detector_params(int MaxNumMatches, double ObjPixRatioThrMin, double ObjPixRatioThrMax, int MaxAngleDeviation, int AngleSearchStep, double ScaleSearchMin, double ScaleSearchMax, double ScaleSearchStep)
{

	//TODO: check if the parameter values are correct/allowed

	maxAngleDeviation = MaxAngleDeviation;
	angleSearchStep = AngleSearchStep;
	scaleSearchMin = ScaleSearchMin;
	scaleSearchMax = ScaleSearchMax;
	scaleSearchStep = ScaleSearchStep;
	objPixRatioThrMin = ObjPixRatioThrMin;
	objPixRatioThrMax = ObjPixRatioThrMax;
	maxNumMatches = MaxNumMatches;

	return 0;
}
int ShapeDetector::locate_shape(const cv::Mat Image)
{
	// preprocessing
	cv::Mat img, imgOrig;
	maxRect.width = 0;
	maxRect.height = 0;
	cv::cvtColor(Image, img, cv::COLOR_BGR2GRAY);
	//cv::flip(img, img, 3);
	imgOrig = img.clone();
	int original_width = img.cols;
	int original_height = img.rows;
	int new_width = original_width * imageScale;
	int new_height = original_height * imageScale;
	cv::resize(img, img, cv::Size(new_width, new_height));
	cv::bitwise_not(img, img);
	cv::threshold(img, img, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

	//check0: if there is movement in the image
	if (numFrames == 0)
	{
		prevFrame = img.clone();
		numFrames++;
	}
	else
	{
		cv::Mat diff;
		cv::subtract(img, prevFrame, diff);

		// Farkýn mutlak deðerini al
		diff = cv::abs(diff);

		// Mutlak deðeri alýnmýþ farkýn toplamýný bul
		double sumDiff = cv::sum(diff)[0];
		if (sumDiff > 20000)
		{
			Output.set_output_empty();
			prevFrame = img.clone();
			numFrames++;
			return 0;
		}
		else if (sumDiff < 10000 && Output.numTemplateFound)
		{
			prevFrame = img.clone();
			numFrames++;
			return 0;

		}
		else
		{
			Output.set_output_empty();
			prevFrame = img.clone();
			numFrames++;
		}

	}

	//check1: if there is no object in the image
	int whitePixels = cv::countNonZero(img);
	int totalPixels = img.total();
	double whiteRatio = (double)whitePixels / totalPixels;



	if (whiteRatio >= objPixRatioThrMax || whiteRatio <= objPixRatioThrMin) {
		Output.set_output_empty();
		return 0;
	}
	Output.set_output_empty();
	//find relevant area in the image and process only this area to save time
	cv::Mat labels, stats, centroids;
	int numOfLabels = cv::connectedComponentsWithStats(img, labels, stats, centroids);
	int maxArea = 0;

	for (int i = 1; i < numOfLabels; i++) {
		int area = stats.at<int>(i, cv::CC_STAT_AREA);
		cv::Rect rect(stats.at<int>(i, cv::CC_STAT_LEFT),
			stats.at<int>(i, cv::CC_STAT_TOP),
			stats.at<int>(i, cv::CC_STAT_WIDTH),
			stats.at<int>(i, cv::CC_STAT_HEIGHT));

		if (area > maxArea) {
			maxArea = area;
			maxRect = rect;
		}
	}

	maxRect.x /= imageScale;
	maxRect.y /= imageScale;
	maxRect.width /= imageScale;
	maxRect.height /= imageScale;


	return 0;
}

int ShapeDetector::detect_shape(const cv::Mat Image)
{
	// preprocessing
	cv::Mat img, imgOrig;
	cv::cvtColor(Image, img, cv::COLOR_BGR2GRAY);
	//cv::flip(img, img, 3);
	imgOrig = img.clone();
	int original_width = img.cols;
	int original_height = img.rows;
	int new_width = original_width * imageScale;
	int new_height = original_height * imageScale;
	cv::resize(img, img, cv::Size(new_width, new_height));
	cv::bitwise_not(img, img);
	cv::threshold(img, img, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
	/*
	// Ýþlenmiþ görüntüyü göster
	cv::namedWindow("Ýkili Görüntü", cv::WINDOW_NORMAL); // Pencere oluþtur
	cv::imshow("Ýkili Görüntü", img); // Görüntüyü pencerede göster
	*/
	// Kullanýcý bir tuþa basana kadar bekle
	cv::waitKey(0);
	foundObjectStatic = false;
	//check0: if there is movement in the image
	if (numFrames == 0)
	{
		prevFrame = img.clone();
		numFrames++;
	}
	else
	{
		cv::Mat diff;
		cv::subtract(img, prevFrame, diff);

		// Farkýn mutlak deðerini al
		diff = cv::abs(diff);

		// Mutlak deðeri alýnmýþ farkýn toplamýný bul
		double sumDiff = cv::sum(diff)[0];
		if (sumDiff > 20000)
		{
			Output.set_output_empty();
			prevFrame = img.clone();
			numFrames++;
			return 0;
		}
		else if (sumDiff < 10000 && Output.numTemplateFound)
		{
			prevFrame = img.clone();
			numFrames++;
			foundObjectStatic = true;
			return 0;

		}
		else
		{
			Output.set_output_empty();
			prevFrame = img.clone();
			numFrames++;
		}

	}

	//check1: if there is no object in the image
	int whitePixels = cv::countNonZero(img);
	int totalPixels = img.total();
	double whiteRatio = (double)whitePixels / totalPixels;

	

	if (whiteRatio >= objPixRatioThrMax || whiteRatio <= objPixRatioThrMin) {
		Output.set_output_empty();
		return 0;
	}
	Output.set_output_empty();
	//find relevant area in the image and process only this area to save time
	cv::Mat labels, stats, centroids;
	int numOfLabels = cv::connectedComponentsWithStats(img, labels, stats, centroids);
	int maxArea = 0;
	for (int i = 1; i < numOfLabels; i++) {
		int area = stats.at<int>(i, cv::CC_STAT_AREA);
		cv::Rect rect(stats.at<int>(i, cv::CC_STAT_LEFT),
			stats.at<int>(i, cv::CC_STAT_TOP),
			stats.at<int>(i, cv::CC_STAT_WIDTH),
			stats.at<int>(i, cv::CC_STAT_HEIGHT));

		if (area > maxArea) {
			maxArea = area;
			maxRect = rect;
		}
	}

	// draw a bbox around the largest CC
	cv::Mat cropped = img(maxRect);
	int cropPad = 40 * imageScale;
	cv::copyMakeBorder(cropped, cropped, cropPad, cropPad, cropPad, cropPad, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
	//cv::imshow("Cropped window", cropped);
	//cv::waitKey(10);

	// Paralel eþleþtirme sýnýfýnýn örneðini oluþtur
	std::vector<double> minCorrelation;
	std::vector<int> bestMatchIndex;
	std::vector<int> bestMatchAngle;
	std::vector<double> bestMatchScale;
	std::vector<cv::Rect> bestMatchLoc;
	int numTemplateFound = 0;
	ParallelTemplateMatching parallelTemplateMatching(cropped, Templates, TemplatesF, TemplateFlags, minCorrelation, bestMatchIndex, bestMatchAngle, bestMatchScale, bestMatchLoc, numTemplateFound, NumTemplates, maxAngleDeviation, angleSearchStep, scaleSearchMin, scaleSearchMax, scaleSearchStep, maxNumMatches);
	
	// Paralel eþleþtirme sýnýfýný cv::parallel_for_ ile kullan
	cv::parallel_for_(cv::Range(0, NumTemplates), parallelTemplateMatching);

	// Korelasyon eþik deðerinin altýndaysa þablon bulunamadý
	if (numTemplateFound==0)
	{  
		//std::cout << "Template could not be found in the image" << std::endl;
		return 0;
	}
	else
	{

		//sort et
		std::vector<int> indices(numTemplateFound);
		for (int i = 0; i < numTemplateFound; ++i)
		{
			indices[i] = i;
		}

		// minCorrelation ile birlikte indeksleri sýralayýn
		std::sort(indices.begin(), indices.end(),
			[&](int a, int b) { return minCorrelation[a] > minCorrelation[b]; });

		// Yeni sýralanmýþ vektörler oluþtur
		std::vector<double> sortedMinCorrelation(numTemplateFound);
		std::vector<int> sortedBestMatchIndex(numTemplateFound);
		std::vector<int> sortedBestMatchAngle(numTemplateFound);
		std::vector<double> sortedBestMatchScale(numTemplateFound);
		std::vector<cv::Rect> sortedBestMatchLoc(numTemplateFound);

		int maxNumOut = min(numTemplateFound, maxNumMatches);
		for (int i = 0; i < maxNumOut; ++i) {
			sortedMinCorrelation[i] = minCorrelation[indices[i]];
			sortedBestMatchIndex[i] = bestMatchIndex[indices[i]];
			sortedBestMatchAngle[i] = bestMatchAngle[indices[i]];
			sortedBestMatchScale[i] = bestMatchScale[indices[i]];
			sortedBestMatchLoc[i] = bestMatchLoc[indices[i]];
		}

		// Orijinal vektörleri güncelle
		minCorrelation = sortedMinCorrelation;
		bestMatchIndex = sortedBestMatchIndex;
		bestMatchAngle = sortedBestMatchAngle;
		bestMatchScale = sortedBestMatchScale;
		bestMatchLoc = sortedBestMatchLoc;

		Output.numTemplateFound = maxNumOut;
		Output.bestMatchIndex = bestMatchIndex;
		Output.bestMatchCorr = minCorrelation;
		Output.bestMatchAngle = bestMatchAngle;
		Output.bestMatchScale = bestMatchScale;

		
		for (int i = 0; i < maxNumOut; i++)
		{
			Output.bestMatchLoc.push_back(maxRect);
			Output.bestMatchLoc[i].x /= imageScale;
			Output.bestMatchLoc[i].y /= imageScale;
			Output.bestMatchLoc[i].width /= imageScale;
			Output.bestMatchLoc[i].height /= imageScale;
			Output.bestMatchName.push_back(TemplateNames[bestMatchIndex[i]]);
		}
	}

	return 0;
}

cv::Mat ShapeDetector::get_template_image(const int Index)
{
	if (Index < NumTemplates && Index >= 0)
	{
		return Templates[Index];
	}
	else
		std::cout << "bad Index" << std::endl;
}
