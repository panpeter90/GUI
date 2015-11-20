/*
This file is part of BGSLibrary.

BGSLibrary is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

BGSLibrary is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BGSLibrary.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "stdafx.h"
#include "../../GUI_IMAGEDlg.h"
#include "VideoCapture.h"
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "../package_bgs/FrameDifferenceBGS.h"
#include "../package_tracking/BlobTracking.h"
#include "../VideoCapture/VideoCapture.h"
#include <ctime>


namespace VC_ROI
{
  IplImage* img_input1 = 0;
  IplImage* img_input2 = 0;
  int roi_x0 = 0;
  int roi_y0 = 0;
  int roi_x1 = 0;
  int roi_y1 = 0;
  int numOfRec = 0;
  int startDraw = 0;
  bool roi_defined = false;
  bool use_roi = true;
  bool disable_event = false;

  void reset(void)
  {
    disable_event = false;
    startDraw = false;
  }

  void VideoCapture_on_mouse(int evt, int x, int y, int flag, void* param)
  {
    if(use_roi == false || disable_event == true)
      return;
    
    if(evt == CV_EVENT_LBUTTONDOWN)
    {
      if(!startDraw)
      {
        roi_x0 = x;
        roi_y0 = y;
        startDraw = 1;
      }
      else
      {
        roi_x1 = x;
        roi_y1 = y;
        startDraw = 0;
        roi_defined = true;
        disable_event = true;
      }
    }

    if(evt == CV_EVENT_MOUSEMOVE && startDraw)
    {
      //redraw ROI selection
      img_input2 = cvCloneImage(img_input1);
      cvRectangle(img_input2, cvPoint(roi_x0,roi_y0), cvPoint(x,y), CV_RGB(255,0,0), 1);
      cvShowImage("Input", img_input2);
      cvReleaseImage(&img_input2);
      //startDraw = false;
      //disable_event = true;
    }
  }
}

ViCapture::ViCapture() : key(0), start_time(0), delta_time(0), freq(0), fps(0), frameNumber(0), stopAt(0),
useVideo(false), input_resize_percent(100), showOutput(true), enableFlip(false)
{
  std::cout << "ViCapture()" << std::endl;
}

ViCapture::~ViCapture()
{
  std::cout << "~ViCapture()" << std::endl;
}


void ViCapture::setVideo(std::string filename)
{
  useVideo = true;
  videoFileName = filename;
}
void ViCapture::setUpVideo()
{
	capture = cvCaptureFromFile(videoFileName.c_str());

	if(!capture)
		std::cerr << "Cannot open video file "<< videoFileName << std::endl;
}

void ViCapture::start()
{

  loadConfig();

  if(!capture)  std::cerr << "Capture error..." << std::endl;
  
  int input_fps = cvGetCaptureProperty(capture,CV_CAP_PROP_FPS);
  std::cout << "input->fps:" << input_fps << std::endl;
  IBGS *bgs;
  //bgs = new PixelBasedAdaptiveSegmenter;
  bgs = new FrameDifferenceBGS ;
  IplImage* frame1 = cvQueryFrame(capture);
  frame = cvCreateImage(cvSize((int)((frame1->width*input_resize_percent)/100) , (int)((frame1->height*input_resize_percent)/100)), frame1->depth, frame1->nChannels);
  //cvCreateImage(cvSize(frame1->width/input_resize_factor, frame1->height/input_resize_factor), frame1->depth, frame1->nChannels);
  std::cout << "input->resize_percent:" << input_resize_percent << std::endl;
  std::cout << "input->width:" << frame->width << std::endl;
  std::cout << "input->height:" << frame->height << std::endl;
  //Blob Tracking Algorithm 
  cv::Mat img_blob;
  BlobTracking* blobTracking;
  blobTracking = new BlobTracking;
  //end
  cv::Mat img_mask;
  cv::Mat img_model_bg;
  double loopDelay = 33.333;
  if(input_fps > 0)
    loopDelay = (1./input_fps)*1000.;
  std::cout << "loopDelay:" << loopDelay << std::endl;

  bool firstTime = true;
  do
  {
    frameNumber++;

    frame1 = cvQueryFrame(capture);
    if(!frame1) break;

    cvResize(frame1, frame);

    if(enableFlip)
      cvFlip(frame, frame, 0);
    
    if(VC_ROI::use_roi == true && VC_ROI::roi_defined == false && firstTime == true)
    {
      VC_ROI::reset();

      do
      {
        cv::Mat img_input(frame);
        
        if(showOutput)
        {
          //cv::imshow("Input", img_input);
		  //ImageDisplay(img_input);
          std::cout << "Set ROI (press ESC to skip)" << std::endl;
          VC_ROI::img_input1 = new IplImage(img_input);
          cvSetMouseCallback("Input", VC_ROI::VideoCapture_on_mouse, NULL);
          key = cvWaitKey(0);
          delete VC_ROI::img_input1;
        }
        else
          key = KEY_ESC;

        if(key == KEY_ESC)
        {
          std::cout << "ROI disabled" << std::endl;
          VC_ROI::reset();
          VC_ROI::use_roi = false;
          break;
        }

        if(VC_ROI::roi_defined)
        {
          std::cout << "ROI defined (" << VC_ROI::roi_x0 << "," << VC_ROI::roi_y0 << "," << VC_ROI::roi_x1 << "," << VC_ROI::roi_y1 << ")" << std::endl;
          break;
        }
        else
          std::cout << "ROI undefined" << std::endl;

      }while(1);
    }

    if(VC_ROI::use_roi == true && VC_ROI::roi_defined == true)
    {
      CvRect rect = cvRect(VC_ROI::roi_x0, VC_ROI::roi_y0, VC_ROI::roi_x1 - VC_ROI::roi_x0, VC_ROI::roi_y1 - VC_ROI::roi_y0);
      cvSetImageROI(frame, rect);
    }
    cv::Mat img_input(frame);
    
    if(showOutput)
      cv::imshow("Input", img_input);

    if(firstTime)
      saveConfig();

	const clock_t begin_time1 = clock();
	bgs->process(img_input, img_mask,img_model_bg);
	std::cout << "Time spent bgs:" <<float( clock () - begin_time1 )/CLOCKS_PER_SEC << "\n" << std::endl ;
	const clock_t begin_time = clock();
	if(!img_mask.empty())
	{
		blobTracking->process(img_input, img_mask, img_blob);
		//for (cvb::CvBlobs::iterator it=blobs.begin(); it!=blobs.end(); ++it) {
			//std::cout << (*it).second->centroid.x << "," << (*it).second->centroid.y  << std::endl;
		//}
		cv::imshow("img_blob", img_blob);
		// Perform vehicle counting
		//vehicleCouting->setInput(img_blob);
		//vehicleCouting->setTracks(blobTracking->getTracks());
		//vehicleCouting->process();
		std::cout << "Time spent blobTracking:" << float( clock () - begin_time ) /  CLOCKS_PER_SEC << "\n" << std::endl ;
	}
     //cv::imshow("img_mask", img_mask);
    //std::cout << "delta_time: " << delta_time << std::endl;

    cvResetImageROI(frame);

    key = cvWaitKey(loopDelay);
    //std::cout << "key: " << key << std::endl;

    if(key == KEY_SPACE)
      key = cvWaitKey(0);

    if(key == KEY_ESC)
      break;

    if(stopAt > 0 && stopAt == frameNumber)
      key = cvWaitKey(0);

    firstTime = false;

  }while(1);

  cvReleaseCapture(&capture);
}

void ViCapture::saveConfig()
{
  CvFileStorage* fs = cvOpenFileStorage("./config/VideoCapture.xml", 0, CV_STORAGE_WRITE);

  cvWriteInt(fs, "stopAt", stopAt);
  cvWriteInt(fs, "input_resize_percent", input_resize_percent);
  cvWriteInt(fs, "enableFlip", enableFlip);
  cvWriteInt(fs, "use_roi", VC_ROI::use_roi);
  cvWriteInt(fs, "roi_defined", VC_ROI::roi_defined);
  cvWriteInt(fs, "roi_x0", VC_ROI::roi_x0);
  cvWriteInt(fs, "roi_y0", VC_ROI::roi_y0);
  cvWriteInt(fs, "roi_x1", VC_ROI::roi_x1);
  cvWriteInt(fs, "roi_y1", VC_ROI::roi_y1);
  cvWriteInt(fs, "showOutput", showOutput);

  cvReleaseFileStorage(&fs);
}

void ViCapture::loadConfig()
{
  CvFileStorage* fs = cvOpenFileStorage("./config/VideoCapture.xml", 0, CV_STORAGE_READ);

  stopAt = cvReadIntByName(fs, 0, "stopAt", 0);
  input_resize_percent = cvReadIntByName(fs, 0, "input_resize_percent", 100);
  enableFlip = cvReadIntByName(fs, 0, "enableFlip", false);
  VC_ROI::use_roi = cvReadIntByName(fs, 0, "use_roi", true);
  VC_ROI::roi_defined = cvReadIntByName(fs, 0, "roi_defined", false);
  VC_ROI::roi_x0 = cvReadIntByName(fs, 0, "roi_x0", 0);
  VC_ROI::roi_y0 = cvReadIntByName(fs, 0, "roi_y0", 0);
  VC_ROI::roi_x1 = cvReadIntByName(fs, 0, "roi_x1", 0);
  VC_ROI::roi_y1 = cvReadIntByName(fs, 0, "roi_y1", 0);
  showOutput = cvReadIntByName(fs, 0, "showOutput", true);

  cvReleaseFileStorage(&fs);
}