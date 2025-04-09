#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <opencv2/opencv.hpp>

// Include files used by samples.
#include "./include/ConfigurationEventPrinter.h"
#include "./include/ImageEventPrinter.h"

using namespace Pylon;
using namespace GenApi;
using namespace std;
using namespace Basler_UniversalCameraParams;


int main()
{
    PylonInitialize();
    int exitCode = 0;

    try
    {
	Pylon::CBaslerUniversalInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());


        camera.Open();

        // Configure for external hardware trigger on Line1
        camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
        camera.TriggerMode.SetValue(TriggerMode_On);
        camera.TriggerSource.SetValue(TriggerSource_Line3);
        camera.TriggerActivation.SetValue(TriggerActivation_RisingEdge);

        // Continuous acquisition mode
        camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);

        // Use LatestImageOnly strategy to always get the newest image
        camera.StartGrabbing(GrabStrategy_LatestImageOnly);

        CGrabResultPtr ptrGrabResult;
        cout << "Press ESC to exit..." << endl;


	cv::namedWindow("Trigger Feed", cv::WINDOW_NORMAL);
	cv::resizeWindow("Trigger Feed", 1920, 1080);

        while (camera.IsGrabbing())
        {
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

            if (ptrGrabResult->GrabSucceeded())
            {
                // Convert to OpenCV Mat (assuming Mono8 or Bayer8)
                const uint8_t* pImageBuffer = reinterpret_cast<const uint8_t*>(ptrGrabResult->GetBuffer());
                int width = ptrGrabResult->GetWidth();
                int height = ptrGrabResult->GetHeight();

                cv::Mat img(height, width, CV_8UC1, const_cast<uint8_t*>(pImageBuffer));

                // For Bayer images: uncomment if needed
                cv::cvtColor(img, img, cv::COLOR_BayerRG2RGB);

                cv::imshow("Trigger Feed", img);
                if (cv::waitKey(1) == 27) // ESC to exit
                    break;
            }
            else
            {
                cerr << "Grab failed: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }

        camera.StopGrabbing();
        camera.Close();
    }
    catch (const GenericException& e)
    {
        cerr << "An exception occurred: " << e.GetDescription() << endl;
        exitCode = 1;
    }

    PylonTerminate();
    return exitCode;
}

