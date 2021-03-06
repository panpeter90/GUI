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
#pragma once

#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "Config.h"

class ViCapture
{
private:
  CvCapture* capture;
  IplImage* frame;
  int key;
  int64 start_time;
  int64 delta_time;
  double freq;
  double fps;
  long frameNumber;
  long stopAt;
  bool useVideo;
  std::string videoFileName;
  int input_resize_percent;
  bool showOutput;
  bool enableFlip;

public:
  ViCapture();
  ~ViCapture();
  void setVideo(std::string filename);
  void start();
  void setUpVideo();
  void saveConfig();
  void loadConfig();
};

