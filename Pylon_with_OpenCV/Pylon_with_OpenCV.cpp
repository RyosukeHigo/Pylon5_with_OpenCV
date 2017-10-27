// Pylon_with_OpenCV.cpp

/*
	Note: Before getting started, Basler recommends reading the Programmer's Guide topic
	in the pylon C++ API documentation that gets installed with pylon.
	If you are upgrading to a higher major version of pylon, Basler also
	strongly recommends reading the Migration topic in the pylon C++ API documentation.

	This sample illustrates how to grab and process images using the CInstantCamera class and OpenCV.
	The images are grabbed and processed asynchronously, i.e.,
	while the application is processing a buffer, the acquisition of the next buffer is done
	in parallel.

	OpenCV is used to demonstrate an image display, an image saving and a video recording.

	The CInstantCamera class uses a pool of buffers to retrieve image data
	from the camera device. Once a buffer is filled and ready,
	the buffer can be retrieved from the camera object for processing. The buffer
	and additional image data are collected in a grab result. The grab result is
	held by a smart pointer after retrieval. The buffer is automatically reused
	when explicitly released or when the smart pointer object is destroyed.
*/

#include "stdafx.h"

// Define if images are to be saved.
// '0'- no; '1'- yes.
#define saveImages 1
// Define if video is to be recorded.
// '0'- no; '1'- yes.
#define recordVideo 1

// Include files to use OpenCV API.
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#include "Pylon_with_OpenCV.h"
#endif

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using OpenCV objects.
using namespace cv;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 100;

// 画像処理
Mat trackImg(Mat img) {
	Mat hsv_img;
	Mat mask;
	GaussianBlur(img, img, Size(5, 5),0);
	cvtColor(img, hsv_img, CV_BGR2HSV);
	inRange(hsv_img, Scalar(0, 100,30), Scalar(5,255,255), mask);	// 色検出でマスク画像の作成
	//Moments mu = moments(img, false);
	//Point2f mc = Point2f(mu.m10 / mu.m00, mu.m01 / mu.m00);
	//circle(img, mc, 4, Scalar(100), 2, 4);
	return mask;
}
int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
	// is initialized during the lifetime of this object.
	Pylon::PylonAutoInitTerm autoInitTerm;

	try
	{
		// Create an instant camera object with the camera device found first.
		CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetVendorName() << " " << camera.GetDeviceInfo().GetModelName() << endl;

		// Get a camera nodemap in order to access camera parameters.
		GenApi::INodeMap& nodemap = camera.GetNodeMap();
		// Open the camera before accessing any parameters.
		camera.Open();
		// Create pointers to access the camera Width and Height parameters.
		GenApi::CIntegerPtr width = nodemap.GetNode("Width");
		GenApi::CIntegerPtr height = nodemap.GetNode("Height");

		// The parameter MaxNumBuffer can be used to control the count of buffers
		// allocated for grabbing. The default value of this parameter is 10.
		camera.MaxNumBuffer = 10;

		// Create a pylon ImageFormatConverter object.
		CImageFormatConverter formatConverter;
		// Specify the output pixel format.
		formatConverter.OutputPixelFormat = PixelType_BGR8packed;
		// Create a PylonImage that will be used to create OpenCV images later.
		CPylonImage pylonImage;
		// Declare an integer variable to count the number of grabbed images
		// and create image file names with ascending number.
		int grabbedImages = 0;

		// Create an OpenCV video creator.
		VideoWriter cvVideoCreator;
		// Create an OpenCV image.
		Mat openCvImage;

		// Define the video file name.
		std::string videoFileName = "openCvVideo.avi";

		// Define the video frame size.
		cv::Size frameSize = Size((int)width->GetValue(), (int)height->GetValue());

		// Set the codec type and the frame rate. You have 3 codec options here.
		// The frame rate should match or be lower than the camera acquisition frame rate.
		cvVideoCreator.open(videoFileName, CV_FOURCC('D', 'I', 'V', 'X'), 30, frameSize, true);
		//cvVideoCreator.open(videoFileName, CV_FOURCC('M','P','4','2'), 20, frameSize, true); 
		//cvVideoCreator.open(videoFileName, CV_FOURCC('M','J','P','G'), 20, frameSize, true);

		// Start the grabbing of c_countOfImagesToGrab images.
		// The camera device is parameterized with a default configuration which
		// sets up free-running continuous acquisition.
		camera.StartGrabbing(c_countOfImagesToGrab, GrabStrategy_LatestImageOnly);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
		// when c_countOfImagesToGrab images have been retrieved.
		while (camera.IsGrabbing())
		{
			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded())
			{
				// Access the image data.
				cout << "SizeX: " << ptrGrabResult->GetWidth() << endl;
				cout << "SizeY: " << ptrGrabResult->GetHeight() << endl;

				// Convert the grabbed buffer to a pylon image.
				formatConverter.Convert(pylonImage, ptrGrabResult);

				// Create an OpenCV image from a pylon image.
				openCvImage = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t *)pylonImage.GetBuffer());

				// Set saveImages to '1' to save images.
				if (saveImages) {
					// Create the current image name for saving.
					std::ostringstream s;
					// Create image name files with ascending grabbed image numbers.
					s << "image_" << grabbedImages << ".jpg";
					std::string imageName(s.str());
					// Save an OpenCV image.
					imwrite(imageName, openCvImage);
					grabbedImages++;
				}

				// Set recordVideo to '1' to record AVI video file.
				if (recordVideo)
					cvVideoCreator.write(openCvImage);

				// Create an OpenCV display window.
				namedWindow("OpenCV Display Window", CV_WINDOW_NORMAL); // other options: CV_AUTOSIZE, CV_FREERATIO


				//画像処理
				Mat gray_img;
				gray_img = trackImg(openCvImage);

				// Display the current image in the OpenCV display window.
				imshow("OpenCV Display Window", gray_img);
				// Define a timeout for customer's input in ms.
				// '0' means indefinite, i.e. the next image will be displayed after closing the window. 
				// '1' means live stream
				waitKey(1);

#ifdef PYLON_WIN_BUILD
				// Display the grabbed image in pylon.
				Pylon::DisplayImage(1, ptrGrabResult);
#endif
			}
			else
			{
				cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}

		// Release the video file on leaving.
		if (recordVideo)
			cvVideoCreator.release();
	}
	catch (GenICam::GenericException &e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	return exitCode;
}
