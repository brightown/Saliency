//////////////////////////////////////////////////////////////////////////
// generic object detector
// jiefeng@2014-3-14
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "common.h"
#include "ImgVisualizer.h"
//#include "WindowEvaluator.h"
//#include "ImageSpaceManager.h"
#include "DataManager/DatasetManager.h"
#include "ObjectSegmentor.h"


struct WinConfig
{
	int width;
	int height;
	WinConfig(int w, int h) { width = w; height = h; }
};

class GenericObjectDetector
{
private:

	Mat Gx, Gy;
	Mat Gmag, Gdir;
	Mat integralGx, integralGy;
	Size imgSize;
	Mat depthMap;
	Mat img;

	cv::TermCriteria shiftCrit;

	DatasetManager db_man;
	visualsearch::ImageSegmentor segmentor;

	vector<WinConfig> winconfs;


	bool ShiftWindow(const Point& seedPt, Size winSz, Point& newPt);

	double ComputeObjectScore(Rect win);

	double ComputeDepthVariance(Rect win);

public:
	
	GenericObjectDetector(void);

	bool Preprocess(const cv::Mat& color_img);

	bool test();

	bool Run(const cv::Mat& color_img);
	
	bool RunSlidingWin(const cv::Mat& color_img, Size winsz);

};

