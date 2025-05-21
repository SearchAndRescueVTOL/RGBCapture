// save_triggered_frames.cpp

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip> // for std::setfill, std::setw
#include <iostream>
#include <opencv2/opencv.hpp>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <pylon/PylonIncludes.h>
#include <sstream> // for std::ostringstream
using namespace Pylon;
using namespace GenApi;
using namespace std;
using namespace Basler_UniversalCameraParams;
std::string getFormattedTimestamp() {
  using namespace std::chrono;
  auto now = system_clock::now();
  std::time_t now_time_t = system_clock::to_time_t(now);
  std::tm now_tm = *std::localtime(&now_time_t);
  auto duration = now.time_since_epoch();
  auto millis = duration_cast<milliseconds>(duration).count() % 1000;
  int hundredths = millis / 10;
  std::ostringstream oss;
  oss << std::setw(2) << std::setfill('0') << now_tm.tm_mon + 1 << "_"
      << std::setw(2) << std::setfill('0') << now_tm.tm_mday << "_"
      << now_tm.tm_year + 1900 << "_" << std::setw(2) << std::setfill('0')
      << now_tm.tm_hour << "_" << std::setw(2) << std::setfill('0')
      << now_tm.tm_min << "_" << std::setw(2) << std::setfill('0')
      << now_tm.tm_sec << "_" << std::setw(2) << std::setfill('0')
      << hundredths;

  return oss.str();
}

int main() {
  // Initialize Pylon runtime before using any Pylon methods
  PylonInitialize();
  int exitCode = 0;
  string time = getFormattedTimestamp();
  string SAVE_DIR = "/mnt/external/RGB/" + time;
  if (!std::filesystem::exists(SAVE_DIR)) {
    if (!std::filesystem::create_directory(SAVE_DIR)) {
      // std::cout << "Directory created: " << SAVE_DIR << std::endl;
      cerr << "Failed to create directory!" << endl;
      PylonTerminate();
      return 1;
    }
  }
  string logFileName = "logs/" + time + ".txt";
  std::ofstream logfile(logFileName, std::ios::app);
  if (!logfile) {
    std::cerr << "Failed to open or create file: " << logFileName << std::endl;
    return 1;
  }

  try {
    // Create an instant camera object with the first found device
    CBaslerUniversalInstantCamera camera(
        CTlFactory::GetInstance().CreateFirstDevice());

    // Open the camera
    camera.Open();

    // --- Trigger configuration ---
    camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
    camera.TriggerMode.SetValue(TriggerMode_On);
    camera.TriggerSource.SetValue(
        TriggerSource_Line3); // use Line3 for external trigger
    camera.TriggerActivation.SetValue(TriggerActivation_RisingEdge);
    // --------------------------------

    // Continuous acquisition mode
    camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
    // Camera Parameters (gain/exposure)
    double gainLowerLimit = 0.0;
    double gainUpperLimit = 6.54;
    double exposureLowerLimit = 10.0;
    double exposureUpperLimit = 100000;
    camera.AutoExposureTimeLowerLimit.SetValue(exposureLowerLimit);
    camera.AutoExposureTimeUpperLimit.SetValue(exposureUpperLimit);
    camera.AutoTargetBrightness.SetValue(0.5);
    camera.ExposureAuto.SetValue(ExposureAuto_Continuous);
    camera.AutoGainLowerLimit.SetValue(gainLowerLimit);
    camera.AutoGainUpperLimit.SetValue(gainUpperLimit);
    camera.GainAuto.SetValue(GainAuto_Continuous);
    camera.AutoFunctionROISelector.SetValue(AutoFunctionROISelector_ROI1);
    camera.AutoFunctionROIUseBrightness.SetValue(true);
    // Auto target brightness 0.5
    // Auto function minimize gain
    // Gain lower limit 0
    // Gain upper limit 6.54
    // Exposure time lower limit 10
    // Exposure time upper limit 100000

    // End camera parameters

    // Use the LatestImageOnly strategy so we always get the newest frame
    camera.StartGrabbing(GrabStrategy_LatestImageOnly);

    cout
        << "Waiting for hardware trigger on Line3. Saving each frame as TIFF..."
        << endl;

    CGrabResultPtr ptrGrabResult;
    int frameIndex = 0;

    // Grab loop
    while (camera.IsGrabbing()) {
      // Wait up to 5000 ms for a trigger+image
      camera.RetrieveResult(5000, ptrGrabResult,
                            TimeoutHandling_ThrowException);

      if (ptrGrabResult->GrabSucceeded()) {
        // Get image dimensions and buffer pointer
        int width = ptrGrabResult->GetWidth();
        int height = ptrGrabResult->GetHeight();
        const uint8_t *buffer =
            reinterpret_cast<const uint8_t *>(ptrGrabResult->GetBuffer());

        // Wrap it in an OpenCV Mat (Mono8 or Bayer8)
        cv::Mat img(height, width, CV_8UC1, const_cast<uint8_t *>(buffer));

        // If Bayer sensor, convert to RGB:
        cv::cvtColor(img, img, cv::COLOR_BayerRG2RGB);

        // Build filename "frame000.tiff", "frame001.tiff", …
        // auto now = chrono::system_clock::now();
        // time_t now_c = chrono::s/MicroXRCE/////ystem_clock::to_time_t(now);
        // tm* now_tm = localtime(&now_c);
        ostringstream ss;
        ss << SAVE_DIR << "/" << getFormattedTimestamp() << "_rgb_#"
           << frameIndex++ << ".tiff";

        string filename = ss.str();

        // Save to TIFF
        if (!cv::imwrite(filename, img)) {
          cerr << "ERROR: Could not write image to " << filename << endl;
          break;
        }
        cout << "Saved " << filename << endl;
        logfile << "Saved " << filename << endl;
        // If you want only the first frame, uncomment:
        // break;
      } else {
        cerr << "ERROR: Grab failed (" << ptrGrabResult->GetErrorCode()
             << "): " << ptrGrabResult->GetErrorDescription() << endl;
      }
    }

    // Stop grabbing and clean up
    camera.StopGrabbing();
    camera.Close();
  } catch (const GenericException &e) {
    // Error handling
    cerr << "An exception occurred: " << e.GetDescription() << endl;
    exitCode = 1;
  }
  logfile.close();
  // Release Pylon resources
  PylonTerminate();
  return exitCode;
}
