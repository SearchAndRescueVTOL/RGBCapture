// save_triggered_frames.cpp

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <opencv2/opencv.hpp>
#include <iomanip>    // for std::setfill, std::setw
#include <sstream>    // for std::ostringstream
extern "C"{
#include <mosquitto.h>
}
struct mosquitto *mosq;
int rc;
#define MQTT_HOST "localhost"
#define MQTT_PORT 1883
#define MQTT_TOPIC "RGB/logs"
using namespace Pylon;
using namespace GenApi;
using namespace std;
using namespace Basler_UniversalCameraParams;
int log_to_mosq(char *msg){
	rc = mosquitto_publish(mosq, NULL, MQTT_TOPIC, strlen(msg), msg, 0, false);
	if (rc != MOSQ_ERR_SUCCESS){
		cout << "Failed to log over mosquitto!\n" << endl;
		return -1;
	}
	else{
		return 0;
	}
}
int main()
{
    // Initialize Pylon runtime before using any Pylon methods
    PylonInitialize();
    int exitCode = 0;

    try
    {
        // Create an instant camera object with the first found device
        CBaslerUniversalInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice() );

        // Open the camera
        camera.Open();

        // --- Trigger configuration ---
        camera.TriggerSelector.SetValue( TriggerSelector_FrameStart );
        camera.TriggerMode.SetValue( TriggerMode_On );
        camera.TriggerSource.SetValue( TriggerSource_Line3 );          // use Line3 for external trigger
        camera.TriggerActivation.SetValue( TriggerActivation_RisingEdge );
        // --------------------------------

        // Continuous acquisition mode
        camera.AcquisitionMode.SetValue( AcquisitionMode_Continuous );

        // Use the LatestImageOnly strategy so we always get the newest frame
        camera.StartGrabbing( GrabStrategy_LatestImageOnly );

        cout << "Waiting for hardware trigger on Line3. Saving each frame as TIFF..." << endl;

        CGrabResultPtr ptrGrabResult;
        int frameIndex = 0;

        // Grab loop
        while ( camera.IsGrabbing() )
        {
            // Wait up to 5000 ms for a trigger+image
            camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

            if ( ptrGrabResult->GrabSucceeded() )
            {
                // Get image dimensions and buffer pointer
                int width  = ptrGrabResult->GetWidth();
                int height = ptrGrabResult->GetHeight();
                const uint8_t* buffer = reinterpret_cast<const uint8_t*>( ptrGrabResult->GetBuffer() );

                // Wrap it in an OpenCV Mat (Mono8 or Bayer8)
                cv::Mat img( height, width, CV_8UC1, const_cast<uint8_t*>(buffer) );

                // If Bayer sensor, convert to RGB:
                cv::cvtColor(img, img, cv::COLOR_BayerRG2RGB);

                // Build filename "frame000.tiff", "frame001.tiff", …
                ostringstream ss;
		ss << "/mnt/external/RGB/";
                ss << "frame" << setfill('0') << setw(3) << frameIndex++ << ".tiff";
                string filename = ss.str();

                // Save to TIFF
                if ( !cv::imwrite( filename, img ) )
                {
                    cerr << "ERROR: Could not write image to " << filename << endl;
                    break;

                }
		string temp = "Saved " + filename;
		char * buff = new char[temp.size() + 1];
		strcpy(buff, temp.c_str());
		log_to_mosq(buff);
                cout << "Saved " << filename << endl;
		delete[] buff;
                // If you want only the first frame, uncomment:
                // break;
            }
            else
            {
                cerr << "ERROR: Grab failed ("
                     << ptrGrabResult->GetErrorCode() << "): "
                     << ptrGrabResult->GetErrorDescription() << endl;
            }
        }

        // Stop grabbing and clean up
        camera.StopGrabbing();
        camera.Close();
    }
    catch ( const GenericException& e )
    {
        // Error handling
        cerr << "An exception occurred: " << e.GetDescription() << endl;
        exitCode = 1;
    }

    // Release Pylon resources
    PylonTerminate();
    return exitCode;
}

