
// GUI_IMAGEDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GUI_IMAGE.h"
#include "GUI_IMAGEDlg.h"
#include "traffic/CLI/CLI.h"
#include "afxdialogex.h"
#include <string>
#include <iostream>
#include <stdio.h>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "resource.h"
#include <opencv\cv.h>
#include <opencv\cvaux.h>
#include <opencv\cxcore.h>
#include <opencv\highgui.h>
#include <io.h>
#include <fcntl.h>
#include "opencv2/opencv.hpp"
//#include <opencv2/nonfree/nonfree.hpp>
#include <fstream>
#include <memory>
#include <functional>
#include <queue>
#include <map>
#include <cstring>
#include <stdlib.h>
#include <sys/types.h>
#include "dirent.h"
#include <sys/stat.h>
#include <stdint.h>
#include <windows.h>
#include <ctime>
#include <string>
//BGS
#include "traffic/package_bgs/FrameDifferenceBGS.h"
#include "traffic/package_tracking/BlobTracking.h"
#include "traffic/VideoCapture/VideoCapture.h"

#define MAX_PATH 512

#define MIN_KPS    3
#define VOCA_COLS  1000

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define SHOW_DEBUG_IMAGE
// CAboutDlg dialog used for App About
using namespace std;
using namespace cv;

/*Various color for detect shirt */

CString msg;
/*option setup global variables*/
int g_featureDetectorSelection = 1;
int g_descriptorExtractorSelection = 1;
int g_descriptorMatcherSelection = 2;
int g_bowTrainerSelection = 1;
int g_classifierSelection = 1;
CLI UserGui;
//number of vehicle
float UpdateLabelFlag = 0;
int ResourceFlag = 0;
int carNumber = 0;
int bikeNumber = 0;
int bicycleNumber = 0;
int craneNumber = 0;
int truckNumber = 0;
int busNumber = 0;
//end
const string kVocabularyFile( "vocabulary.xml" );
const string kBowImageDescriptorsDir( "bagOfWords" );
const string kSvmsDirs( "svms" );
ImageAnalysis imageAnalysis(NULL,NULL);
/*end of option setup*/
// some utility functions 
void MakeDir( const string& filepath );
void help( const char* progName );
void GetDirList( const string& directory, vector<string>* dirlist );
void GetFileList( const string& directory, vector<string>* filelist );
void ComputeBowImageDescriptors( const string& databaseDir, Mat& vocabulary,
	const vector<string>& categories,
	const Ptr<FeatureDetector>& detector,
	const Ptr<DescriptorExtractor>& extractor,
	Ptr<BOWImgDescriptorExtractor>& bowExtractor,
	const string& imageDescriptorsDir,
	map<string, Mat>* samples);
void TrainSvm( const map<string, Mat>& samples, const string& category, const CvSVMParams& svmParams, CvSVM* svm );
Mat BuildVocabulary( const string& databaseDir,
	const vector<string>& categories,
	const Ptr<FeatureDetector>& detector,
	const Ptr<DescriptorExtractor>& extractor,
	int wordCount);
void  opencv_llc_bow_Descriptor(Mat &image, Mat &vocabulary,  vector<KeyPoint> &key_points, Mat &llc_descriptor);
void  train();
void test();
//

enum                             {cBLACK=0,cWHITE, cGREY, cRED, cORANGE, cYELLOW, cGREEN, cAQUA, cBLUE, cPURPLE, cPINK,  NUM_COLOR_TYPES};
char* sCTypes[NUM_COLOR_TYPES] = {"Black", "White","Grey","Red","Orange","Yellow","Green","Aqua","Blue","Purple","Pink"};
uchar cCTHue[NUM_COLOR_TYPES] =    {0,       0,      0,     0,     20,      30,      55,    85,   115,    138,     161};
uchar cCTSat[NUM_COLOR_TYPES] =    {0,       0,      0,    255,   255,     255,     255,   255,   255,    255,     255};
uchar cCTVal[NUM_COLOR_TYPES] =    {0,      255,    120,   255,   255,     255,     255,   255,   255,    255,     255};
//param
class Params {
public:
	Params(): wordCount(VOCA_COLS){}
	int		wordCount;
	Ptr<FeatureDetector> featureDetector;
	Ptr<DescriptorExtractor> descriptorExtractor;
	Ptr<DescriptorMatcher> descriptorMatcher;
};

CvScalar text_color= {255,255,255,0};
/* Face Detection HaarCascade Classifier file */
const char* cascadeFileFace = "haarcascades\\haarcascade_frontalface_alt.xml";
CvHaarClassifierCascade* cascadeFace;
string file_name="";
string video_name="";
bool is_image = true;
bool is_exit = false;
bool is_sparse_coding = false;
Params params;
/***********************************************************************
Function:  Draw a rectangle around the given object (defaults to a red color)
Input: - Image
	   - CvRect
	   - Color
*****************************************************************************/
void drawRectangle(IplImage *img, CvRect face, CvScalar col)	{
	CvPoint p1, p2;
	p1.x = face.x;
	p1.y = face.y;
	p2.x = face.x + face.width;
	p2.y = face.y + face.height;
	cvRectangle(img, p1, p2, col, 2);
}
/***********************************************************************
Function: Returns a new image that is a cropped version of the original image. 
Input: Image and CvRect
Output: New image
************************************************************************/
IplImage* cropRectangle(IplImage *img, CvRect region)
{
	IplImage *imageTmp,*imageRGB;
	CvSize size;
	size.height = img->height;
	size.width = img-> width;
	if (img->depth != IPL_DEPTH_8U) {
		std::cerr << "ERROR: Unknown image depth of " << img->depth << " given in cropRectangle() instead of 8." << std::endl;
		exit(1);
	}
	// First create a new (color or greyscale) IPL Image and copy contents of img into it.
	imageTmp = cvCreateImage(size, IPL_DEPTH_8U, img->nChannels);
	cvCopy(img, imageTmp);
	cvSetImageROI(imageTmp, region);
	// Copy region of interest (i.e. face) into a new iplImage (imageRGB) and return it
	size.width = region.width;
	size.height = region.height;
	imageRGB = cvCreateImage(size, IPL_DEPTH_8U, img->nChannels);
	cvCopy(imageTmp, imageRGB);	// Copy just the region.

    cvReleaseImage( &imageTmp );
	return imageRGB;		
}
/**********************************************************************
Function : Detect face use haarcascades classifier
Input: pImage , haarcascades classifier
Output: List of rectangle for detect region
**********************************************************************/
vector<CvRect> findObjectsInImage(IplImage *origImg, CvHaarClassifierCascade* cascade, CvSize minFeatureSize = cvSize(20, 20))
{
	CvMemStorage* storage;
	vector<CvRect> detRects;
	storage = cvCreateMemStorage(0);
	cvClearMemStorage( storage );

	IplImage *detectImg = origImg;	/* Assume the input image is to be used.*/
	IplImage *greyImg = 0;
	/*if the image is color we change to grey image */
	if (origImg->nChannels > 1) {
		greyImg = cvCreateImage(cvSize(origImg->width, origImg->height), 8, 1 );
		cvCvtColor( origImg, greyImg, CV_BGR2GRAY );
		detectImg = greyImg;
	}
	/*Detect face in image */
	CvSeq* rects = cvHaarDetectObjects( detectImg, cascade, storage,
                                        1.1, 2, CV_HAAR_DO_CANNY_PRUNING,
                                        minFeatureSize );
	/*Get detected region*/	
	for(int i = 0; i < (rects ? rects->total : 0); i++ )
	{
        CvRect *r = (CvRect*)cvGetSeqElem( rects, i );
		detRects.push_back(*r);
    }
	/*cvReleaseHaarClassifierCascade( &cascade )*/
	if (greyImg)
		cvReleaseImage( &greyImg );
	cvReleaseMemStorage( &storage );
	return detRects;
}
/******************************************************************************
Function: Get color of a pixel
Input: HSV value
Output: Color type between 0 to NUM_COLOR_TYPES
******************************************************************************/
int getPixelColorType(int H, int S, int V)
{
	int color;
	if (V < 75)
		color = cBLACK;
	else if (V > 190 && S < 27)
		color = cWHITE;
	else if (S < 53 && V < 185)
		color = cGREY;
	else {	// Is a color
		if (H < 14)
			color = cRED;
		else if (H < 25)
			color = cORANGE;
		else if (H < 34)
			color = cYELLOW;
		else if (H < 73)
			color = cGREEN;
		else if (H < 102)
			color = cAQUA;
		else if (H < 127)
			color = cBLUE;
		else if (H < 149)
			color = cPURPLE;
		else if (H < 175)
			color = cPINK;
		else	// full circle 
			color = cRED;	// back to Red
	}
	return color;
}
void MakeDir( const string& filepath )
{
	char path[MAX_PATH];

	strncpy(path, filepath.c_str(),  MAX_PATH);

#ifdef _WIN32
	CreateDirectoryA(path,NULL);
#else
	CreateDirectoryA(path, 0755);
#endif
}

void ListDir( const string& directory, vector<string>* entries)
{
	char dir[MAX_PATH];
	string  str_dir = directory;

	strncpy(dir, str_dir.c_str(), MAX_PATH);

	DIR             *p_dir;
	struct dirent *p_dirent;

	p_dir = opendir(dir);

	while(p_dirent = readdir(p_dir))
	{
		string  str_fn = p_dirent->d_name;

		if (str_fn != "." && str_fn != "..")  entries->push_back(str_fn);
	}
}

void GetDirList( const string& directory, vector<string>* dirlist )
{
	ListDir( directory, dirlist);
}

void GetFileList( const string& directory, vector<string>* filelist )
{
	ListDir( directory, filelist);

}

static void ImageDisplay(cv::Mat src_); //MINHNT

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();



// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()
// CGUI_IMAGEDlg dialog
CGUI_IMAGEDlg::CGUI_IMAGEDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CGUI_IMAGEDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGUI_IMAGEDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, LoadImage);
	DDX_Control(pDX, IDC_COMBO2, ComboBox2);
	DDX_Control(pDX, IDC_COMBO1, ComboBox1);
	DDX_Control(pDX, IDC_COMBO3, ComboBox3);
	DDX_Control(pDX, IDC_COMBO4, ComboBox4);
	DDX_Control(pDX, IDC_COMBO5, ComboBox5);
	DDX_Control(pDX, IDC_COMBO6, ComboBox6);
	DDX_Control(pDX, IDC_PICTURE, Picture1);
	DDX_Control(pDX, IDC_CHECK1, CheckBox);
	DDX_Control(pDX, IDC_STATIC_CAR, StaticCar);
	DDX_Control(pDX, IDC_STATIC_BICYCLE, StaticBicycle);
}

BEGIN_MESSAGE_MAP(CGUI_IMAGEDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CGUI_IMAGEDlg::OnBnClickedButton1)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CGUI_IMAGEDlg::OnCbnSelchangeCombo2)
	ON_BN_CLICKED(IDOK, &CGUI_IMAGEDlg::OnBnClickedOk)
	ON_BN_CLICKED(ID_REC, &CGUI_IMAGEDlg::OnBnClickedRec)
	ON_BN_CLICKED(IDC_CHECK1, &CGUI_IMAGEDlg::OnBnClickedCheck1)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CGUI_IMAGEDlg::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CGUI_IMAGEDlg::OnCbnSelchangeCombo3)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CGUI_IMAGEDlg::OnCbnSelchangeCombo4)
	ON_CBN_SELCHANGE(IDC_COMBO5, &CGUI_IMAGEDlg::OnCbnSelchangeCombo5)
	ON_CBN_SELCHANGE(IDC_COMBO6, &CGUI_IMAGEDlg::OnCbnSelchangeCombo6)
	ON_BN_CLICKED(IDC_START, &CGUI_IMAGEDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CGUI_IMAGEDlg message handlers

BOOL CGUI_IMAGEDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// Add "About..." menu item to system menu.
	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	cascadeFace = (CvHaarClassifierCascade*)cvLoad(cascadeFileFace, 0, 0, 0 );
	namedWindow("DisplayImage",1);
	HWND hWnd =(HWND)cvGetWindowHandle("DisplayImage");
	HWND hParent=::GetParent(hWnd);
	::SetParent(hWnd,GetDlgItem(IDC_PICTURE)->m_hWnd);
	::ShowWindow(hParent,SW_HIDE);
	Mat display_init = cv::Mat(800, 1200, CV_8UC3, Scalar(150,150,150));
	imshow("DisplayImage",display_init);
	// TODO: Add extra initialization here
	//Combobox
	ComboBox1.SetCurSel(0);
	ComboBox3.SetCurSel(0);
	ComboBox4.SetCurSel(1);
	ComboBox5.SetCurSel(0);
	ComboBox6.SetCurSel(0);
	//end combobox
	ComboBox2.SetCurSel(0); //Image
	//number of vehicle
	CFont *m_pFont = new CFont();
	m_pFont->CreatePointFont(250, _T("Arial"));
	StaticCar.SetFont(m_pFont, TRUE);
	StaticBicycle.SetFont(m_pFont, TRUE);
	UpdateLabelAll();
	//end number vehicle
	
	if (!AllocConsole()){
		AfxMessageBox(_T("Failed to create the console!"));
	}
	*stdout = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), _O_WRONLY), _T("a"));
	*stderr = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE), _O_WRONLY), _T("a"));
	*stdin = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_INPUT_HANDLE), _O_WRONLY), _T("r"));
	printf("Init console window successfully\n");

	//init console window end
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGUI_IMAGEDlg::UpdateLabelOne(){
	if(UpdateLabelFlag == (float)1){
		UpdateLabelCar();
		cout << "UpdateLabelCar" << endl;
	}else if(UpdateLabelFlag == (float)3){
		UpdateLabelBicycle();
		cout << "UpdateLabelBicycle" << endl;
	}
}
void CGUI_IMAGEDlg::UpdateLabelAll(){
	UpdateLabelCar();
	UpdateLabelBicycle();
}

void CGUI_IMAGEDlg::UpdateLabelCar(){
	CString temp_carNumber;
	stringstream convert;
	convert << carNumber;
	temp_carNumber = convert.str().c_str();
	StaticCar.SetWindowText(temp_carNumber);
}
void CGUI_IMAGEDlg::UpdateLabelBicycle(){
	CString temp_bicycleNumber;
	stringstream convert;
	convert << bicycleNumber;
	temp_bicycleNumber = convert.str().c_str();
	StaticBicycle.SetWindowText(temp_bicycleNumber);
}


void CGUI_IMAGEDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGUI_IMAGEDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGUI_IMAGEDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CGUI_IMAGEDlg::OnBnClickedButton1() //load image
{

	if(ResourceFlag == 0) {
		CString Filter =_T("image file(*.bmp; *.jpg) |*.bmp;*.jpg|All Files (*.*)|*.*||");
		CFileDialog dlg(TRUE,_T("*.jpg"),NULL,OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY,Filter,NULL);
		dlg.m_ofn.lpstrTitle=_T("Load Image");
		if (dlg.DoModal()==IDOK)
		{
			CString CSfile_name = dlg.GetPathName();
			file_name=(CStringA(CSfile_name));
			Mat src = imread(file_name,1);
			Mat dest;
			//resize(src,dest,Size(320,240),0,0,1);
			//imshow("DisplayImage",src);
			ImageDisplay(src);
		}
	}else{
		CString Filter =_T("Video file(*.mp4; *.avi) |*.mp4;*.avi|All Files (*.*)|*.*||");
		CFileDialog dlg(TRUE,_T("*.avi"),NULL,OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY,Filter,NULL);
		dlg.m_ofn.lpstrTitle=_T("Load Video");
		if (dlg.DoModal()==IDOK)
		{
			CString CSfile_name = dlg.GetPathName();
			video_name=(CStringA(CSfile_name));
			VideoCapture cap(video_name); // open the default camera
			if(!cap.isOpened()){  // check if we succeeded
				cout << "Error is existed when open this video \n" <<endl;
			}
			Mat edges;
			for(;;)
			{
				Mat frame;
				if (!cap.read(frame))             
					break; // get a new frame from video
				imshow("DisplayImage",frame);
				if(waitKey(30)>0 || is_exit==true) {
					break;
				}
			}
			cout << "Close this video \n" << endl;
		}
	}
	// TODO: Add your control notification handler code here
}


void CGUI_IMAGEDlg::OnCbnSelchangeCombo2()
{
	// TODO: Add your control notification handler code here
	int choice = ComboBox2.GetCurSel();
	switch (choice){
		case 0 :
			ResourceFlag = 0; //Image
			break;
		case 1 :
			ResourceFlag = 1; //Video
			break;
		default:
			break;
	}
}

void ImageDisplay(cv::Mat src_)
{
	Mat disp;
	int width_, height_;
	if(!src_.empty())
	{
		if((src_.cols > 900) && (src_.cols > src_.rows))
		{
			width_ = 900;
			height_ = int(width_*src_.rows/src_.cols);
		}

		if(src_.rows > 600)
		{
			height_ = 600;
			width_ = int(height_*src_.cols/src_.rows);
		}
		else
		{
			width_ = src_.cols;
			height_ = src_.rows;
		}
		cv::resize(src_, disp, cv::Size(width_, height_));
		cv::imshow("DisplayImage", disp);
	}
}




void CGUI_IMAGEDlg::OnBnClickedOk() //exit button
{
	// TODO: Add your control notification handler code here
	if (!FreeConsole()){
		AfxMessageBox(_T("Could not free the console!"));
	}
	is_exit = true;
	CDialogEx::OnOK();
}

void CGUI_IMAGEDlg::OnBnClickedRec() //Recognize button
{
	/*if(is_sparse_coding){
		test();
	}else{
		imageAnalysis.processImage(file_name);
		UpdateLabelOne();
	}*/
	ViCapture* videoCapture;
	videoCapture = new ViCapture;
	videoCapture->setVideo("D:/video/100ANV01/9.MP4");
	videoCapture->setUpVideo();
	videoCapture->start();
	system("pause");
}

void CGUI_IMAGEDlg::OnBnClickedCheck1()
{
	
	// TODO: Add your control notification handler code here
	UINT nCheck = CheckBox.GetCheck();
    if (nCheck == BST_CHECKED)
    {
        is_sparse_coding = true;
    }
    else
    {
        is_sparse_coding = false;
    }

}



void CGUI_IMAGEDlg::OnCbnSelchangeCombo1() //FeatureDetector
{
	// TODO: Add your control notification handler code here
	int choice = ComboBox1.GetCurSel();
	g_featureDetectorSelection = choice + 1;
	//msg.Format(_T("%d"), g_featureDetectorSelection);
	//AfxMessageBox(msg);
}

void CGUI_IMAGEDlg::OnCbnSelchangeCombo3() //descriptorExtractor
{
	int choice = ComboBox3.GetCurSel();
	g_descriptorExtractorSelection = choice +1;

}

void CGUI_IMAGEDlg::OnCbnSelchangeCombo4() //descriptorMatcher
{
	int choice = ComboBox4.GetCurSel();
	g_descriptorMatcherSelection = choice +1;

}

void CGUI_IMAGEDlg::OnCbnSelchangeCombo5()  // bowTrainer
{
	int choice = ComboBox5.GetCurSel();
	g_bowTrainerSelection = choice + 1;
}

void CGUI_IMAGEDlg::OnCbnSelchangeCombo6() //classifier
{
	int choice = ComboBox6.GetCurSel();
	g_classifierSelection = choice +1;
}


void CLI::startInteractiveCLI() {
	int userOption = 1;
	string filename = "";
	int cameraDeviceNumber = 0;

	ConsoleInput::getInstance()->clearConsoleScreen();

	int screenWidth = 1920; // ConsoleInput::getInstance()->getIntCin("  >> Screen width (used to arrange windows): ", "  => Width >= 100 !!!\n", 100);
	int screenHeight = 1080; // ConsoleInput::getInstance()->getIntCin("  >> Screen height (used to arrange windows): ", "  => Width >= 100 !!!\n", 100);
	bool optionsOneWindow = false; // ConsoleInput::getInstance()->getYesNoCin("  >> Use only one window for options trackbars? (Y/N): ");
	bool classifierTrained = false;

	do {
		try {
			ConsoleInput::getInstance()->clearConsoleScreen();

			if (classifierTrained) {
				userOption = getUserOption();
				if (userOption == 1) {
					setupTraining();
				} else if (userOption == 2) {
					_imageDetector->evaluateDetector(TEST_IMGAGES_LIST);
				} else {
					if (userOption == 3 || userOption == 4) {
						filename = "";
						do {
							cout << "  >> Path to file: ";
							filename = ConsoleInput::getInstance()->getLineCin();

							if (filename == "") {
								cerr << "  => File path can't be empty!\n" << endl;
							}
						} while (filename == "");
					} else if (userOption == 5) {
						cameraDeviceNumber = ConsoleInput::getInstance()->getIntCin("  >> Insert the camera device number to use (default: 0): ", "  => Camera device number must be >= 0 !!!\n", 0);
					}

					ImageAnalysis imageAnalysis(_imagePreprocessor, _imageDetector);
					imageAnalysis.setScreenWidth(screenWidth);
					imageAnalysis.setScreenHeight(screenHeight);
					imageAnalysis.setOptionsOneWindow(optionsOneWindow);
					switch (userOption) {
					case 3: { if (!imageAnalysis.processImage(filename)) { cerr << "  => Failed to load image " << filename << "!" << endl; } break; }
					case 4: { if (!imageAnalysis.processVideo(filename)) { cerr << "  => Failed to load video " << filename << "!" << endl; } break; }
					case 5: { if (!imageAnalysis.processVideo(cameraDeviceNumber)) { cerr << "  => Failed to open camera " << cameraDeviceNumber << "!" << endl; } break; }
					default: break;
					}				
				}
			} else {
				setupTraining();
				classifierTrained = true;
			}

			if (userOption != 0) {
				cout << "\n\n" << endl;
				ConsoleInput::getInstance()->getUserInput();
			}
		} catch (...) {
			cerr << "\n\n\n!!!!! Caught unexpected exception !!!!!\n\n\n" << endl;
		}
	} while (userOption != 0);

	cout << "\n\n\n" << endl;
	cout << "\n\n" << endl;
	ConsoleInput::getInstance()->getUserInput();
}


int CLI::getUserOption() {
	cout << " ## Detect car from:\n";
	cout << "   1 - Train classifier\n";
	cout << "   2 - Evaluate detector\n";
	cout << "   3 - Test detector from image\n";
	cout << "   4 - Test detector from video\n";
	cout << "   5 - Test detector from camera\n";
	cout << "   0 - Exit\n";
	return ConsoleInput::getInstance()->getIntCin("\n >>> Option [0, 4]: ", "Select one of the options above!", 0, 6);
}


void CLI::setupTraining() {
	cout << "\n\n ## Training setup:\n" << endl;
	int featureDetectorSelection = g_featureDetectorSelection;
	int descriptorExtractorSelection = g_descriptorExtractorSelection;
	int descriptorMatcherSelection = g_descriptorMatcherSelection;
	int bowTrainerSelection = g_bowTrainerSelection;
	int classifierSelection = g_classifierSelection;

	Ptr<FeatureDetector> featureDetector;
	Ptr<DescriptorExtractor> descriptorExtractor;
	Ptr<DescriptorMatcher> descriptorMatcher;
	Ptr<BOWTrainer> bowTrainer;

	stringstream trainingConfigsTag;

	switch (featureDetectorSelection) {
	case 1: { featureDetector = new cv::SiftFeatureDetector();			trainingConfigsTag << "_SIFT-Detector"; break; }
	case 2: { featureDetector = new cv::SurfFeatureDetector(400);		trainingConfigsTag << "_SURF-Detector"; break; }
	case 3: { featureDetector = new cv::GoodFeaturesToTrackDetector();	trainingConfigsTag << "_GFTT-Detector"; break; }
	case 4: { featureDetector = new cv::FastFeatureDetector();			trainingConfigsTag << "_FAST-Detector"; break; }		
	case 5: { featureDetector = new cv::OrbFeatureDetector();			trainingConfigsTag << "_ORB-Detector";  break; }
	case 6: { featureDetector = new cv::BRISK();						trainingConfigsTag << "_BRISK-Detector"; break; }
	case 7: { featureDetector = new cv::StarFeatureDetector();			trainingConfigsTag << "_STAR-Detector"; break; }
	case 8: { featureDetector = new cv::MserFeatureDetector();			trainingConfigsTag << "_MSER-Detector"; break; }	
	default: break;
	}

	switch (descriptorExtractorSelection) {
	case 1: { descriptorExtractor = new cv::SiftDescriptorExtractor();	trainingConfigsTag << "_SIFT-Extractor"; break; }
	case 2: { descriptorExtractor = new cv::SurfDescriptorExtractor();	trainingConfigsTag << "_SURF-Extractor"; break; }
	case 3: { descriptorExtractor = new cv::FREAK();					trainingConfigsTag << "_FREAK-Extractor"; break; }
	case 4: { descriptorExtractor = new cv::BriefDescriptorExtractor();	trainingConfigsTag << "_BRIEF-Extractor"; break; }		
	case 5: { descriptorExtractor = new cv::OrbDescriptorExtractor();	trainingConfigsTag << "_ORB-Extractor";  break; }
	case 6: { descriptorExtractor = new cv::BRISK();					trainingConfigsTag << "_BRISK-Extractor";  break; }

	default: break;
	}

	bool binaryDescriptor;
	int bfNormType;
	Ptr<cv::flann::IndexParams> flannIndexParams/* = new cv::flann::AutotunedIndexParams()*/;
	if (descriptorExtractorSelection > 2) { // binary descriptors
		binaryDescriptor = true;
		bfNormType = cv::NORM_HAMMING;
		//flannIndexParams = new cv::flann::HierarchicalClusteringIndexParams();
		flannIndexParams = new cv::flann::LshIndexParams(20, 10, 2);
	} else { // float descriptors
		binaryDescriptor = false;
		bfNormType = cv::NORM_L2;
		flannIndexParams = new cv::flann::KDTreeIndexParams();
	}

	switch (descriptorMatcherSelection) {
	case 1: { descriptorMatcher = new cv::FlannBasedMatcher(flannIndexParams);	trainingConfigsTag << "_Flann-Matcher"; break; }
	case 2: { descriptorMatcher = new cv::BFMatcher(bfNormType, false);			trainingConfigsTag << "_BF-Matcher"; break; }
	default: break;
	}

	switch (bowTrainerSelection) {
	case 1: { bowTrainer = new cv::BOWKMeansTrainer(VOCABULARY_WORD_COUNT);	trainingConfigsTag << "_KMeans-BowTrainer"; break; }
	default: break;
	}

	string trainingDataFilename = trainingConfigsTag.str();

	stringstream vocabularyFilenameSS;
	vocabularyFilenameSS << VOCABULARY_TAG << trainingConfigsTag.str();

	switch (classifierSelection) {
	case 1: { trainingConfigsTag << "_SVM-Classifier"; break; }
	case 2: { trainingConfigsTag << "_ANN-Classifier"; break; }
	case 3: { trainingConfigsTag << "_Bayes-Classifier"; break; }		
	case 4: { trainingConfigsTag << "_DT-Classifier"; break; }
	case 5: { trainingConfigsTag << "_BT-Classifier"; break; }
	case 6: { trainingConfigsTag << "_GBT-Classifier"; break; }
	case 7: { trainingConfigsTag << "_RT-Classifier"; break; }
	case 8: { trainingConfigsTag << "_ERT-Classifier"; break; }		
			//case 9: { trainingConfigsTag << "_KNN-Classifier"; break; }
	default: break;
	}

	stringstream classifierFilenameSS;	
	classifierFilenameSS << CLASSIFIER_TAG << trainingConfigsTag.str();

	_bowVocabulary = new BowVocabulary(featureDetector, descriptorExtractor, descriptorMatcher, bowTrainer, _imagePreprocessor, vocabularyFilenameSS.str(), CLASSIFIER_TAG + trainingDataFilename, binaryDescriptor);

	switch (classifierSelection) {
	case 1: { _imageClassifier = new ImageClassifierSVM(_bowVocabulary, classifierFilenameSS.str()); break; }
	case 2: { _imageClassifier = new ImageClassifierANN(_bowVocabulary, classifierFilenameSS.str()); break; }
	case 3: { _imageClassifier = new ImageClassifierBayes(_bowVocabulary, classifierFilenameSS.str()); break; }		
	case 4: { _imageClassifier = new ImageClassifierDecisionTrees(_bowVocabulary, classifierFilenameSS.str()); break; }
	case 5: { _imageClassifier = new ImageClassifierBoost(_bowVocabulary, classifierFilenameSS.str()); break; }
	case 6: { _imageClassifier = new ImageClassifierGradientBoostingTrees(_bowVocabulary, classifierFilenameSS.str()); break; }
	case 7: { _imageClassifier = new ImageClassifierRandomTrees(_bowVocabulary, classifierFilenameSS.str()); break; }
	case 8: { _imageClassifier = new ImageClassifierExtremelyRandomizedTrees(_bowVocabulary, classifierFilenameSS.str()); break; }				
			//case 9: { _imageClassifier = new ImageClassifierKNN(_bowVocabulary, classifierFilenameSS.str()); break; }
	default: break;
	}

	//_imageClassifier->trainClassifier(VOCABULARY_IMAGES_LIST, CLASSIFIER_IMGAGES_LIST);
	_imageClassifier->trainClassifier("traffic/image/listcar.txt", "traffic/image/listcartrain.txt");

	_imageDetector = new ImageDetectorSlidingWindow(_imageClassifier);
	cout << "-->Training setup OK \n" << endl;
	imageAnalysis.setImagePreprocessor(UserGui._imagePreprocessor);
	imageAnalysis.setImageDetector(UserGui._imageDetector);
	
}

void CGUI_IMAGEDlg::OnBnClickedButton2()
{
	// TODO: Start training
	if(is_sparse_coding){
		train();
	}else {
		UserGui.setupTraining();
	}
}

/*
 * loop through every directory
 * compute each image's keypoints and descriptors
 * train a vocabulary
 */
void  train()
{
	
	switch (g_featureDetectorSelection) {
	case 1: { params.featureDetector = new cv::SiftFeatureDetector(); break; }
	case 2: { params.featureDetector = new cv::SurfFeatureDetector(400); break; }
	case 3: { params.featureDetector = new cv::GoodFeaturesToTrackDetector(); break; }
	case 4: { params.featureDetector = new cv::FastFeatureDetector(); break; }		
	case 5: { params.featureDetector = new cv::OrbFeatureDetector(); break; }
	case 6: { params.featureDetector = new cv::BRISK(); break; }
	case 7: { params.featureDetector = new cv::StarFeatureDetector(); break; }
	case 8: { params.featureDetector = new cv::MserFeatureDetector(); break; }	
	default: break;
	}

	switch (g_descriptorExtractorSelection) {
	case 1: { params.descriptorExtractor = new cv::SiftDescriptorExtractor(); break; }
	case 2: { params.descriptorExtractor = new cv::SurfDescriptorExtractor(); break; }
	case 3: { params.descriptorExtractor = new cv::FREAK();	 break; }
	case 4: { params.descriptorExtractor = new cv::BriefDescriptorExtractor(); break; }		
	case 5: { params.descriptorExtractor = new cv::OrbDescriptorExtractor(); break; }
	case 6: { params.descriptorExtractor = new cv::BRISK(); break; }
	default: break;
	}
	bool binaryDescriptor;
	int bfNormType;
	Ptr<cv::flann::IndexParams> flannIndexParams/* = new cv::flann::AutotunedIndexParams()*/;
	if (g_descriptorExtractorSelection > 2) { // binary descriptors
		binaryDescriptor = true;
		bfNormType = cv::NORM_HAMMING;
		//flannIndexParams = new cv::flann::HierarchicalClusteringIndexParams();
		flannIndexParams = new cv::flann::LshIndexParams(20, 10, 2);
	} else { // float descriptors
		binaryDescriptor = false;
		bfNormType = cv::NORM_L2;
		flannIndexParams = new cv::flann::KDTreeIndexParams();
	}

	switch (g_descriptorMatcherSelection) {
	case 1: { params.descriptorMatcher = new cv::FlannBasedMatcher(flannIndexParams); break; }
	case 2: { params.descriptorMatcher = new cv::BFMatcher(bfNormType, false); break; }
	default: break;
	}

	string databaseDir = "data/train";
	string resultDir =  "data/result/";

	string bowImageDescriptorsDir = resultDir + kBowImageDescriptorsDir;
	string svmsDir = resultDir + kSvmsDirs;
	MakeDir( resultDir );
	MakeDir( bowImageDescriptorsDir );
	MakeDir( svmsDir );

	// key: image category name
	// value: histogram of image
	vector<string> categories;
	GetDirList( databaseDir, &categories );

	Ptr<FeatureDetector> detector = params.featureDetector;
	Ptr<DescriptorExtractor> extractor = params.descriptorExtractor;
	Ptr<DescriptorMatcher> matcher = params.descriptorMatcher;

	if ( detector.empty() || extractor.empty() || matcher.empty() ) {
		cout << "feature detector or descriptor extractor or descriptor matcher cannot be created.\n Maybe try other types?" << endl;
	}

	Mat vocabulary;
	string vocabularyFile = resultDir + '/' + kVocabularyFile;
	FileStorage fs( vocabularyFile, FileStorage::READ );
	if ( fs.isOpened() ) {
		fs["vocabulary"] >> vocabulary;
	} else {
		vocabulary = BuildVocabulary( databaseDir, categories, detector, extractor, params.wordCount );
		FileStorage fs( vocabularyFile, FileStorage::WRITE );
		if ( fs.isOpened() ) {
			fs << "vocabulary" << vocabulary;
		}
	}
	Ptr<BOWImgDescriptorExtractor> bowExtractor = new BOWImgDescriptorExtractor( extractor, matcher );
	bowExtractor -> setVocabulary( vocabulary );
	map<string, Mat> samples;//key: category name, value: histogram

	ComputeBowImageDescriptors( databaseDir, vocabulary, categories, detector, extractor, bowExtractor, bowImageDescriptorsDir,  &samples );

	SVMParams svmParams;
	svmParams.svm_type = CvSVM::C_SVC;
	svmParams.kernel_type = CvSVM::LINEAR;
	//svmParams.kernel_type = CvSVM::RBF;
	svmParams.term_crit = cvTermCriteria(CV_TERMCRIT_ITER, 1e+3, 1e-6);

	int sign = 0; //sign of the positive class
	float confidence = -FLT_MAX;
	for ( map<string, Mat>::const_iterator itr = samples.begin(); itr != samples.end(); ++itr ) {
		CvSVM svm;
		string svmFileName = svmsDir + "/" + itr -> first + ".xml";

		std::cout << "TrainSvm " <<  svmFileName  << std::endl;

		TrainSvm( samples, itr->first, svmParams, &svm );
		if (svmsDir != "") svm.save( svmFileName.c_str() );
	}

	std::cout << "train  done" << std::endl;
}

Mat BuildVocabulary( const string& databaseDir,
					 const vector<string>& categories,
					 const Ptr<FeatureDetector>& detector,
					 const Ptr<DescriptorExtractor>& extractor,
					 int wordCount) {
	Mat allDescriptors;
	for ( uint32_t index = 0; index != categories.size(); ++index ) {
		cout << "processing category " << categories[index] << endl;
		string currentCategory = databaseDir + '/' + categories[index];
		vector<string> filelist;
		GetFileList( currentCategory, &filelist);
		for ( vector<string>::iterator fileindex = filelist.begin(); fileindex != filelist.end(); ++fileindex ) {
			string filepath = currentCategory + '/' + *fileindex;
			Mat image = imread( filepath );
			if ( image.empty() ) {
				continue; // maybe not an image file
			}
			vector<KeyPoint> keyPoints;
			vector<KeyPoint> keyPoints01;
			Mat descriptors;
			detector -> detect( image, keyPoints01);

                        for(uint32_t i=0; i<keyPoints01.size(); i++)
                        {
                                KeyPoint  myPoint;

                                myPoint = keyPoints01[i];

                                if (myPoint.size >= MIN_KPS) keyPoints.push_back(myPoint);
                        }


			extractor -> compute( image, keyPoints, descriptors );
			if ( allDescriptors.empty() ) {
				allDescriptors.create( 0, descriptors.cols, descriptors.type() );
			}
			allDescriptors.push_back( descriptors );
		}
		cout << "done processing category " << categories[index] << endl;
	}
	assert( !allDescriptors.empty() );
	cout << "build vocabulary..." << endl;
	BOWKMeansTrainer bowTrainer( wordCount );
	Mat vocabulary = bowTrainer.cluster( allDescriptors );
	cout << "done build vocabulary..." << endl;
	return vocabulary;
}

void  opencv_llc_bow_Descriptor(Mat &image, Mat &vocabulary,  vector<KeyPoint> &key_points, Mat &llc_descriptor)
{
        //std::cout << "opencv_llc_bow_Descriptor" << std::endl;

 	Mat descriptors;

        //Params params;

        Ptr<DescriptorExtractor> extractor =params.descriptorExtractor;

        extractor -> compute( image, key_points, descriptors );

        int     knn = 5;
        float  fbeta = 1e-4;

        vector<vector<DMatch> > matches;

        //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "BruteForce" );
        //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create( "FlannBased" );

		Ptr<DescriptorMatcher> matcher = params.descriptorMatcher;
        matcher -> knnMatch( descriptors, vocabulary, matches, knn );

        Mat  des_mat_r01;
        for (int icx=0; icx<descriptors.rows; icx++)
        {
                des_mat_r01 = descriptors.row(icx);

                vector<DMatch> &matchesv1 = matches[icx];

                Mat  mat_cbknn;

                mat_cbknn.release();


                for (int i=0; i<knn; i++)
                {
                        Mat  mat_idx01 = vocabulary.row(matchesv1[i].trainIdx);

                        mat_cbknn.push_back(mat_idx01);
                }
                //std::cout << "mat_cbknn size : " << mat_cbknn.rows << "  " << mat_cbknn.cols << std::endl;

                Mat  ll_mat = Mat::eye(knn, knn, CV_32FC1);
                Mat  z_mat = mat_cbknn - repeat(des_mat_r01, 5, 1);
                Mat  one_mat = Mat::ones(knn, 1, CV_32FC1);
                Mat  c_mat = z_mat*z_mat.t();

                float  ftrace = trace(c_mat).val[0];

                c_mat = c_mat + ll_mat*fbeta*ftrace;

                Mat  w_mat = c_mat.inv()*one_mat;

                w_mat = w_mat/sum(w_mat).val[0];

                w_mat = w_mat.t();

                for (int i=0; i<knn; i++)
                {
                        llc_descriptor.at<float>(0, matchesv1[i].trainIdx) += w_mat.at<float>(0,i);
                }
        }

        llc_descriptor = llc_descriptor/(descriptors.rows*1.0);
}


// bag of words of an image as its descriptor, not keypoint descriptors
void ComputeBowImageDescriptors( const string& databaseDir, Mat& vocabulary,
								 const vector<string>& categories,
								 const Ptr<FeatureDetector>& detector,
								 const Ptr<DescriptorExtractor>& extractor,
								 Ptr<BOWImgDescriptorExtractor>& bowExtractor,
								 const string& imageDescriptorsDir,
								 map<string, Mat>* samples) {

        std::cout << "vocabulary rows cols = " << vocabulary.rows << "  " << vocabulary.cols << std::endl;

	for (uint32_t  i = 0; i != categories.size(); ++i ) {
		string currentCategory = databaseDir + '/' + categories[i];
		vector<string> filelist;
		GetFileList( currentCategory, &filelist);
		for ( vector<string>::iterator fileitr = filelist.begin(); fileitr != filelist.end(); ++fileitr ) {
			string descriptorFileName = imageDescriptorsDir + "/" + categories[i] + "_" + ( *fileitr ) + ".xml";

			std::cout << "bow: " << descriptorFileName << std::endl;

			FileStorage fs( descriptorFileName, FileStorage::READ );
			Mat imageDescriptor;
			if ( fs.isOpened() ) { // already cached
				fs["imageDescriptor"] >> imageDescriptor;
			} else {
				string filepath = currentCategory + '/' + *fileitr;
				Mat image = imread( filepath );
				if ( image.empty() ) {
					continue; // maybe not an image file
				}
				vector<KeyPoint> keyPoints;
				vector<KeyPoint> keyPoints01;

				detector -> detect( image, keyPoints01 );

                                for(uint32_t i=0; i<keyPoints01.size(); i++)
                                {
                                        KeyPoint  myPoint;

                                        myPoint = keyPoints01[i];

                                        if (myPoint.size >= MIN_KPS) keyPoints.push_back(myPoint);
                                }


                                imageDescriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

				opencv_llc_bow_Descriptor( image, vocabulary, keyPoints, imageDescriptor );

				//std::cout << "imageDescriptor rows cols = " << imageDescriptor.rows << "  "
                                                //<< imageDescriptor.cols << std::endl;

				fs.open( descriptorFileName, FileStorage::WRITE );
				if ( fs.isOpened() ) {
					fs << "imageDescriptor" << imageDescriptor;
				}
			}
			if ( samples -> count( categories[i] ) == 0 ) {
				( *samples )[categories[i]].create( 0, imageDescriptor.cols, imageDescriptor.type() );
			}
			( *samples )[categories[i]].push_back( imageDescriptor );
		}
	}
}

void TrainSvm( const map<string, Mat>& samples, const string& category, const CvSVMParams& svmParams, CvSVM* svm ) {
	Mat allSamples( 0, samples.at( category ).cols, samples.at( category ).type() );
	Mat responses( 0, 1, CV_32SC1 );
	//assert( responses.type() == CV_32SC1 );
	allSamples.push_back( samples.at( category ) );
	Mat posResponses( samples.at( category ).rows, 1, CV_32SC1, Scalar::all(1) );
	responses.push_back( posResponses );

	for (  map<string, Mat>::const_iterator itr = samples.begin(); itr != samples.end(); ++itr ) {
		if ( itr -> first == category ) {
			continue;
		}
		allSamples.push_back( itr -> second );
		Mat response( itr -> second.rows, 1, CV_32SC1, Scalar::all( -1 ) );
		responses.push_back( response );

	}
	svm -> train( allSamples, responses, Mat(), Mat(), svmParams );
}

void test()
{

	string  sample_name  = "Bicycle";
	string  test_dir  =  "data/imgs/Bicycle";
	string  svms_dir = "data/result/svms";
	string  voc_fn =  "data/result/vocabulary.xml";

	Ptr<FeatureDetector> detector = params.featureDetector;
	Ptr<DescriptorExtractor> extractor = params.descriptorExtractor;

	vector<string>  imgs_fns;
	vector<string>  names_img_class;
	vector<string>  svms_fns;

	GetFileList( svms_dir, &svms_fns );
	GetFileList( test_dir, &imgs_fns );

	map<string, CvSVM*>  svms_map;


	for ( vector<string>::iterator itr = svms_fns.begin(); itr != svms_fns.end();  itr++)
	{
		string   svm_fn = *itr;

		int  n = svm_fn.find(".xml");

		string   name_ic;

		name_ic.assign(svm_fn, 0, n);

		//std::cout << "name_ic = " << name_ic << std::endl;

		CvSVM  *psvm = new CvSVM;

		string svmFileName = svms_dir + "/" + svm_fn;

		FileStorage fs( svmFileName, FileStorage::READ );
		if ( fs.isOpened() )
		{
			fs.release();
			psvm->load( svmFileName.c_str() );

			svms_map.insert(pair<string, CvSVM*>(name_ic, psvm));
		}
		else
		{
			std::cout << "svm : " << svmFileName << " can not load " << std::endl;
			exit(-1);
		}

		std::cout << name_ic << " svm :  " << svmFileName << std::endl;
	}

	Mat vocabulary;
	std::string vocabularyFile =  voc_fn;

	FileStorage fs( vocabularyFile, FileStorage::READ );
	if ( fs.isOpened() )  fs["vocabulary"] >> vocabulary;

	std::cout << "vocabularyFile :  " << vocabularyFile << std::endl;
	std::cout << "vocabulary rows cols = " << vocabulary.rows << "  " << vocabulary.cols << std::endl;

	std::cout << sample_name << " test...  " << std::endl;

	int  num_correct = 0;

	for ( vector<string>::iterator itr = imgs_fns.begin(); itr != imgs_fns.end();  itr++)
	{


		string  category;

		string  img_fn = *itr;

		string  queryImage = test_dir + "/" + img_fn;

		Mat image = imread( queryImage );
		const clock_t begin_time = clock();
		vector<KeyPoint> keyPoints;
		vector<KeyPoint> keyPoints01;
		detector -> detect( image, keyPoints01 );


		for(uint32_t i=0; i<keyPoints01.size(); i++)
		{
			KeyPoint  myPoint;

			myPoint = keyPoints01[i];

			if (myPoint.size >= MIN_KPS) keyPoints.push_back(myPoint);
		}

		Mat queryDescriptor;

		queryDescriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

		opencv_llc_bow_Descriptor( image, vocabulary, keyPoints, queryDescriptor );

		int sign = 0; //sign of the positive class
		float confidence = -FLT_MAX;
		for (map<string, CvSVM*>::const_iterator itr = svms_map.begin(); itr != svms_map.end(); ++itr )
		{
			CvSVM  *psvm = itr->second;

			if ( sign == 0 ) {

				float scoreValue = psvm->predict( queryDescriptor, true );
				float classValue = psvm->predict( queryDescriptor, false );
				sign = ( scoreValue < 0.0f ) == ( classValue < 0.0f )? 1 : -1;

			}
			float curConfidence = sign * psvm->predict( queryDescriptor, true );
			if ( curConfidence > confidence ) {
				confidence = curConfidence;
				category = itr -> first;
			}

		}

		std::cout << queryImage << " : " << category << std::endl;

		if (sample_name == category) num_correct++;
		std::cout << "Time spent detect:" <<float( clock () - begin_time ) /  CLOCKS_PER_SEC << "\n" << endl;
	}


	std::cout << num_correct << " " << imgs_fns.size() << " " << num_correct*1.0/imgs_fns.size() << std::endl;
	system("pause");
}
