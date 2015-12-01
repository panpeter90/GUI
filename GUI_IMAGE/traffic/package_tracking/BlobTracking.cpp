#include "stdafx.h"
#include "BlobTracking.h"
#include "cvblob/cvblob.h"
#include <iostream>
#include <stdio.h>
//#include <opencv2/nonfree/nonfree.hpp>
#include <fstream>
#include <cstring>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#define MARGIN 70

BlobTracking::BlobTracking() : firstTime(true), minArea(500), maxArea(20000), debugTrack(false), debugBlob(false), showBlobMask(false), showOutput(true)
{
  std::cout << "BlobTracking()" << std::endl;
}

BlobTracking::~BlobTracking()
{
  std::cout << "~BlobTracking()" << std::endl;
}

const cvb::CvTracks BlobTracking::getTracks()
{
  return tracks;
}

void BlobTracking::process(const cv::Mat &img_input, const cv::Mat &img_mask, cv::Mat &img_output,  bool &hasResult, cv::Mat &imagepredict, bool isblob )
{
	
  if(img_input.empty() || img_mask.empty())
    return;
  loadConfig();
  hasResult = false;
  //if(firstTime)
  //  saveConfig();
  cv::Mat img_final;
  IplImage* frame = new IplImage(img_input);
  cv::Mat img_recognize;
  img_input.copyTo(img_recognize);
  IplImage* example = new IplImage(img_recognize);
  line_pos = frame->width/2 + MARGIN;
  frameHeight = frame->height;
  //cvConvertScale(frame, frame, 1, 0);
  IplImage* examplemask = new IplImage(img_mask);
  IplImage* segmentated = new IplImage(img_mask);
  //
  //IplConvKernel* morphKernel = cvCreateStructuringElementEx(5, 5, 1, 1, CV_SHAPE_RECT, NULL);
  //cvMorphologyEx(segmentated, segmentated, NULL, morphKernel, CV_MOP_OPEN, 1);

  //if(showBlobMask)
    //cvShowImage("Blob Mask", segmentated);

  IplImage* labelImg = cvCreateImage(cvGetSize(frame), IPL_DEPTH_LABEL, 1);

  cvb::CvBlobs blobs;

  if(isblob) {
	  unsigned int result = cvb::cvLabel(segmentated, labelImg, blobs);
  
	  //cvb::cvFilterByArea(blobs, 1500, 20000);

	  cvb::cvFilterByArea(blobs, minArea, maxArea);
  
	  //cvb::cvRenderBlobs(labelImg, blobs, frame, frame, CV_BLOB_RENDER_BOUNDING_BOX);
	  if(debugBlob)
		cvb::cvRenderBlobs(labelImg, blobs, frame, frame, CV_BLOB_RENDER_BOUNDING_BOX|CV_BLOB_RENDER_CENTROID|CV_BLOB_RENDER_ANGLE|CV_BLOB_RENDER_TO_STD);
	  else
	  cvb::cvRenderBlobs(labelImg, blobs, frame, frame, CV_BLOB_RENDER_BOUNDING_BOX|CV_BLOB_RENDER_CENTROID);

	  //cvb::cvUpdateTracks(blobs, tracks, 200.,5);
  
	  //if(debugTrack)
		//cvb::cvRenderTracks(tracks, frame, frame, CV_TRACK_RENDER_ID|CV_TRACK_RENDER_BOUNDING_BOX|CV_TRACK_RENDER_TO_STD);
	 // else
		//cvb::cvRenderTracks(tracks, frame, frame, CV_TRACK_RENDER_ID|CV_TRACK_RENDER_BOUNDING_BOX);
  
	  //std::map<CvID, CvTrack *> CvTracks
	  for (cvb::CvBlobs::iterator it = blobs.begin(); it!=blobs.end(); ++it) {
		  std::cout << (*it).second->centroid.x << "," << (*it).second->centroid.y  << std::endl;
		  //if((*it).second->centroid.x <= line_pos && (*it).second->centroid.x > line_pos - MARGIN){
			if((*it).second->centroid.x <= line_pos){
			  cvSetImageROI(example, cvRect((*it).second->minx, (*it).second->miny, (*it).second->maxx-(*it).second->minx, (*it).second->maxy-(*it).second->miny));
			  //cvShowImage("cvSetImageROI", example);
			  cvSetImageROI(examplemask, cvRect((*it).second->minx, (*it).second->miny, (*it).second->maxx-(*it).second->minx, (*it).second->maxy-(*it).second->miny));
			  //cvShowImage("cvSetImageROIMask", examplemask);
			  cv::Mat img_final_save(example);
			  //cv::Mat img_final_mask(examplemask);
			  //cv::Mat img_final_recognize;
			  //img_final.copyTo(img_final_recognize,img_final_mask);
			  img_recognize.copyTo(img_final, img_mask);
			  //cv::imshow("img_final_mask", img_final_mask);
			  //cv::imshow("img_final", img_final);
			  hasResult = true;
			 // img_final.copyTo(imagepredict);
			   img_final_save.copyTo(imagepredict);
			  
			  static int number_id = 0;
			  std::stringstream convert;
			  convert << number_id;
			  number_id++;
			  //temp_truckNumber = convert.str().c_str();
			  cv::imwrite("D:/video/VehicleCapture/" + convert.str()+ ".jpg" ,imagepredict);
			  //cv::imwrite("D:/video/VehicleCapture/" + convert.str()+ ".jpg" ,img_final_save);
			  //system("pause");

		  }
	  } //for
  } // if(blob)
  //if(showOutput)
  //cvShowImage("Blob Tracking", frame);
  cv::Mat img_result(frame);
  img_result.copyTo(img_output);
  cv::line(img_output, cv::Point(line_pos, 0), cvPoint(line_pos, frameHeight), cv::Scalar(0,255,0), 1);
  //cvReleaseImage(&frame);
  //cvReleaseImage(&segmentated);
  cvReleaseImage(&labelImg);
  delete frame;
  delete segmentated;
  delete example;
  delete examplemask;
  cvReleaseBlobs(blobs);
  //cvReleaseStructuringElement(&morphKernel);
  firstTime = false;
}

void BlobTracking::saveConfig()
{
  CvFileStorage* fs = cvOpenFileStorage("config/BlobTracking.xml", 0, CV_STORAGE_WRITE);

  cvWriteInt(fs, "minArea", minArea);
  cvWriteInt(fs, "maxArea", maxArea);
  
  cvWriteInt(fs, "debugTrack", debugTrack);
  cvWriteInt(fs, "debugBlob", debugBlob);
  cvWriteInt(fs, "showBlobMask", showBlobMask);
  cvWriteInt(fs, "showOutput", showOutput);

  cvReleaseFileStorage(&fs);
}

void BlobTracking::loadConfig()
{
  CvFileStorage* fs = cvOpenFileStorage("config/BlobTracking.xml", 0, CV_STORAGE_READ);
  
  minArea = cvReadIntByName(fs, 0, "minArea", 500);
  maxArea = cvReadIntByName(fs, 0, "maxArea", 20000);

  debugTrack = cvReadIntByName(fs, 0, "debugTrack", false);
  debugBlob = cvReadIntByName(fs, 0, "debugBlob", false);
  showBlobMask = cvReadIntByName(fs, 0, "showBlobMask", false);
  showOutput = cvReadIntByName(fs, 0, "showOutput", true);

  cvReleaseFileStorage(&fs);
}