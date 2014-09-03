

#pragma once

#include "ObjectRanker.h"
#include "Common/common_libs.h"
#include "IO/Camera/KinectDataMan.h"
#include "IO/Camera/OpenCVCameraIO.hpp"
#include "Processors/Attention/BingObjectness.h"
#include "Common/tools/ImgVisualizer.h"
#include "Processors/nms.h"
#include "Processors/Segmentation/IterativeSegmentor.h"

//////////////////////////////////////////////////////////////////////////
// general video demo components
// 1. raw data input
// 2. processing
// 3. visualization
//////////////////////////////////////////////////////////////////////////

enum DemoType
{
	DEMO_SAL,
	DEMO_OBJECT_WIN,
	DEMO_OBJECT_SEG
};

using namespace visualsearch::common;
using namespace visualsearch::io::camera;

class ObjProposalDemo
{
private:
	visualsearch::processors::attention::BingObjectness bing;
	visualsearch::processors::attention::ObjectRanker ranker;
	visualsearch::tools::ImgVisualizer imgvis;
	visualsearch::processors::attention::SaliencyComputer salcomputer;
	visualsearch::processors::segmentation::IterativeSegmentor iterSegmentor;

	string DATADIR;

	int frameid;

public:

	ObjProposalDemo(void);

	bool RunObjSegProposal(Mat& cimg, Mat& dmap);

	bool RunObjWinProposal(Mat& cimg, Mat& dmap);

	bool RunSaliency(Mat& cimg, Mat& dmap, visualsearch::processors::attention::SaliencyType saltype);

	bool RunVideoDemo(SensorType stype, DemoType dtype);

};
