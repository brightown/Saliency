//////////////////////////////////////////////////////////////////////////
// class for analyzing shapes
// jiefeng@2014-3-27
//////////////////////////////////////////////////////////////////////////

#pragma once


#include "common.h"



class ShapeAnalyzer
{
public:
	ShapeAnalyzer(void);

	// get basic contour shapes
	bool ExtractShapes(const cv::Mat& img, double cannyTh_low, double cannyTh_high, int contour_mode, vector<BasicShape>& shapes);
};

