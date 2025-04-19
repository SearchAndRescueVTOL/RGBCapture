#include <pylon/PylonIncludes.h>
#include <iostream>
#include "image_utils.hpp"
#include "log.h"

int main()
{
    /////////////
    FILE *logfile = fopen("RGBCaptureAndSaveLogOutput.txt", "a");  // "a" for append mode
    if (!logfile) {
        fprintf(stderr, "Failed to open log file\n");
        return 1;
    }
    log_add_fp(logfile, LOG_TRACE);  // Log everything (TRACE and above)
    log_info("RGBCaptureAndSave Program started");
    /////////////

    Pylon::PylonInitialize();

    try {
        // Create and open the first available camera
        Pylon::CInstantCamera camera(Pylon::CTlFactory::GetInstance().CreateFirstDevice());
        camera.Open();

        std::cout << "Using device: " << camera.GetDeviceInfo().GetModelName() << std::endl;
        log_trace("RGBCaptureAndSave Using Device");

        // Start grabbing
        camera.StartGrabbing(1); // Only grab one frame
        Pylon::CGrabResultPtr ptrGrabResult;

        camera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

        if (ptrGrabResult->GrabSucceeded()) {
            std::cout << "Image grabbed. Saving to file..." << std::endl;
            log_trace("RGBCaptureAndSave Image grabbed. Saving to file...");
            saveImage(*ptrGrabResult, "captured_image.png");
            std::cout << "Image saved as captured_image.png" << std::endl;
            log_trace("RGBCaptureAndSave Image saved as captured_image.png");

        } else {
            std::cerr << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
        }

    } catch (const Pylon::GenericException& e) {
        std::cerr << "An exception occurred: " << e.GetDescription() << std::endl;
        log_trace("RGBCaptureAndSave An exception occurred:");
        return 1;
    }

    Pylon::PylonTerminate();
    return 0;
}
