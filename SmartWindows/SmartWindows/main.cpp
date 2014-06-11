

//#include "ImageSpaceManager.h"
//#include "WindowEvaluator.h"
#include "GenericObjectDetector.h"
#include "ShapeAnalyzer.h"
#include "ImgVisualizer.h"
#include "DataManager/DatasetManager.h"
#include "DataManager/NYUDepth2DataMan.h"
#include <string>
#include "ObjectSegmentor.h"
#include "a9wins/A9Window.h"
#include "Saliency/Composition/SalientRegionDetector.h"
#include "ObjectTester.h"
#include "Saliency/Depth/DepthSaliency.h"
using namespace std;

int main()
{


	ObjectTester tester;
	tester.TestObjectRanking(DB_BERKELEY3D);
	//tester.RunVideoDemo();
	return 0;

	ShapeAnalyzer shaper;
	GenericObjectDetector detector;
	DatasetManager dbMan;
	dbMan.Init(DB_VOC07);
	visualsearch::ImageSegmentor segmentor;
	Berkeley3DDataManager b3dman;
	NYUDepth2DataMan nyuman;
	DepthSaliency dsal;

	// process
	if( !detector.InitBingObjectness() )
		return -1;

	string datadir = "E:\\Datasets\\RGBD_Dataset\\NYU\\Depth2\\";
	string imgfn = "159.jpg";
	Mat timg = imread(datadir + imgfn);
	if(timg.empty())
		return 0;

	Mat dimg;
	string dmapfn = datadir + "159_d.txt";
	nyuman.LoadDepthData(dmapfn, dimg);
	
	//dimg = dimg * 1000;
	//Mat cloud;
	//dsal.DepthToCloud(dimg, cloud);
	////dsal.OutputToOBJ(cloud, "temp.obj");
	//return 0;

	FileInfos imgfns;
	FileInfo tmpfns;
	tmpfns.filename = imgfn;
	tmpfns.filepath = datadir + imgfn;
	imgfns.push_back(tmpfns);
	map<string, vector<ImgWin>> rawgtwins;
	nyuman.LoadGTWins(imgfns, rawgtwins);
	vector<ImgWin> gtwins = rawgtwins[imgfn];

	//resize(timg, timg, Size(200,200));
	//imshow("input img", timg);
	visualsearch::ImgVisualizer::DrawFloatImg("dmap", dimg, Mat());
	visualsearch::ImgVisualizer::DrawImgWins("gt", timg, gtwins);
	waitKey(10);
	//Mat normimg;
	//normalize(timg, timg, 0, 255, NORM_MINMAX);

	//////////////////////////////////////////////////////////////////////////
	// get objectness proposal

	double start_t = cv::getTickCount();

	vector<ImgWin> boxes;

	detector.GetObjectsFromBing(timg, boxes, 500);

	Mat objectnessmap;
	//detector.CreateScoremapFromWins(timg.cols, timg.rows, boxes, objectnessmap);
	//visualsearch::ImgVisualizer::DrawFloatImg("objmap", objectnessmap, objectnessmap);

	std::cout<<"Bing time: "<<(cv::getTickCount()-start_t) / cv::getTickFrequency()<<"s."<<std::endl;
	
	// make images
	vector<Mat> imgs(boxes.size());
	for (int i=0; i<boxes.size(); i++)
	{
		imgs[i] = timg(boxes[i]);
	}

	Mat dispimg;
	visualsearch::ImgVisualizer::DrawImgCollection("objectness", imgs, 15, 15, dispimg);
	imshow("objectness", dispimg);
	visualsearch::ImgVisualizer::DrawImgWins("objdet", dimg, boxes);
	waitKey(10);
	
	// rank windows with CC
	/*SalientRegionDetector salDetector;
	salDetector.Init(timg);

	start_t = getTickCount();

	salDetector.RankWins(boxes);*/
	
	//////////////////////////////////////////////////////////////////////////
	// depth ranking
	dsal.RankWins(dimg, boxes);

	Mat salmap;
	//detector.CreateScoremapFromWins(timg.cols, timg.rows, boxes, salmap);
	//visualsearch::ImgVisualizer::DrawFloatImg("salmap", salmap, salmap);

	std::cout<<"Saliency time: "<<(cv::getTickCount()-start_t) / cv::getTickFrequency()<<"s."<<std::endl;

	// make images
	vector<ImgWin> topBoxes;
	for (int i=0; i<boxes.size(); i++)
	{
		cout<<boxes[i].score<<endl;
		imgs[i] = timg(boxes[i]);
		if(i<50)
			topBoxes.push_back(boxes[i]);
	}

	visualsearch::ImgVisualizer::DrawImgCollection("objectness", imgs, 15, 15, dispimg);
	imshow("rank by CC", dispimg);
	visualsearch::ImgVisualizer::DrawImgWins("ddet", dimg, topBoxes);
	visualsearch::ImgVisualizer::DrawImgWins("cdet", timg, topBoxes);
	waitKey(0);

	cv::destroyAllWindows();

	//getchar();

	return 0;
}