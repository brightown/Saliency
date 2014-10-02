#include "ObjectRanker.h"

namespace visualsearch
{
	namespace processors
	{
		namespace attention
		{
			ObjectRanker::ObjectRanker(void)
			{
				features::color::ColorFeatParams cparams;
				cparams.feat_type = features::color::COLOR_FEAT_MEAN;
				cparams.histParams.color_space = features::color::COLOR_LAB;
				colordesc.Init(cparams);



			}

			//////////////////////////////////////////////////////////////////////////

			bool ObjectRanker::RankSegments(const Mat& cimg, const Mat& dmap, const vector<SuperPixel>& sps, SegmentRankType rtype, vector<int>& orded_sp_ids)
			{
				if(rtype == SEG_RANK_CC)
					return RankSegmentsByCC(cimg, sps, orded_sp_ids);
				if(rtype == SEG_RANK_SALIENCY)
					return RankSegmentsBySaliency(cimg, dmap, sps, orded_sp_ids);

				return true;
			}

			bool ObjectRanker::RankWindowsBySaliency(const Mat& cimg, vector<ImgWin>& wins, vector<int>& ordered_win_ids)
			{
				map<float, int, greater<float>> win_scores;
				vector<Mat> feats;
				ComputeWindowRankFeatures(cimg, Mat(), wins, feats);
				for (size_t i=0; i<wins.size(); i++)
				{
					wins[i].score = sum(feats[i]).val[0]/feats[i].cols;
					win_scores[wins[i].score] = i;
				}

				ordered_win_ids.clear();
				ordered_win_ids.reserve(wins.size());
				for (map<float, int, greater<float>>::const_iterator pi=win_scores.begin();
					pi!=win_scores.end(); pi++)
				{
					ordered_win_ids.push_back(pi->second);
				}

				return true;
			}

			bool ObjectRanker::RankSegmentsBySaliency(const Mat& cimg, const Mat& dmap, const vector<SuperPixel>& sps, vector<int>& orded_sp_ids)
			{
				// compute saliency map
				Mat sal_map;
				salcomputer.ComputeSaliencyMap(cimg, SAL_HC, sal_map);
				imshow("salmap", sal_map);

				// compute saliency score for each superpixel
				map<float, int, greater<float>> sp_scores;
				for (size_t i=0; i<sps.size(); i++)
				{
					float objscore = cv::mean(sal_map, sps[i].mask).val[0];
					//float contextscore = 
					sp_scores[objscore] = i;
				}

				orded_sp_ids.clear();
				orded_sp_ids.reserve(sp_scores.size());
				for (auto pi=sp_scores.begin(); pi!=sp_scores.end(); pi++) {
					orded_sp_ids.push_back(pi->second);
				}

				return true;
			}

			bool ObjectRanker::RankSegmentsByCC(const Mat& cimg, const vector<SuperPixel>& sps, vector<int>& orded_sp_ids)
			{
				map<float, int, greater<float>> sp_scores;
				for (size_t i=0; i<sps.size(); i++)
				{
					float curscore = ComputeCenterSurroundColorContrast(cimg, sps[i]);
					sp_scores[curscore] = i;
				}

				orded_sp_ids.clear();
				orded_sp_ids.reserve(sp_scores.size());
				for (map<float, int, greater<float>>::const_iterator pi=sp_scores.begin();
					pi!=sp_scores.end(); pi++)
				{
					orded_sp_ids.push_back(pi->second);
				}

				return true;
			}

			bool ObjectRanker::ComputeSegmentRankFeature(const Mat& cimg, const Mat& dmap, SuperPixel& sp, Mat& feat)
			{
				vector<float> vals;
				// geometric features
				segprocessor.ExtractBasicSegmentFeatures(sp, cimg, dmap);
				vals.push_back(sp.area / (sp.mask.rows*sp.mask.cols));	// area percentage
				vals.push_back(sp.isConvex);
				vals.push_back(sp.meanDepth);
				vals.push_back(sp.area / sp.box.area());	// segment /box ratio
				vals.push_back(sp.perimeter / (2*(sp.box.width+sp.box.height)));	// segment perimeter / box perimeter ratio
				vals.push_back(sp.centroid.x / cimg.cols);
				vals.push_back(sp.centroid.y / cimg.rows);	// relative position in image

				// saliency features


				// convert to mat
				feat.create(1, vals.size(), CV_32F);
				for (size_t i=0; i<vals.size(); i++) feat.at<float>(i) = vals[i];

				return true;
			}

			bool ObjectRanker::ComputeWindowRankFeatures(const Mat& cimg, const Mat& dmap, vector<ImgWin>& wins, vector<Mat>& feats)
			{
				feats.clear();
				feats.resize(wins.size());
				// geometric features
				//vals.push_back(win.area() / (cimg.rows*cimg.cols));		// area percentage
				//vals.push_back(mean(dmap(win)).val[0]);					// mean depth
				//vals.push_back(win.width*1.0f / win.height);			// window ratio
				//vals.push_back((win.x+win.width/2)*1.0f / cimg.cols);
				//vals.push_back((win.y+win.height/2)*1.0f / cimg.rows);	// relative position in image

				// saliency features
				vector<Mat> salmaps(1);
				//salcomputer.ComputeSaliencyMap(cimg, SAL_FT, salmaps[0]);
				//salcomputer.ComputeSaliencyMap(cimg, SAL_SR, salmaps[1]);
				salcomputer.ComputeSaliencyMap(cimg, SAL_HC, salmaps[0]);
				//salcomputer.ComputeSaliencyMap(cimg, SAL_LC, salmaps[3]);

				for (size_t i=0; i<wins.size(); i++)
				{
					vector<float> vals;
					for (size_t j=0; j<salmaps.size(); j++)
					{
						//cout<<salmaps[j].rows<<" "<<salmaps[j].cols<<endl;
						vals.push_back(mean(salmaps[j](wins[i])).val[0]);		// mean ft sal
					}
					
					// convert to mat
					feats[i].create(1, vals.size(), CV_32F);
					for (size_t k=0; k<vals.size(); k++) feats[i].at<float>(k) = vals[k];
				}

				return true;
			}

			float ObjectRanker::ComputeCenterSurroundColorContrast(const Mat& cimg, const SuperPixel& sp)
			{
				// ignore object bigger than half the image
				if(sp.box.area() > cimg.rows*cimg.cols*0.5f)
					return 0;

				// context window
				ImgWin spbox(sp.box.x, sp.box.y, sp.box.width, sp.box.height);
				ImgWin contextWin = tools::ToolFactory::GetContextWin(cimg.cols, cimg.rows, spbox, 1.5);
				Mat contextMask(cimg.rows, cimg.cols, CV_8U);
				contextMask.setTo(0);
				contextMask(contextWin).setTo(1);
				contextMask.setTo(0, sp.mask);

				// compute color descriptors
				Mat contextColor;
				Mat spColor;
				colordesc.Compute(cimg, contextColor, contextMask);
				colordesc.Compute(cimg, spColor, sp.mask);

				float score = norm(contextColor, spColor, NORM_L2);
				return score;
			}

			bool ObjectRanker::LearnObjectPredictorFromNYUDepth()
			{
				string temp_dir = "E:\\Results\\objectness\\";	// save intermediate results
				char str[50];

				ImageSegmentor imgsegmentor;
				vector<float> seg_ths(3);
				seg_ths[0] = 50;
				seg_ths[1] = 100;
				seg_ths[2] = 200;

				// generate training samples from nyudata
				NYUDepth2DataMan nyuman;
				FileInfos imgfiles, dmapfiles;
				nyuman.GetImageList(imgfiles);
				nyuman.GetDepthmapList(dmapfiles);
				map<string, vector<Mat>> objmasks;
				imgfiles.erase(imgfiles.begin()+10, imgfiles.end());
				nyuman.LoadGTMasks(imgfiles, objmasks);

				// positive sample: object segments
				Mat possamps, negsamps;
				for(size_t i=0; i<imgfiles.size(); i++)
				{
					Mat cimg = imread(imgfiles[i].filepath);
					Mat dmap;
					nyuman.LoadDepthData(dmapfiles[i].filepath, dmap);

					const vector<Mat> masks = objmasks[imgfiles[i].filename];
					for (size_t k=0; k<masks.size(); k++)
					{
						SuperPixel cursegment;
						cursegment.mask = masks[k];
						sprintf_s(str, "%d_posseg_%d.jpg", i, k);
						//imwrite(temp_dir + string(str), cursegment.mask*255);
						Mat curposfeat;
						ComputeSegmentRankFeature(cimg, dmap, cursegment, curposfeat);
						possamps.push_back(curposfeat);
					}

					// negative samples: random segments don't overlap with objects
					for (size_t k=0; k<seg_ths.size(); k++)
					{
						imgsegmentor.m_dThresholdK = seg_ths[k];
						imgsegmentor.DoSegmentation(cimg);
						vector<SuperPixel>& tsps = imgsegmentor.superPixels;
						// randomly select 3x samples for every segment level
						int sumnum = 0;
						while(sumnum < masks.size())
						{
							int sel_id = rand() % imgsegmentor.superPixels.size();
							bool isvalid = true;
							for (size_t j=0; j<masks.size(); j++)
							{
								Mat intersectMask = masks[j] & tsps[sel_id].mask;
								if( countNonZero(intersectMask) / countNonZero(masks[j]) > 0.3 )
								{
									isvalid = false;
									break;
								}
							}
							// pass all tests
							sprintf_s(str, "%d_negseg_%d.jpg", i, sumnum);
							//imwrite(temp_dir + string(str), tsps[sel_id].mask*255);
							Mat curnegfeat;
							ComputeSegmentRankFeature(cimg, dmap, tsps[sel_id], curnegfeat);
							negsamps.push_back(curnegfeat);
							sumnum++;
						}
					}

					cout<<"Finished "<<i<<"/"<<imgfiles.size()<<" image"<<endl;
				}

				// train svm
				CvSVM model;
				Mat responses(1, possamps.rows+negsamps.rows, CV_32S);
				for(int r=0; r<possamps.rows; r++) responses.at<int>(r) = 1;
				for(int r=possamps.rows; r<responses.cols; r++) responses.at<int>(r) = -1;
				Mat allsamps;
				allsamps.push_back(possamps);
				allsamps.push_back(negsamps);

				SVMParams params;	
				model.train_auto(allsamps, responses, Mat(), Mat(), params);

				// save
				model.save("svm.model");

				// training performance
				float pos_corr = 0;
				for (int r=0; r<possamps.rows; r++)
				{
					float res = model.predict(possamps.row(r));
					if(res > 0)
						pos_corr++;
				}
				float neg_corr = 0;
				for (int r=0; r<negsamps.rows; r++)
				{
					float res = model.predict(negsamps.row(r));
					if(res < 0)
						neg_corr++;
				}
				cout<<"Pos accu: "<<pos_corr / possamps.rows<<endl;
				cout<<"Neg accu: "<<neg_corr / negsamps.rows<<endl;
				cout<<"total accu: "<<(pos_corr+neg_corr) / (possamps.rows + negsamps.rows)<<endl;

				return true;
			}

			bool ObjectRanker::LearnObjectWindowPredictor()
			{
				string temp_dir = "E:\\Results\\objectness\\";	// save intermediate results
				char str[50];

				ImageSegmentor imgsegmentor;
				vector<float> seg_ths(3);
				seg_ths[0] = 50;
				seg_ths[1] = 100;
				seg_ths[2] = 200;

				// generate training samples from nyudata
				NYUDepth2DataMan nyuman;
				FileInfos imgfiles, dmapfiles;
				nyuman.GetImageList(imgfiles);
				nyuman.GetDepthmapList(dmapfiles);
				map<string, vector<Mat>> objmasks;
				imgfiles.erase(imgfiles.begin()+10, imgfiles.end());
				nyuman.LoadGTMasks(imgfiles, objmasks);

				// positive sample: object segments
				Mat possamps, negsamps;
				for(size_t i=0; i<imgfiles.size(); i++)
				{
					Mat cimg = imread(imgfiles[i].filepath);
					Mat dmap;
					nyuman.LoadDepthData(dmapfiles[i].filepath, dmap);

					const vector<Mat> masks = objmasks[imgfiles[i].filename];
					vector<ImgWin> poswins;
					for (size_t k=0; k<masks.size(); k++)
					{
						SuperPixel cursegment;
						cursegment.mask = masks[k];
						segprocessor.ExtractBasicSegmentFeatures(cursegment, cimg, dmap);
						sprintf_s(str, "%d_posseg_%d.jpg", i, k);
						//imwrite(temp_dir + string(str), cursegment.mask*255);
						poswins.push_back(ImgWin(cursegment.box.x, cursegment.box.y, cursegment.box.width, cursegment.box.height));
						vector<ImgWin> wins;
						wins.push_back(poswins[poswins.size()-1]);
						vector<Mat> feats;
						ComputeWindowRankFeatures(cimg, dmap, wins, feats);
						possamps.push_back(feats[0]);
					}

					// negative samples: random window don't overlap with object windows
					int negnum = 0;
					vector<ImgWin> negwins;
					while(negnum < masks.size())
					{
						// generate random window
						ImgWin selwin;
						selwin.x = rand() % cimg.cols;
						selwin.y = rand() % cimg.rows;
						selwin.width = rand() % (cimg.cols-selwin.x-1);
						selwin.height = rand() % (cimg.rows-selwin.y-1);

						// test against all object windows
						bool isvalid = true;
						for(size_t k=0; k<poswins.size(); k++)
						{
							Rect intersectWin = selwin & poswins[k];
							if(intersectWin.area()*1.0f / poswins[k].area() > 0.3f)
							{ isvalid = false; break; }
						}
						// pass all tests
						if(isvalid)
						{
							sprintf_s(str, "%d_negseg_%d.jpg", i, negnum);
							//imwrite(temp_dir + string(str), tsps[sel_id].mask*255);
							negwins.push_back(selwin);
							vector<ImgWin> wins;
							wins.push_back(selwin);
							vector<Mat> feats;
							ComputeWindowRankFeatures(cimg, dmap, wins, feats);
							negsamps.push_back(feats[0]);
							negnum++;
						}
					}

					cout<<"Finished "<<i<<"/"<<imgfiles.size()<<" image"<<endl;
				}

				// train svm
				CvSVM model;
				Mat responses(1, possamps.rows+negsamps.rows, CV_32S);
				for(int r=0; r<possamps.rows; r++) responses.at<int>(r) = 1;
				for(int r=possamps.rows; r<responses.cols; r++) responses.at<int>(r) = -1;
				Mat allsamps;
				allsamps.push_back(possamps);
				allsamps.push_back(negsamps);

				SVMParams params;	
				model.train_auto(allsamps, responses, Mat(), Mat(), params);

				// save
				model.save("svm.model");

				// training performance
				float pos_corr = 0;
				for (int r=0; r<possamps.rows; r++)
				{
					float res = model.predict(possamps.row(r));
					if(res > 0)
						pos_corr++;
				}
				float neg_corr = 0;
				for (int r=0; r<negsamps.rows; r++)
				{
					float res = model.predict(negsamps.row(r));
					if(res < 0)
						neg_corr++;
				}
				cout<<"Pos accu: "<<pos_corr / possamps.rows<<endl;
				cout<<"Neg accu: "<<neg_corr / negsamps.rows<<endl;
				cout<<"total accu: "<<(pos_corr+neg_corr) / (possamps.rows + negsamps.rows)<<endl;

				return true;

			}
		}
	}
	
}



