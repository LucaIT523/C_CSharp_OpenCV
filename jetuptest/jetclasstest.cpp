#include "jetclass.h"
#include <string>
#include <locale>
#include <codecvt>

int main()
{

    //create detector class
    ShapeDetector detector;
    detector.Output.set_output_empty();
    
    //read template files
    const std::wstring directory = L"ince_tasarimlar"; // Windows format
    const std::wstring extension = L".png";
    int resRead = detector.read_shape_directory(directory, extension);
    if (resRead == -1)
    {
        std::cout << "Template folder couldn't be read." << std::endl;
        return 0;
    }
    else
    {
        std::cout << "Template folder has been read." << std::endl;
    }

    //set flags for the active templates
    std::vector<std::wstring> activeTemplateNames = { L"all" };             //klasordeki tum resimler kullanilsin
    int resTempRead = detector.set_templates_to_detect(activeTemplateNames);
    if (resTempRead == -1)
    {
        std::cout << "activeTemplates.txt file couldn't be created!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "activeTemplates.txt file has been created!" << std::endl;
    }

    //call this to change detector params when necessary
    detector.set_detector_params(10, 0.001, 0.5, 9, 3, 0.5, 0.6, 0.03);


    //start locating test
    cv::Mat img = cv::imread("image_2023-11-24_14-33-57.png", cv::IMREAD_COLOR);
    int resLocate = detector.locate_shape(img);

    //show localization result
    //cv::Mat imgClone = img.clone();
    //if (detector.maxRect.width > 0)
    //    cv::rectangle(imgClone, detector.maxRect, cv::Scalar::all(0), 2, 8, 0);
    //cv::namedWindow("Locating", cv::WINDOW_NORMAL); // Pencere oluþtur
    //cv::imshow("Locating", imgClone); // Görüntüyü pencerede göster


    //start detection test
    std::clock_t start = std::clock();  // start time
    int resDetect = detector.detect_shape(img);
    std::clock_t end = std::clock();  // end time
    double elapsedMillis = (double(end - start) / CLOCKS_PER_SEC) * 1000;  // calculate the time in ms
    
                                                                           
    //show the result
    if (resDetect == -1)
    {
        std::cout << "error detecting the shape!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "matched with "<< detector.Output.numTemplateFound << "template images" << std::endl;
        for (int i = 0; i < detector.Output.numTemplateFound; i++)
        {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
            std::string bestMatchName = converter.to_bytes(detector.Output.bestMatchName[i]);
            std::string bestMatchCorr = std::to_string(detector.Output.bestMatchCorr[i]);
            std::string bestMatchScale = std::to_string(detector.Output.bestMatchScale[i]);
            std::string bestMatchAngle = std::to_string(detector.Output.bestMatchAngle[i]);

            std::cout << bestMatchName << " detected with confidence: " << bestMatchCorr << ".Angle: " << bestMatchAngle << ", Scale : " << bestMatchScale << ", in time:" << elapsedMillis << std::endl;

        }
    }

}