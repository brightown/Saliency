//////////////////////////////////////////////////////////////////////////
//	superpixel segmentation
//	fengjie@cis.pku
//	2011/9/6
//////////////////////////////////////////////////////////////////////////


#pragma once

#include "../Common/common_libs.h"


namespace visualsearch
{
	struct SuperPixel
	{
		// shape
		cv::Mat mask;
		cv::Rect box;
		bool boundary;
		Contour contour;
		Contour approx_contour;
		int perimeter;
		bool isConvex;
		int area;
		// spatial
		float meanDepth;
		Point2f centroid;

		// features
		MatFeatureSet feats;
		Scalar meancolor;
	};

	// mainly for oversegmentation
	class ImageSegmentor
	{
	public:

		// params
		float m_dSmoothSigma;
		float m_dThresholdK;
		int m_dMinArea;

	public:

		std::vector<SuperPixel> superPixels;
		
		cv::Mat m_segImg;
		cv::Mat m_idxImg;	// superpixel index
		cv::Mat m_mean_img;	// set mean color to pixels in same segment
		cv::Mat m_adjacency_mat;	// adjacency matrix for superpixels

	public:

		ImageSegmentor(void);

		int DoSegmentation(const cv::Mat& img);

		// this is used for any input collection
		bool ComputeAdjacencyMat(const std::vector<SuperPixel>& sps, cv::Mat& adjacencyMat);
	};

}
