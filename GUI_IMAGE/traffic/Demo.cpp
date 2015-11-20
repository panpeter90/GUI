#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "package_bgs/PBAS/PixelBasedAdaptiveSegmenter.h"
#include "package_bgs/FrameDifferenceBGS.h"
#include "package_tracking/BlobTracking.h"
#include "package_analysis/VehicleCouting.h"
#include "VideoCapture/VideoCapture.h"
#include <ctime>
/*int main(int argc, char **argv)
{
  std::cout << "Using OpenCV " << CV_MAJOR_VERSION << "." << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << std::endl;
  int resize_factor = 100;
  CvCapture *capture = 0;
  capture = cvCaptureFromAVI("dataset/video.avi");//D:/video/9.MP4
  if(!capture){
    std::cerr << "Cannot open video!" << std::endl;
    return 1;
  }
  IplImage *frame_aux = cvQueryFrame(capture);
  IplImage *frame = cvCreateImage(cvSize((int)((frame_aux->width*resize_factor)/100) , (int)((frame_aux->height*resize_factor)/100)), frame_aux->depth, frame_aux->nChannels);
  //cvResize(frame_aux, frame);
  
  // Background Subtraction Algorithm 
  IBGS *bgs;
  //bgs = new PixelBasedAdaptiveSegmenter;
  bgs = new FrameDifferenceBGS ;
 //Blob Tracking Algorithm 
  cv::Mat img_blob;
  BlobTracking* blobTracking;
  blobTracking = new BlobTracking;

  //Vehicle Counting Algorithm //
  VehicleCouting* vehicleCouting;
  vehicleCouting = new VehicleCouting;

  std::cout << "Press 'q' to quit..." << std::endl;
  int key = 0;
  IplImage *frameTemp;
  while(key != 'q')
  {
    frameTemp = cvQueryFrame(capture);
	 cvResize(frameTemp,frame );
    if(!frame) break;

    cv::Mat img_input(frame);
    cv::imshow("Input", img_input);

    // bgs->process(...) internally process and show the foreground mask image
    cv::Mat img_mask;
	cv::Mat img_model_bg;
	const clock_t begin_time = clock();
		bgs->process(img_input, img_mask,img_model_bg);
    std::cout << "Time spent bgs:" <<float( clock () - begin_time ) /  CLOCKS_PER_SEC << "\n" << std::endl ;
    if(!img_mask.empty())
    {
      // Perform blob tracking
		const clock_t begin_time = clock();
       blobTracking->process(img_input, img_mask, img_blob);
	    cv::imshow("img_blob", img_blob);
      // Perform vehicle counting
    //vehicleCouting->setInput(img_blob);
      //vehicleCouting->setTracks(blobTracking->getTracks());
      //vehicleCouting->process();
		std::cout << "Time spent blobTracking:" << float( clock () - begin_time ) /  CLOCKS_PER_SEC << "\n" << std::endl ;
    }

    key = cvWaitKey(150);
  }
  system("pause");
  
  delete vehicleCouting;
  delete blobTracking;
  delete bgs;
  
  cvDestroyAllWindows();
  cvReleaseCapture(&capture);
  
  return 0;
} */
int main(int argc, char **argv){

	VideoCapture* videoCapture;
	videoCapture = new VideoCapture;
	videoCapture->setVideo("D:/video/100ANV01/9.MP4");
	videoCapture->setUpVideo();
	videoCapture->start();

	system("pause"); 
};