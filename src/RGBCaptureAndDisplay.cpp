#include "image_utils.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>

int main() {
  Pylon::PylonInitialize();

  try {
    // Create and open the first available camera
    Pylon::CInstantCamera camera(
        Pylon::CTlFactory::GetInstance().CreateFirstDevice());
    camera.Open();

    std::cout << "Using device: " << camera.GetDeviceInfo().GetModelName()
              << std::endl;

    // Start grabbing
    camera.StartGrabbing(1); // Grab one frame
    Pylon::CGrabResultPtr ptrGrabResult;

    camera.RetrieveResult(5000, ptrGrabResult,
                          Pylon::TimeoutHandling_ThrowException);

    if (ptrGrabResult->GrabSucceeded()) {
      std::cout << "Image grabbed. Displaying..." << std::endl;

      cv::Mat img = grabResultToMat(*ptrGrabResult);
      cv::imshow("RGB Image", img);
      cv::waitKey(0);
      cv::destroyAllWindows();

    } else {
      std::cerr << "Error: " << ptrGrabResult->GetErrorCode() << " "
                << ptrGrabResult->GetErrorDescription() << std::endl;
    }

  } catch (const Pylon::GenericException &e) {
    std::cerr << "An exception occurred: " << e.GetDescription() << std::endl;
    return 1;
  }

  Pylon::PylonTerminate();
  return 0;
}
