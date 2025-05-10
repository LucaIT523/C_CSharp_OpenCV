#pragma once

#include "pch.h"
#include <iostream>
#include "jetclass.h"

#define		ERR_DLL_INIT		-99

struct ST_ActiveTemplateName
{
	char*	data;
};

extern "C" {

//. Init 
__declspec(dllexport) ShapeDetector* cs_Init();

//. 
__declspec(dllexport) int cs_read_shape_directory(ShapeDetector* p_Handle, char*	p_directory, char* p_extension);

//. 
__declspec(dllexport) int cs_set_templates_to_detect(ShapeDetector* p_Handle, ST_ActiveTemplateName	p_stActiveTemplateNames[] , int p_nItemCount);

//. 
__declspec(dllexport) int cs_set_detector_params(ShapeDetector* p_Handle, int MaxNumMatches, double ObjPixRatioThrMin, double ObjPixRatioThrMax, int MaxAngleDeviation, int AngleSearchStep, double ScaleSearchMin, double ScaleSearchMax, double ScaleSearchStep);

//.
__declspec(dllexport) cv::Mat* cs_new_image(char* p_ImgPath);

//.
__declspec(dllexport) void cs_del_image(cv::Mat* p_img);

//.
__declspec(dllexport) int cs_detect_shape(ShapeDetector* p_Handle, char* p_imgdata, int p_nW, int p_nH, int p_nCh);

//.
__declspec(dllexport) int cs_locate_shape(ShapeDetector* p_Handle, char* p_img, int p_nW, int p_nH, int p_nCh);

//.
__declspec(dllexport) int cs_gnumTemplateFound(ShapeDetector* p_Handle);

//.
__declspec(dllexport) char* cs_getbestMatchName(ShapeDetector* p_Handle, int	p_nID);

//.
__declspec(dllexport) char* cs_getbestMatchCorr(ShapeDetector* p_Handle, int	p_nID);

//.
__declspec(dllexport) char* cs_getbestMatchScale(ShapeDetector* p_Handle, int	p_nID);

//.
__declspec(dllexport) char* cs_getbestMatchAngle(ShapeDetector* p_Handle, int	p_nID);


//. Close
__declspec(dllexport) void cs_Close(ShapeDetector* p_Handle);




}

