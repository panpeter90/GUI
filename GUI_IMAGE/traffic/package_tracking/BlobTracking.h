#pragma once

#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "cvblob/cvblob.h"

class BlobTracking
{
private:
  bool firstTime;
  int minArea;
  int maxArea;
  int line_pos;
  int frameHeight;
  bool debugTrack;
  bool debugBlob;
  bool showBlobMask;
  bool showOutput;

  cvb::CvTracks tracks;
  void saveConfig();
  void loadConfig();

public:
  BlobTracking();
  ~BlobTracking();

  void process(const cv::Mat &img_input, const cv::Mat &img_mask, cv::Mat &img_output, bool &hasResult,cv::Mat &img_predict,bool isblob);
  const cvb::CvTracks getTracks();
};

