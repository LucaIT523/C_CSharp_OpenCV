#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <windows.h>

class DetectionResult {
private:

public:
	int numTemplateFound;				//a flag to show that the detection is successful
	std::vector<int> bestMatchIndex;				//the index of the detected template 
	std::vector <std::wstring> bestMatchName;		//the file name of the detected template image
	std::vector<int> bestMatchAngle;				//the angle of the  detected object
	std::vector<double> bestMatchScale;			//the scale of the detefcted object
	std::vector <cv::Rect> bestMatchLoc;			//the bounding box coordinated of the detected object in the image
	std::vector<double> bestMatchCorr;			//the confidence of the detection

	int set_output_empty();
};

class ShapeDetector {
private:
	//template params
	double tempReadScale = 0.2;						//the downsampling factor the template images 
	std::vector<cv::Mat> Templates, TemplatesF;		
	std::vector<std::wstring> TemplateFiles;		
	std::vector<std::wstring> TemplateNames;		
	std::vector<bool> TemplateFlags;
	
	cv::Mat prevFrame;
	int numFrames = 0;



	std::vector<std::wstring> getFilesInDirectory(const std::wstring& directory, const std::wstring& ext);
	std::wstring extractFileName(const std::wstring& filepath);

public:

	//detector params
	double imageScale = 0.2;
	int maxAngleDeviation = 9;		//maximum angle deviation that is wanted to be detected (higher values increase search time) 
	int angleSearchStep = 6;			//resolution of angle search (lower values increase search time)
	double scaleSearchMin = 0.50;		//minimum scale change in the template image that is wanted to be detected in the input image
	double scaleSearchMax = 0.60;		//maximum scale change in the template image that is wanted to be detected in the input image
	double scaleSearchStep = 0.03;	//resolution of the scale change in the template image that is wanted to be detected in the input image
	double objPixRatioThrMax = 0.10;
	double objPixRatioThrMin = 0.001;
	int maxNumMatches = 5;
	int NumTemplates=0;
	DetectionResult Output;
	cv::Rect maxRect;
	bool foundObjectStatic = false;
	/*
	read_shape_directory: it reads the image files in a given directory, calculates and stores the necessary shape feature vectors upfront. it is necessary to call this function to initialize the shape detector
	inputs: 
		Directory: the location of the template images, e.g. "c:\\templates"
		Extension: the file extension of the template images, can be one of the following: ".jpg", ".png", ".bmp" 
	*/
	int read_shape_directory(const std::wstring Directory, const std::wstring Extension);
	


	/*
	set_templates_to_detect: it defines which templates are to be searched in the input images. if not set explicitly, all templates are searched in default.
	inputs: 
		Filenames: a vector of the name of the files of the templates that are going to be in the search space. The filename doesn't include the extension here
	*/
	int set_templates_to_detect(const std::vector<std::wstring> Filenames);						



	/*
	set_detector_params: can be used to set the detection parameters when needed. Check ShapeDetector class for the parameter definitions.
	*/
	int set_detector_params(int MaxNumMatches, double ObjPixRatioThrMin, double ObjPixRatioThrMax, int MaxAngleDeviation, int AngleSearchStep, double ScaleSearchMin, double ScaleSearchMax, double ScaleSearchStep);



	/*
	detect_shape: the function to be called for shape detection. Be sure to initialize the ShapeDetector object with "read_shape_directory" before calling this function.
	inputs:
		Image: input image
	outputs:
		Output: check DetectionResult class for the output parameters
	*/
	int detect_shape(const cv::Mat Image);	

	int locate_shape(const cv::Mat Image);

	cv::Mat get_template_image(const int Index);
};