#include "stdafx.h"
#include "ImageDetectorSlidingWindow.h"

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  <ImageDetectorSlidingWindow>  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
ImageDetectorSlidingWindow::ImageDetectorSlidingWindow(Ptr<ImageClassifier> imageClassifier) : ImageDetector(imageClassifier) {}
ImageDetectorSlidingWindow::~ImageDetectorSlidingWindow() {}

extern float UpdateLabelFlag;
extern int carNumber;
extern int busNumber;
extern int containerNumber;
extern int truckNumber;
extern int vanNumber;

void ImageDetectorSlidingWindow::detectTargets(Mat& image, vector<Rect>& targetsBoundingRectanglesOut, Mat& votingMaskOut, Mat& scaledVotingMaskOut, bool showTargetBoundingRectangles, bool showImageKeyPoints, size_t* numberOfWindowsOut) {
	//cout << "-> Detecting targets..." << endl;
	PerformanceTimer performanceTimer;
	//performanceTimer.start();
	float prediction = getImageClassifier()->analyzeImage(image);
	//cout << "-> Detected " << " targets in " << performanceTimer.getElapsedTimeFormated()  << endl;
	UpdateLabelFlag = prediction;
	if(UpdateLabelFlag == (float)1){
		carNumber++;
	}else if(UpdateLabelFlag == (float)2){
		busNumber++;
	}else if(UpdateLabelFlag == (float)3){
		containerNumber++;
	}else if(UpdateLabelFlag == (float)4){
		truckNumber++;
	}else if(UpdateLabelFlag == (float)5){
		vanNumber++;
	}
	//cout << "UpdateLabelFlag" << UpdateLabelFlag << endl;
	//cout << "car 1- truck 2 - bus 3" << endl;
	cout << "Result label is " << prediction << endl;
	//cout << "-> Detected " << " targets in " << performanceTimer.getElapsedTimeFormated()  << endl;
}

/*void ImageDetectorSlidingWindow::detectTargets1(Mat& image, vector<Rect>& targetsBoundingRectanglesOut, Mat& votingMaskOut, Mat& scaledVotingMaskOut, bool showTargetBoundingRectangles, bool showImageKeyPoints, size_t* numberOfWindowsOut) {
	cout << "    -> Detecting targets..." << endl;
	PerformanceTimer performanceTimer;
	performanceTimer.start();

	float bestMatch = 0;		
	votingMaskOut = Mat::zeros(image.rows, image.cols, CV_16UC1);	
	
	size_t numberOfWindows = 0;
	
	for (double boxSizePercentage = SLIDING_WINDOW_BOX_GROWING_PERCENTAGE_START; boxSizePercentage <= 1.0; boxSizePercentage += SLIDING_WINDOW_BOX_GROWING_PERCENTAGE_INCREMENT) {
		int windowSizeX = (int)(boxSizePercentage * image.cols);
		int windowSizeY = (int)(boxSizePercentage * image.rows);
		
		int windowMovingXIncrement = (int)(windowSizeX * SLIDING_WINDOW_MOVING_PERCENTAGE_INCREMENT);
		int windowMovingYIncrement = (int)(windowSizeY * SLIDING_WINDOW_MOVING_PERCENTAGE_INCREMENT);

		for (int yPosition = 0; yPosition <= image.rows; yPosition += windowMovingYIncrement) {
			if (yPosition + windowSizeY > image.rows) { break; }
			for (int xPosition = 0; xPosition < image.cols; xPosition += windowMovingXIncrement) {
				if (xPosition + windowSizeX > image.cols) { break; }

				Mat window = image(Rect(xPosition, yPosition, windowSizeX, windowSizeY));
				
				if (window.cols > 6 && window.rows > 6) {
					float prediction = getImageClassifier()->analyzeImage(window);
					++numberOfWindows;
					//cout << prediction << " ";
					if (prediction > 0.75) {
						#pragma omp parallel for
						for (int yWindow = 0; yWindow < window.rows; ++yWindow) {
							for (int xWindow = 0; xWindow < window.cols; ++xWindow) {							
								#pragma omp atomic
								++votingMaskOut.at<unsigned short>(yPosition + yWindow, xPosition + xWindow);
							}
						}
					}
				}
			}
		}
	}
		

	if (showImageKeyPoints) {
		getImageClassifier()->analyzeImage(image, showImageKeyPoints);		
	}			
	
	Mat detectionMask;
	//cv::threshold(votingMask, detectionMask, numberOfWindows * DETECTION_MASK_THRESHOLD_NUMBER_WINDOWS_RATIO, (std::numeric_limits<unsigned char>::max)(), CV_THRESH_BINARY);
	cv::inRange(votingMaskOut, Scalar(numberOfWindows * DETECTION_MASK_THRESHOLD_NUMBER_WINDOWS_RATIO), Scalar((std::numeric_limits<unsigned char>::max)()), detectionMask);

	ImageUtils::findMaskBoundingRectangles(detectionMask, targetsBoundingRectanglesOut);

	if (showTargetBoundingRectangles) {
		for (size_t i = 0; i < targetsBoundingRectanglesOut.size(); i++) {
			cv::rectangle(image, targetsBoundingRectanglesOut[i], TARGET_DETECTIONS_RECTANGLE_COLOR, 2);
		}
	}

	cout << "    -> Detected " << targetsBoundingRectanglesOut.size() << " targets in " << performanceTimer.getElapsedTimeFormated() << " with " << numberOfWindows << " ROIs" << endl;

	if (numberOfWindowsOut != NULL) {
		*numberOfWindowsOut = numberOfWindows;
	}


	//double minVote;
	//double maxVote;
	//cv::minMaxLoc(votingMask, &minVote, &maxVote);
	//double convertionScale = (double)maxVote / (double)(std::numeric_limits<unsigned char>::max)();

	double convertionScale = (double)(std::numeric_limits<unsigned char>::max)() / (double)(numberOfWindows * GRAYSCALE_CONVERTION_MAX_NUMBER_WINDOWS_RATIO);
	
	votingMaskOut.convertTo(scaledVotingMaskOut, CV_8UC1, convertionScale);
}*/
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  <ImageDetectorSlidingWindow>  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
