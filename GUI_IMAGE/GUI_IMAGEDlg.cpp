
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
int g_descriptorMatcherSelection = 1;
int g_bowTrainerSelection = 1;
int g_classifierSelection = 1;
CLI UserGui;
ImageAnalysis imageAnalysis(NULL,NULL);
/*end of option setup*/

enum                             {cBLACK=0,cWHITE, cGREY, cRED, cORANGE, cYELLOW, cGREEN, cAQUA, cBLUE, cPURPLE, cPINK,  NUM_COLOR_TYPES};
char* sCTypes[NUM_COLOR_TYPES] = {"Black", "White","Grey","Red","Orange","Yellow","Green","Aqua","Blue","Purple","Pink"};
uchar cCTHue[NUM_COLOR_TYPES] =    {0,       0,      0,     0,     20,      30,      55,    85,   115,    138,     161};
uchar cCTSat[NUM_COLOR_TYPES] =    {0,       0,      0,    255,   255,     255,     255,   255,   255,    255,     255};
uchar cCTVal[NUM_COLOR_TYPES] =    {0,      255,    120,   255,   255,     255,     255,   255,   255,    255,     255};

CvScalar text_color= {255,255,255,0};
/* Face Detection HaarCascade Classifier file */
const char* cascadeFileFace = "haarcascades\\haarcascade_frontalface_alt.xml";
CvHaarClassifierCascade* cascadeFace;
string file_name="";
bool is_image = true;
bool is_exit=false;
bool is_show_debug = false;
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
	ComboBox2.SetCurSel(1);
	cascadeFace = (CvHaarClassifierCascade*)cvLoad(cascadeFileFace, 0, 0, 0 );
		namedWindow("DisplayImage",1);
		HWND hWnd =(HWND)cvGetWindowHandle("DisplayImage");
		HWND hParent=::GetParent(hWnd);
		::SetParent(hWnd,GetDlgItem(IDC_PICTURE)->m_hWnd);
		::ShowWindow(hParent,SW_HIDE);
		Mat display_init = cv::Mat(600, 900, CV_8UC3, Scalar(150,150,150));
		imshow("DisplayImage",display_init);
	// TODO: Add extra initialization here
	//Combobox
		ComboBox1.SetCurSel(0);
		ComboBox3.SetCurSel(0);
		ComboBox4.SetCurSel(0);
		ComboBox5.SetCurSel(0);
		ComboBox6.SetCurSel(0);
	//end combobox
		
		//init console window
		if (!AllocConsole()){
			AfxMessageBox(_T("Failed to create the console!"));
		}
		*stdout = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), _O_WRONLY), _T("a"));
		*stderr = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE), _O_WRONLY), _T("a"));
		*stdin = *_tfdopen(_open_osfhandle((intptr_t) GetStdHandle(STD_INPUT_HANDLE), _O_WRONLY), _T("r"));
		printf("Init console window\n");
		//end
	return TRUE;  // return TRUE  unless you set the focus to a control
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
	//CString text;
	//EditBox.GetWindowTextW(text);
	//EditBox.SetWindowTextW(_T("hello")+text);
	Mat display_init = cv::Mat(600, 900, CV_8UC3, Scalar(150,150, 150));
	ImageDisplay(display_init);
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
	// TODO: Add your control notification handler code here
}


void CGUI_IMAGEDlg::OnCbnSelchangeCombo2()
{
	// TODO: Add your control notification handler code here
	int choice = ComboBox2.GetCurSel();
	switch (choice){
		case 0 :
			text_color = CV_RGB(0,0,0);
			break;
		case 1 :
			text_color = CV_RGB(255,255,255);
			MessageBox(_T("white color"));
			break;
		case 2:
			text_color = CV_RGB(255,0,0);
			break;
		case 3:
			text_color = CV_RGB(0,255,0);
			break;
		case 4:
			text_color = CV_RGB(0,0,255);
			break;
		case 5:
			text_color = CV_RGB(255,255,0);
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
	CDialogEx::OnOK();
}

void CGUI_IMAGEDlg::OnBnClickedRec() //Recognize button
{
	/*IplImage* imageDisplay;
	IplImage* imageIn;
	if (file_name == "")
	{
		return;
	}
	else
	{
	const char *file_name_1= file_name.c_str();
	imageIn = cvLoadImage(file_name_1, CV_LOAD_IMAGE_UNCHANGED);
		if (!imageIn) {
			cerr << "Couldn't load image file '" << file_name << "'" << endl;
			exit(1);
		}
	imageDisplay = cvCloneImage(imageIn);
	// If trying to debug the color detector code, enable this:
	if(is_show_debug)
	{
		// Create a HSV image showing the color types of the whole image, for debugging.
		IplImage *imageInHSV = cvCreateImage(cvGetSize(imageIn), 8, 3);
		cvCvtColor(imageIn, imageInHSV, CV_BGR2HSV);	// (note that OpenCV stores RGB images in B,G,R order.
		IplImage* imageDisplayHSV = cvCreateImage(cvGetSize(imageIn), 8, 3);	// Create an empty HSV image
		//cvSet(imageDisplayHSV, cvScalar(0,0,0, 0));	// Clear HSV image to blue.
		int hIn = imageDisplayHSV->height;
		int wIn = imageDisplayHSV->width;
		int rowSizeIn = imageDisplayHSV->widthStep;		// Size of row in bytes, including extra padding
		char *imOfsDisp = imageDisplayHSV->imageData;	// Pointer to the start of the image HSV pixels.
		char *imOfsIn = imageInHSV->imageData;	// Pointer to the start of the input image HSV pixels.
		for (int y=0; y<hIn; y++) {
			for (int x=0; x<wIn; x++) {
				// Get the HSV pixel components
				uchar H = *(uchar*)(imOfsIn + y*rowSizeIn + x*3 + 0);	// Hue
				uchar S = *(uchar*)(imOfsIn + y*rowSizeIn + x*3 + 1);	// Saturation
				uchar V = *(uchar*)(imOfsIn + y*rowSizeIn + x*3 + 2);	// Value (Brightness)
				// Determine what type of color the HSV pixel is.
				int ctype = getPixelColorType(H, S, V);
				//ctype = x / 60;
				// Show the color type on the displayed image, for debugging.
				*(uchar*)(imOfsDisp + (y)*rowSizeIn + (x)*3 + 0) = cCTHue[ctype];	// Hue
				*(uchar*)(imOfsDisp + (y)*rowSizeIn + (x)*3 + 1) = cCTSat[ctype];	// Full Saturation (except for black & white)
				*(uchar*)(imOfsDisp + (y)*rowSizeIn + (x)*3 + 2) = cCTVal[ctype];		// Full Brightness
			}
		}
		// Display the HSV debugging image
		IplImage *imageDisplayHSV_RGB = cvCreateImage(cvGetSize(imageDisplayHSV), 8, 3);
		cvCvtColor(imageDisplayHSV, imageDisplayHSV_RGB, CV_HSV2BGR);	// (note that OpenCV stores RGB images in B,G,R order.
		cvNamedWindow("Colors", 1);
		cvShowImage("Colors", imageDisplayHSV_RGB);
	}
	// SHOW_DEBUG_IMAGE


	// First, search for all the frontal faces in the image
	CvRect foundFace = cvRect(0, 0, 0, 0);	// Set init values if nothing was detected.
	vector<CvRect> rectFaces;
	double timeFaceDetectStart = (double)cvGetTickCount();	// Record the timing.
	rectFaces = findObjectsInImage(imageIn, cascadeFace);
	double tallyFaceDetectTime = (double)cvGetTickCount() - timeFaceDetectStart;
	cout << "Found " << rectFaces.size() << " faces in " << tallyFaceDetectTime/((double)cvGetTickFrequency()*1000.) << "ms\n";

	// Process each detected face
	cout << "Detecting shirt colors below the faces." << endl;
	for (int r=0; r<rectFaces.size(); r++) {
		float initialConfidence = 1.0f;
		int bottom;
		CvRect rectFace = rectFaces[r];
		drawRectangle(imageDisplay, rectFace, CV_RGB(255,0,0));

		// Create the shirt region, to be below the detected face and of similar size.
		float SHIRT_DY = 1.5f;	// Distance from top of face to top of shirt region, based on detected face height.
		float SHIRT_SCALE_X = 0.6f;	// Width of shirt region compared to the detected face
		float SHIRT_SCALE_Y = 0.6f;	// Height of shirt region compared to the detected face
		CvRect rectShirt;
		rectShirt.x = rectFace.x + (int)(0.5f * (1.0f-SHIRT_SCALE_X) * (float)rectFace.width);
		rectShirt.y = rectFace.y + (int)(SHIRT_DY * (float)rectFace.height) + (int)(0.5f * (1.0f-SHIRT_SCALE_Y) * (float)rectFace.height);
		rectShirt.width = (int)(SHIRT_SCALE_X * rectFace.width);
		rectShirt.height = (int)(SHIRT_SCALE_Y * rectFace.height);
		cout << "Shirt region is from " << rectShirt.x << ", " << rectShirt.y << " to " << rectShirt.x + rectShirt.width - 1 << ", " << rectShirt.y + rectShirt.height - 1 << endl;

		// If the shirt region goes partly below the image, try just a little below the face
		bottom = rectShirt.y+rectShirt.height-1;
		if (bottom > imageIn->height-1) {
			SHIRT_DY = 0.95f;	// Distance from top of face to top of shirt region, based on detected face height.
			SHIRT_SCALE_Y = 0.3f;	// Height of shirt region compared to the detected face
			// Use a higher shirt region
			rectShirt.y = rectFace.y + (int)(SHIRT_DY * (float)rectFace.height) + (int)(0.5f * (1.0f-SHIRT_SCALE_Y) * (float)rectFace.height);
			rectShirt.height = (int)(SHIRT_SCALE_Y * rectFace.height);
			initialConfidence = initialConfidence * 0.5f;	// Since we are using a smaller region, we are less confident about the results now.
			cout << "Warning: Shirt region goes past the end of the image. Trying to reduce the shirt region position to " << rectShirt.y << " with a height of " << rectShirt.height << endl;
		}

		// Try once again if it is partly below the image.
		bottom = rectShirt.y+rectShirt.height-1;
		if (bottom > imageIn->height-1) {
			bottom = imageIn->height-1;	// Limit the bottom
			rectShirt.height = bottom - (rectShirt.y-1);	// Adjust the height to use the new bottom
			initialConfidence = initialConfidence * 0.7f;	// Since we are using a smaller region, we are less confident about the results now.
			cout << "Warning: Shirt region still goes past the end of the image. Trying to reduce the shirt region height to " << rectShirt.height << endl;
		}

		// Make sure the shirt region is in the image
		if (rectShirt.height <= 1) {
			cout << "Warning: Shirt region is not in the image at all, so skipping this face." << endl;
		}
		else {

			// Show the shirt region
			drawRectangle(imageDisplay, rectShirt, text_color);

			// Convert the shirt region from RGB colors to HSV colors
			//cout << "Converting shirt region to HSV" << endl;
			IplImage *imageShirt = cropRectangle(imageIn, rectShirt);
			IplImage *imageShirtHSV = cvCreateImage(cvGetSize(imageShirt), 8, 3);
			cvCvtColor(imageShirt, imageShirtHSV, CV_BGR2HSV);	// (note that OpenCV stores RGB images in B,G,R order.
			if( !imageShirtHSV ) {
				cerr << "ERROR: Couldn't convert Shirt image from BGR2HSV." << endl;
				exit(1);
			}

			//cout << "Determining color type of the shirt" << endl;
			int h = imageShirtHSV->height;				// Pixel height
			int w = imageShirtHSV->width;				// Pixel width
			int tallyColors[NUM_COLOR_TYPES];
			for (int i=0; i<NUM_COLOR_TYPES; i++)
				tallyColors[i] = 0;
			// Scan the shirt image to find the tally of pixel colors
			for (int y=0; y<h; y++) {
				for (int x=0; x<w; x++) {
					CvScalar s =cvGetAt(imageShirtHSV,y,x);
					// Determine what type of color the HSV pixel is.
					int ctype = getPixelColorType(s.val[0], s.val[1], s.val[2]);
					// Keep count of these colors.
					tallyColors[ctype]++;
				}
			}

			// Print a report about color types, and find the max tally
			//cout << "Number of pixels found using each color type (out of " << (w*h) << ":\n";
			int tallyMaxIndex = 0;
			int tallyMaxCount = -1;
			int pixels = w * h;
			for (int i=0; i<NUM_COLOR_TYPES; i++) {
				int v = tallyColors[i];
				cout << sCTypes[i] << " " << (v*100/pixels) << "%, ";
				if (v > tallyMaxCount) {
					tallyMaxCount = tallyColors[i];
					tallyMaxIndex = i;
				}
			}
			cout << endl;
			int percentage = initialConfidence * (tallyMaxCount * 100 / pixels);
			cout << "Color of shirt: " << sCTypes[tallyMaxIndex] << " (" << percentage << "% confidence)." << endl << endl;
			// Display the color type over the shirt in the image.
			CvFont font;
			cvInitFont(&font,CV_FONT_HERSHEY_PLAIN,0.8,1.0, 0,1, CV_AA);	// For OpenCV 2.0
			char text[256];
			sprintf_s(text, sizeof(text)-1, "%d%%", percentage);		
			cvPutText(imageDisplay, sCTypes[tallyMaxIndex], cvPoint(rectShirt.x, rectShirt.y + rectShirt.height+12), &font, text_color);
			cvPutText(imageDisplay, text, cvPoint(rectShirt.x, rectShirt.y + rectShirt.height +24), &font,text_color);
			// Free resources.
			cvReleaseImage( &imageShirtHSV );
			cvReleaseImage( &imageShirt );
		}//end if valid height
	}//end for loop
	// Display the RGB debugging image
    cvShowImage("DisplayImage", imageDisplay);
	}*/
	imageAnalysis.processImage(file_name);
}

void CGUI_IMAGEDlg::OnBnClickedCheck1()
{
	
	// TODO: Add your control notification handler code here
	UINT nCheck = CheckBox.GetCheck();
    if (nCheck == BST_CHECKED)
    {
        is_show_debug = true;
    }
    else
    {
        is_show_debug = false;
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
	UserGui.setupTraining();
}