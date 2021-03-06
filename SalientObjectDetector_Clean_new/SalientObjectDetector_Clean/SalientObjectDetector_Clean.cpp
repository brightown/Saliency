// SalientObjectDetector_Clean.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <SalientRegionDetector.h>

#include <BitmapHelper.h>
#include <direct.h>

#include "FolderHelper.h"
#include "time_ex.h"




// multiple nms;  
int _tmain(int argc, _TCHAR* argv[])
{

	SalientRegionDetector detector;

	// set parameters
	if ( !detector.read_para(argc, argv) )
		return -1;

	// input image directory name
	string imgdir(detector.g_para.g_imagefile);
	if(imgdir[imgdir.length()-1] == '\\')
		imgdir = imgdir.substr(0, imgdir.length()-1);

	// save path
	string saveroot(detector.g_para.g_outfile);
	if(saveroot[saveroot.length()-1] == '\\')
		saveroot = saveroot.substr(0, saveroot.length()-1);
	saveroot += "\\";

	float total_runtime;	// total processing time
	vector<string> picnames;	// image list
	int ret = GetPicFilesinDir(imgdir.c_str(), picnames);
	string runinfo_file = saveroot + "runinfo.txt";
	ofstream runinfo(runinfo_file.c_str());
	runinfo<<"img_name img_sz(w,h) resize_time seg_time seg_num init_time detection_time obj_num total_time"<<endl;

	Timer tot_t;
	for(size_t id=0; id<picnames.size(); id++)
	{
		// get pure image name
		// find last "\\" and "."
		int startpos = picnames[id].find_last_of("\\")+1;
		int endpos = picnames[id].find_last_of(".")-1;
		string fname = picnames[id].substr(startpos, endpos-startpos+1);	// only image name
		string imgfilename = picnames[id];

		printf("\nStart processing image: %s(%d/%d, %.2f%%).\n", picnames[id].c_str(), id+1, picnames.size(), (float)(id+1)*100/picnames.size());

		// create output directory
		// return: 0 - success; -1: already exist
		int ret = mkdir(saveroot.c_str());
		string saveimgprefix = saveroot + fname;

		// read image
		wstring imgname = string2wstring(imgfilename);
		Bitmap img( imgname.c_str() );
		if (img.GetLastStatus() != Ok) continue;
		// note that in PixelFormat24bppRGB bmp, color is packed as (B,G,R) instead of (R,G,B)
		//	this may cause failure for some image
		if (img.GetPixelFormat() != PixelFormat24bppRGB) continue;	// 32 fail

		// downsample image
		float down_ratio;
		Size newSz;
		Size oldSz(img.GetWidth(), img.GetHeight());
		down_ratio = SalientRegionDetector::compute_downsample_ratio(oldSz, detector.g_para.downSampleFactor, newSz);
		Bitmap new_img(newSz.Width, newSz.Height, img.GetPixelFormat());
		{
			Timer t;
			SalientRegionDetector::down_sample_image(img, down_ratio, new_img);
			detector.g_runinfo.downsize_t = t.Stamp();
			printf("down size (%.2f) image to %d %d: %.1f ms\n", down_ratio, new_img.GetWidth(), new_img.GetHeight(), t.Stamp()*1000);
		}

		BitmapData bmp_data;
		new_img.LockBits(&Rect(0, 0, new_img.GetWidth(), new_img.GetHeight()), ImageLockModeRead, PixelFormat24bppRGB, &bmp_data);

		// run algorithm
		{
			Timer t;

			// init
			Timer init_t;
			detector.Init(bmp_data);
			detector.g_runinfo.init_t = init_t.Stamp();

			// run detection
			int num_scales = detector.RunMultiSlidingWindow();

			detector.g_runinfo.det_t = t.Stamp();
			printf("\ndetection time (%d scales) : %.2f s.\n", num_scales, t.Stamp());
		}
		

		// save result
		{
			new_img.UnlockBits(&bmp_data);

			// upsample result size to original image
			if (down_ratio != 1)
				for (unsigned int i = 0; i < detector.all_objs.size(); i++)
				{
					for( size_t j = 0; j < detector.all_objs[i].size(); j++ )
					{
						ScoredRect& rc = detector.all_objs[i][j];
						rc.X = rc.X / down_ratio + 0.5;
						rc.Y = rc.Y / down_ratio + 0.5;
						rc.Width = rc.Width / down_ratio + 0.5;
						rc.Height = rc.Height / down_ratio + 0.5;
					}
				}

			// save results to data file
			detector.SaveDetectionResults(saveimgprefix);

			// save result image (original size)
			for( size_t i = 0; i < detector.nms_vals.size(); i++ )
			{
				char str[30];
				sprintf(str, "%.2f", detector.nms_vals[i]);

				Bitmap *buffer = img.Clone(0,0,img.GetWidth(), img.GetHeight(), img.GetPixelFormat());

				Graphics g(buffer);

				detector.DrawResult(&g, down_ratio, detector.all_objs[i]);

				string result_filename = saveimgprefix + "_" + string(str) + "_res.jpg";
				if ( !SaveBitmap(*buffer, L"image/jpeg", string2wstring(result_filename).c_str()) )
					cout << "fail to save image result in " << result_filename << endl;

				delete buffer;
				buffer = NULL;
			}

			if(detector.g_para.useBGMap)
			{
				// save bg map image
				detector.SaveBGMap(saveimgprefix);
			}

			// output total processing time
			detector.g_runinfo.tol_img_t = \
				detector.g_runinfo.det_t + detector.g_runinfo.downsize_t + \
				detector.g_runinfo.init_t + detector.g_runinfo.seg_t;
			printf("Total time for detection: %.2fs\n", detector.g_runinfo.tol_img_t);

			// output runtime info to file
			runinfo<<fname<<" "<<new_img.GetWidth()<<" "<<new_img.GetHeight()
				<<" "<<detector.g_runinfo.downsize_t<<" "<<detector.g_runinfo.seg_t
				<<" "<<detector.g_runinfo.seg_num<<" "<<detector.g_runinfo.init_t
				<<" "<<detector.g_runinfo.det_t<<" "<<detector.g_runinfo.det_num
				<<" "<<detector.g_runinfo.tol_img_t<<endl;
		}
	}

	total_runtime = tot_t.Stamp();
	printf("\nAverage time for one image: %.2fs\n", total_runtime/picnames.size());
	

	return 0;
}