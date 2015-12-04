
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
//boost lib
#include <boost/thread.hpp>

/*----------------------------------------------include files -----------------------------------------------*/
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

//global variable
CWnd *g_pMyDialog ;
CWnd *pWnd_STATIC_CAR;
CWnd *pWnd_STATIC_TRUCK;
CWnd *pWnd_STATIC_BUS;
CWnd *pWnd_STATIC_VAN;
CWnd *pWnd_STATIC_CONTAINER;
//

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
int ResourceFlag = 1; //image or video
int carNumber = 0;
int truckNumber = 0;
int busNumber = 0;
int vanNumber = 0;
int containerNumber = 0;
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
void opencv_llc_bow_Descriptor(Mat &image, Mat &vocabulary,  vector<KeyPoint> &key_points, Mat &llc_descriptor);
void train();
void test();
void test_non_spare();
void PrepareForPredict();
void test_single_image(Mat predictImage);
void test_single_image_non_spare (Mat predictImage);
void updateCarNumber();
void updateTruckNumber();
void updateBusNumber();
void updateVanNumber();
void updateContainerNumber();

//param
class Params {
public:
	Params(): wordCount(VOCA_COLS){}
	int		wordCount;
	Ptr<FeatureDetector> featureDetector;
	Ptr<DescriptorExtractor> descriptorExtractor;
	Ptr<DescriptorMatcher> descriptorMatcher;
};

string file_name="";
string video_name="";
bool is_image = true;
bool is_exit = false;
bool is_llc = true;
bool is_debug_mode = false;
Params params;
ViCapture* videoCapture;

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
	DDX_Control(pDX, IDC_CHECK2, CheckBoxDebug);
	DDX_Control(pDX, IDC_STATIC_CAR, StaticCar);
	DDX_Control(pDX, IDC_STATIC_TRUCK, StaticTruck);
	DDX_Control(pDX, IDC_STATIC_BUS, StaticBus);
	DDX_Control(pDX, IDC_STATIC_VAN, StaticVan);
	DDX_Control(pDX, IDC_STATIC_CONTAINER, StaticContainer);
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
	ON_BN_CLICKED(IDC_CHECK2, &CGUI_IMAGEDlg::OnBnClickedCheck2)
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
	ComboBox2.SetCurSel(1); //video
	//Check llc
	CheckBox.SetCheck(BST_CHECKED);
	//Check debug
	CheckBoxDebug.SetCheck(BST_UNCHECKED);
	//number of vehicle
	CFont *m_pFont = new CFont();
	m_pFont->CreatePointFont(250, _T("Arial"));
	StaticCar.SetFont(m_pFont, TRUE);
	StaticTruck.SetFont(m_pFont, TRUE);
	StaticBus.SetFont(m_pFont, TRUE);
	StaticVan.SetFont(m_pFont, TRUE);
	StaticContainer.SetFont(m_pFont, TRUE);
	//end number vehicle
	if (!AllocConsole()){
		AfxMessageBox(_T("Failed to create the console!"));
	}
	*stdout = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), _O_WRONLY), _T("a"));
	*stderr = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE), _O_WRONLY), _T("a"));
	*stdin = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_INPUT_HANDLE), _O_WRONLY), _T("r"));
	printf("Init console window successfully\n");
	//Static text 
	g_pMyDialog = AfxGetMainWnd();
	pWnd_STATIC_CAR = g_pMyDialog->GetDlgItem(IDC_STATIC_CAR);
	pWnd_STATIC_TRUCK = g_pMyDialog->GetDlgItem(IDC_STATIC_TRUCK);
	pWnd_STATIC_BUS = g_pMyDialog->GetDlgItem(IDC_STATIC_BUS);
	pWnd_STATIC_VAN = g_pMyDialog->GetDlgItem(IDC_STATIC_VAN);
	pWnd_STATIC_CONTAINER = g_pMyDialog->GetDlgItem(IDC_STATIC_CONTAINER);
	//end static text
	//init console window end
	updateCarNumber();
	updateTruckNumber();
	updateBusNumber();
	updateVanNumber();
	updateContainerNumber();
	return TRUE;  // return TRUE  unless you set the focus to a control
}



void updateCarNumber(){
	CString temp_carNumber;
	stringstream convert;
	convert << carNumber;
	temp_carNumber = convert.str().c_str();
	pWnd_STATIC_CAR->SetWindowText(temp_carNumber);
}
void updateTruckNumber(){
	CString temp_truckNumber;
	stringstream convert;
	convert << truckNumber;
	temp_truckNumber = convert.str().c_str();
	pWnd_STATIC_TRUCK->SetWindowText(temp_truckNumber);
}

void updateBusNumber(){
	CString temp_busNumber;
	stringstream convert;
	convert << busNumber;
	temp_busNumber = convert.str().c_str();
	pWnd_STATIC_BUS->SetWindowText(temp_busNumber);
}
void updateVanNumber(){
	CString temp_vanNumber;
	stringstream convert;
	convert << vanNumber;
	temp_vanNumber = convert.str().c_str();
	pWnd_STATIC_VAN->SetWindowText(temp_vanNumber);
}

void updateContainerNumber(){
	CString temp_containerNumber;
	stringstream convert;
	convert << containerNumber;
	temp_containerNumber = convert.str().c_str();
	pWnd_STATIC_CONTAINER->SetWindowText(temp_containerNumber);
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
			//Mat dest;
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
			video_name = (CStringA(CSfile_name));
			//VideoCapture cap(video_name); // open the default camera
			videoCapture = new ViCapture;
			videoCapture->setVideo(video_name);
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
	/*if(is_llc){
		test();//test_non_spare();
	}else {
		test_non_spare();
	}
	return;*/
	if(ResourceFlag == 0){
		if(is_llc){
			PrepareForPredict();
			Mat image = imread(file_name);
			//imshow("Hello",image);
			//std::cout << "Time spent bgs:" <<float( clock () - begin_time1 )/CLOCKS_PER_SEC << "\n" << std::endl ;
			const clock_t begin_time = clock();
			test_single_image(image);
			std::cout << "Time testing:" <<float( clock () - begin_time )/CLOCKS_PER_SEC << "\n" << std::endl ;

		}else{
			Mat image = imread(file_name);
			const clock_t begin_time = clock();
			test_single_image_non_spare(image);
			std::cout << "Time testing:" <<float( clock () - begin_time )/CLOCKS_PER_SEC << "\n" << std::endl ;
		}

	} else {
		if(is_llc){
			PrepareForPredict();
		}
		videoCapture->setUpVideo();
		videoCapture->start();
	}
}

void CGUI_IMAGEDlg::OnBnClickedCheck1()
{
	
	// TODO: Add your control notification handler code here
	UINT nCheck = CheckBox.GetCheck();
    if (nCheck == BST_CHECKED)
    {
        is_llc = true;
    }
    else
    {
        is_llc = false;
    }

}
void CGUI_IMAGEDlg::OnBnClickedCheck2()
{
	// TODO: Add your control notification handler code here
	UINT nCheck = CheckBoxDebug.GetCheck();
	if (nCheck == BST_CHECKED)
	{
		is_debug_mode = true;
	}
	else
	{
		is_debug_mode = false;
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
	if(is_llc){
		const clock_t begin_time = clock();
		train();
		std::cout << "Time spent train linear SVM:" <<float( clock () - begin_time )/CLOCKS_PER_SEC << "\n" << std::endl ;
	}else {
		const clock_t begin_time = clock();
		UserGui.setupTraining();
		std::cout << "Time spent train non-linear SVM :" <<float( clock () - begin_time )/CLOCKS_PER_SEC << "\n" << std::endl ;
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


Mat vocabulary_sparse;
map<string, CvSVM*> svms_map_sparse;
Ptr<FeatureDetector> detector_sparse;
Ptr<DescriptorExtractor> extractor_sparse ;

void PrepareForPredict(){

	string  svms_dir = "data/result/svms";
	string  voc_fn =  "data/result/vocabulary.xml";
	vector<string>  svms_fns;

	detector_sparse = params.featureDetector;
	extractor_sparse = params.descriptorExtractor;
	GetFileList( svms_dir, &svms_fns );
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

			svms_map_sparse.insert(pair<string, CvSVM*>(name_ic, psvm));
		}
		else
		{
			std::cout << "svm : " << svmFileName << " can not load " << std::endl;
			exit(-1);
		}

		//std::cout << name_ic << " svm :  " << svmFileName << std::endl;
	}

	std::string vocabularyFile =  voc_fn;

	FileStorage fs( vocabularyFile, FileStorage::READ );
	if ( fs.isOpened() )  fs["vocabulary"] >> vocabulary_sparse;

	//std::cout << "vocabularyFile :  " << vocabularyFile << std::endl;
	//std::cout << "vocabulary rows cols = " << vocabulary_sparse.rows << "  " << vocabulary_sparse.cols << std::endl;
}

void test_non_spare(){
	string classCar ="";
	string dir[5]= {"Car","Bus","Container","Truck","Van"};
	const clock_t begin_time = clock();

	
		for(int k=0;k<=4;k++) {
			string  sample_name  = dir[k];
			string  test_dir  = "D:/video/TestResult/" + dir[k];
			//string  sample_name  = "Bus";
			//string  test_dir  =  "D:/OpenCV/Project/GUIIMAGE/GUI/GUI_IMAGE/data/train/Bus";// Car,Bus,Container,Truck,Van
			//string  test_dir  =  "D:/video/MauTest/1/Bus";
			vector<string>  imgs_fns;
			vector<string>  names_img_class;
			PrepareForPredict();
			GetFileList( test_dir, &imgs_fns );
			//std::cout << sample_name << " test...  " << std::endl;
			int  num_correct = 0;
			int  vehicleinclass=0;
			int  vehicleinrightclass=0;
			int  carclass=0;
			int  busclass=0;
			int  conclass=0;
			int  truckclass=0;
			int  vanclass=0;
			for ( vector<string>::iterator itr = imgs_fns.begin(); itr != imgs_fns.end();  itr++)
			{
				string  category;
				string  img_fn = *itr;
				string  queryImage = test_dir + "/" + img_fn;
				vehicleinclass++;
				Mat image = imread(queryImage);
				imageAnalysis.processImage(image);
				if(UpdateLabelFlag == (float)1){
					classCar ="Car";
					carclass++;
				} else if(UpdateLabelFlag == (float)2){
					classCar ="Bus";
					busclass++;
				} else if(UpdateLabelFlag == (float)3){
					classCar ="Container";
					conclass++;
				} else if(UpdateLabelFlag == (float)4){
					classCar ="Truck";
					truckclass++;
				} else if(UpdateLabelFlag == (float)5){
					classCar ="Van";
					vanclass++;
				}
				std::cout << queryImage  << " is class : " << classCar << std::endl;
				if(classCar == sample_name){
					vehicleinrightclass++;
				}
			}
			std::cout << vehicleinrightclass << "/" << vehicleinclass << "==>" << ((float)vehicleinrightclass/vehicleinclass)*100 <<"%" <<std::endl;
			std::cout <<" car: " << carclass <<"Bus: " <<busclass << " Con: " << conclass << " Truck: "  <<truckclass  << " Van: " << vanclass <<std::endl;
			std::cout << "----------------------"<< sample_name <<"-------------------"<< std::endl;
		}
		std::cout << "Time spent testing non-linear SVM:" << float( clock () - begin_time )/CLOCKS_PER_SEC << "\n" << std::endl ;

}

void test()
{
		string dir[5]= {"Car","Bus","Container","Truck","Van"};
		const clock_t begin_time = clock();
		for(int k=0;k<=4;k++) {
			string  sample_name  = dir[k];
			string  test_dir  =  "D:/video/TestResult/" + dir[k];

			//string  sample_name  = "Bus";
			//string  test_dir  =  "D:/OpenCV/Project/GUIIMAGE/GUI/GUI_IMAGE/data/train/Bus";// Car,Bus,Container,Truck,Van
			//string  test_dir  =  "D:/video/MauTest/1/Bus";

			vector<string>  imgs_fns;
			vector<string>  names_img_class;
			PrepareForPredict();
			GetFileList( test_dir, &imgs_fns );

			//std::cout << sample_name << " test...  " << std::endl;

			int  num_correct = 0;
			int  carclass=0;
			int  busclass=0;
			int  conclass=0;
			int  truckclass=0;
			int  vanclass=0;
			for ( vector<string>::iterator itr = imgs_fns.begin(); itr != imgs_fns.end();  itr++)
			{


				string  category;

				string  img_fn = *itr;

				string  queryImage = test_dir + "/" + img_fn;

				Mat image = imread( queryImage );
				const clock_t begin_time = clock();
				vector<KeyPoint> keyPoints;
				vector<KeyPoint> keyPoints01;
				detector_sparse -> detect( image, keyPoints01 );


				for(uint32_t i=0; i<keyPoints01.size(); i++)
				{
					KeyPoint  myPoint;

					myPoint = keyPoints01[i];

					if (myPoint.size >= MIN_KPS) keyPoints.push_back(myPoint);
				}

				Mat queryDescriptor;

				queryDescriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

				opencv_llc_bow_Descriptor( image, vocabulary_sparse, keyPoints, queryDescriptor );

				int sign = 0; //sign of the positive class
				float confidence = -FLT_MAX;
				for (map<string, CvSVM*>::const_iterator itr = svms_map_sparse.begin(); itr != svms_map_sparse.end(); ++itr )
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
				if(category =="Car"){
					carclass++;
				}else if(category == "Bus"){
					busclass++;
				}else if(category == "Container"){
					conclass++;
				}else if(category == "Truck"){
					truckclass++;
				}else if(category == "Van"){
					vanclass++;
				}
				if (sample_name == category) num_correct++;
				//std::cout << "Time spent detect:" <<float( clock () - begin_time ) /  CLOCKS_PER_SEC << "\n" << endl;
			}
			std::cout << num_correct << "/" << imgs_fns.size() << "==>" << num_correct*100.0/imgs_fns.size() <<"%"<<std::endl;
			std::cout <<" car: " << carclass <<"Bus: " <<busclass << " Con: " << conclass << " Truck: "  <<truckclass  << " Van: " << vanclass <<std::endl;
			std::cout << "----------------------"<< sample_name <<"-------------------"<< std::endl;	
			}
		std::cout << "Time spent testing linear SVM:" << float( clock () - begin_time )/CLOCKS_PER_SEC << "\n" << std::endl ;

}


/*void test()
{
	string dir[5]= {"Car","Bus","Container","Truck","Van"};
	string sample[8]= {"1","2","3","4","5","6","7","8"};
	for(int j=0;j<=7;j++) {
		for(int k=0;k<=4;k++) {

		string  sample_name  = dir[k];
		string  test_dir  =  "D:/video/MauTest/"+ sample[j] +"/"+ dir[k];

	//string  sample_name  = "Bus";
	//string  test_dir  =  "D:/OpenCV/Project/GUIIMAGE/GUI/GUI_IMAGE/data/train/Bus";// Car,Bus,Container,Truck,Van
	//string  test_dir  =  "D:/video/MauTest/1/Bus";

	vector<string>  imgs_fns;
	vector<string>  names_img_class;
	PrepareForPredict();
	GetFileList( test_dir, &imgs_fns );

	//std::cout << sample_name << " test...  " << std::endl;

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
		detector_sparse -> detect( image, keyPoints01 );


		for(uint32_t i=0; i<keyPoints01.size(); i++)
		{
			KeyPoint  myPoint;

			myPoint = keyPoints01[i];

			if (myPoint.size >= MIN_KPS) keyPoints.push_back(myPoint);
		}

		Mat queryDescriptor;

		queryDescriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

		opencv_llc_bow_Descriptor( image, vocabulary_sparse, keyPoints, queryDescriptor );

		int sign = 0; //sign of the positive class
		float confidence = -FLT_MAX;
		for (map<string, CvSVM*>::const_iterator itr = svms_map_sparse.begin(); itr != svms_map_sparse.end(); ++itr )
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
		//std::cout << "Time spent detect:" <<float( clock () - begin_time ) /  CLOCKS_PER_SEC << "\n" << endl;
	}

		std::cout << num_correct << "/" << imgs_fns.size() << "==>" << num_correct*100.0/imgs_fns.size() <<"%"<<std::endl;
		}
	std::cout << "------------------ " << sample[j] <<" ------------------"<<std::endl;
	}
	system("pause");
}*/

void test_single_image_non_spare (Mat predictImage){
	imageAnalysis.processImage(predictImage);
	if(UpdateLabelFlag ==(float)1){
		updateCarNumber();
	}else if(UpdateLabelFlag ==(float)2){
		updateBusNumber();
	}else if(UpdateLabelFlag ==(float)3){
		updateContainerNumber();
	}else if(UpdateLabelFlag ==(float)4){
		updateTruckNumber();
	}else if(UpdateLabelFlag ==(float)5){
		updateVanNumber();
	}

}

void test_single_image (Mat predictImage){

	const clock_t begin_time = clock();
	vector<KeyPoint> keyPoints;
	vector<KeyPoint> keyPoints01;
	detector_sparse -> detect( predictImage, keyPoints01 );
	string  category;
	for(uint32_t i=0; i<keyPoints01.size(); i++)
	{
		KeyPoint  myPoint;

		myPoint = keyPoints01[i];

		if (myPoint.size >= MIN_KPS) keyPoints.push_back(myPoint);
	}

	Mat queryDescriptor;

	queryDescriptor = Mat::zeros(1, VOCA_COLS, CV_32F);

	opencv_llc_bow_Descriptor( predictImage, vocabulary_sparse, keyPoints, queryDescriptor );

	int sign = 0; //sign of the positive class
	float confidence = -FLT_MAX;
	for (map<string, CvSVM*>::const_iterator itr = svms_map_sparse.begin(); itr != svms_map_sparse.end(); ++itr )
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
	if(category == "Car"){
		carNumber++;
		updateCarNumber();
	}
	if(category == "Truck"){
		truckNumber++;
		updateTruckNumber();
	}
	if(category == "Bus"){
		busNumber++;
		updateBusNumber();
	}
	if(category == "Van"){
		vanNumber++;
		updateVanNumber();
	}
	if(category == "Container"){
		containerNumber++;
		updateContainerNumber();
	}
	std::cout << "Image is" << " : " << category << std::endl;
	//system("pause");
}

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
	//end Blob Tracking
	int frameignore = 0;
	float timewaiting= 0; 
	//
	cv::Mat img_mask;
	cv::Mat img_model_bg;
	double loopDelay = 33.333;
	bool firstTime = true;
	bool hasResult = false;	
	bool isblob = true;
	Mat imagepredict;

	if(input_fps > 0)
		loopDelay = (1./input_fps)*1000.;
	std::cout << "loopDelay:" << loopDelay << std::endl;
	loopDelay = 45;
	//system("pause");
	do
	{
		//frameNumber++;
		const clock_t begin_time1 = clock();
		frame1 = cvQueryFrame(capture);
		if(!frame1) break;

		ImageDisplay(frame1); //display input full size
		cvResize(frame1, frame);
		/*if(enableFlip)
			cvFlip(frame, frame, 0);*/
		if(VC_ROI::use_roi == true && VC_ROI::roi_defined == false && firstTime == true)
		{
			VC_ROI::reset();
			do
			{
				cv::Mat img_input(frame);
				if(showOutput)
				{
					cv::imshow("Input", img_input);
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

			} while(1);
		}
		
		if(VC_ROI::use_roi == true && VC_ROI::roi_defined == true)
		{
			CvRect rect = cvRect(VC_ROI::roi_x0, VC_ROI::roi_y0, VC_ROI::roi_x1 - VC_ROI::roi_x0, VC_ROI::roi_y1 - VC_ROI::roi_y0);
			cvSetImageROI(frame, rect);
		}
		cv::Mat img_input(frame);

		//if(showOutput)
			//cv::imshow("Input Region", img_input);

		if(firstTime)
			saveConfig();

		
		bgs->process(img_input, img_mask,img_model_bg);

		//std::cout << "Time spent bgs:" <<float( clock () - begin_time1 )/CLOCKS_PER_SEC << "\n" << std::endl ;
		//const clock_t begin_time = clock();

		if(!img_mask.empty())
		{
			
			if(is_debug_mode){
				cv::imshow("img_mask", img_mask);
			}
			if(isblob == false){
				frameignore++;
				if(frameignore >= 80) {
					isblob = true;
					frameignore = 0;
				}
			}
			std::cout << "frameignore :" << frameignore << "\n" << std::endl ;
			blobTracking->process(img_input, img_mask, img_blob, hasResult, imagepredict, isblob);
			std::cout << "hasResult: " << hasResult << std::endl ;
			if(is_debug_mode){
				cv::imshow("img_blob", img_blob);
			}
			if(hasResult){
				if( imagepredict.cols < (VC_ROI::roi_x1 - VC_ROI::roi_x0 - 5) ) {
					isblob = false ;
					if(is_debug_mode){
						cv::imshow("imagepredict", imagepredict);
					}

					if(is_llc){
						boost::thread t(&test_single_image,imagepredict);
					}else {
						boost::thread t(&test_single_image_non_spare,imagepredict);
					}
				}
			}
			//std::cout << "Time spent blobTracking:" << float( clock () - begin_time ) /  CLOCKS_PER_SEC << "\n" << std::endl ;
		}
		cvResetImageROI(frame);
		timewaiting = loopDelay - (double(clock() - begin_time1));
		if(timewaiting <= 1 ) {
			timewaiting = 1;
		}
		std::cout << "timewaiting:" << timewaiting << "\n" << std::endl ;
		key = cvWaitKey(timewaiting);
		//key = cvWaitKey(loopDelay);
		//std::cout << "key: " << key << std::endl;

		/*if(key == KEY_SPACE)
			key = cvWaitKey(0);

		if(key == KEY_ESC)
			break;

		if(stopAt > 0 && stopAt == frameNumber)
			key = cvWaitKey(0);*/

		firstTime = false;

	} while(1);

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


