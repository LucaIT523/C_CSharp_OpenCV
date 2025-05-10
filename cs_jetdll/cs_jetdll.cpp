#include "pch.h"
#include "cs_jetdll.h"
#include <string>
#include <locale>
#include <codecvt>


std::wstring stringToWstring(const std::string& t_str)
{
	//setup converter
	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;

	//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
	return converter.from_bytes(t_str);
}
cv::Mat ConvertToCvMat(char* imageData, int width, int height, int channels)
{
	cv::Mat mat(height, width, CV_MAKETYPE(CV_8UC3, channels));
	memcpy(mat.data, imageData, width * height * channels);
	return mat;
}

ShapeDetector* cs_Init()
{
	long			w_Handle = NULL;
	ShapeDetector*	w_detector = NULL;

	//. 
	w_detector = new ShapeDetector;

	if (w_detector == NULL) {
		return NULL;
	}
	w_detector->Output.set_output_empty();

	//. OK
	return w_detector;
}

void cs_Close(ShapeDetector* p_Handle)
{
	ShapeDetector* w_detector = NULL;

	if (p_Handle == NULL) {
		return;
	}

	delete p_Handle;
	return;
}

int cs_read_shape_directory(ShapeDetector* p_Handle, char* p_directory, char* p_extension)
{
	int				w_nRtn = -1;
	ShapeDetector*	w_detector = NULL;

	if (p_Handle == NULL) {
		return ERR_DLL_INIT;
	}

	std::string	 str_directory = p_directory;
	std::wstring wstr_directory = stringToWstring(str_directory);
	std::string	 str_extension = p_extension;
	std::wstring wstr_extension = stringToWstring(str_extension);

	w_nRtn = p_Handle->read_shape_directory(wstr_directory, wstr_extension);

	return w_nRtn;
}

int cs_set_templates_to_detect(ShapeDetector* p_Handle, ST_ActiveTemplateName	p_stActiveTemplateNames[], int p_nItemCount)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return ERR_DLL_INIT;
	}
	std::vector<std::wstring> activeTemplateNames;

	for (int i = 0; i < p_nItemCount; i++) {
		std::string	 str_activeTemplateName = p_stActiveTemplateNames[i].data;
		std::wstring wstr_activeTemplateName = stringToWstring(str_activeTemplateName);
		activeTemplateNames.push_back(wstr_activeTemplateName);
	}

	w_nRtn = p_Handle->set_templates_to_detect(activeTemplateNames);

	return w_nRtn;
}

int cs_set_detector_params(ShapeDetector* p_Handle, int MaxNumMatches, double ObjPixRatioThrMin, double ObjPixRatioThrMax, int MaxAngleDeviation, int AngleSearchStep, double ScaleSearchMin, double ScaleSearchMax, double ScaleSearchStep)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return ERR_DLL_INIT;
	}

	w_nRtn = p_Handle->set_detector_params(MaxNumMatches, ObjPixRatioThrMin, ObjPixRatioThrMax, MaxAngleDeviation, AngleSearchStep, ScaleSearchMin, ScaleSearchMax, ScaleSearchStep);

	return w_nRtn;

}
cv::Mat* cs_new_image(char* p_ImgPath)
{
	int				w_nRtn = -1;

	cv::Mat* w_RtnImg = new cv::Mat(cv::imread(p_ImgPath, cv::IMREAD_COLOR));

	return w_RtnImg;
}
//.
void cs_del_image(cv::Mat* p_img)
{
	delete p_img;
	return;
}

int cs_locate_shape(ShapeDetector* p_Handle, char* p_img, int p_nW, int p_nH, int p_nCh)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return ERR_DLL_INIT;
	}
	
	cv::Mat img = ConvertToCvMat((char*)p_img, p_nW, p_nH, p_nCh);
	w_nRtn = p_Handle->locate_shape(img);

	return w_nRtn;
}


int cs_detect_shape(ShapeDetector* p_Handle, char* p_img, int p_nW, int p_nH, int p_nCh)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return ERR_DLL_INIT;
	}
	cv::Mat img = ConvertToCvMat((char*)p_img, p_nW, p_nH, p_nCh);
	w_nRtn = p_Handle->detect_shape(img);

	return w_nRtn;
}

int cs_gnumTemplateFound(ShapeDetector* p_Handle)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return ERR_DLL_INIT;
	}

	return p_Handle->Output.numTemplateFound;
}

char* cs_getbestMatchName(ShapeDetector* p_Handle, int	p_nID)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return NULL;
	}

	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string bestMatchName = converter.to_bytes(p_Handle->Output.bestMatchName[p_nID]);
	
	return _strdup(bestMatchName.c_str());
}


char* cs_getbestMatchCorr(ShapeDetector* p_Handle, int	p_nID)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return NULL;
	}

	std::string bestMatchCorr = std::to_string(p_Handle->Output.bestMatchCorr[p_nID]);
	return _strdup(bestMatchCorr.c_str());
}

char* cs_getbestMatchScale(ShapeDetector* p_Handle, int	p_nID)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return NULL;
	}

	std::string bestMatchScale = std::to_string(p_Handle->Output.bestMatchScale[p_nID]);
	return _strdup(bestMatchScale.c_str());
}
char* cs_getbestMatchAngle(ShapeDetector* p_Handle, int	p_nID)
{
	int				w_nRtn = -1;

	if (p_Handle == NULL) {
		return NULL;
	}

	std::string bestMatchAngle = std::to_string(p_Handle->Output.bestMatchAngle[p_nID]);

	return _strdup(bestMatchAngle.c_str());
}