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
#include "VideoCapture.h"



ViCapture::ViCapture() : key(0), start_time(0), delta_time(0), freq(0), fps(0), frameNumber(0), stopAt(0),
useVideo(false), input_resize_percent(100), showOutput(true), enableFlip(false)
{
  std::cout << "ViCapture()" << std::endl;
}

ViCapture::~ViCapture()
{
  std::cout << "~ViCapture()" << std::endl;
}


