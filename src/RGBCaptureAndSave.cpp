#include <pylon/PylonIncludes.h>
#include <iostream>
#include "image_utils.hpp"

int main()
{
    Pylon::PylonInitialize();

    try {
        // Create and open the first available camera
        Pylon::CInstantCamera camera(Pylon::CTlFactory::GetInstance().CreateFirstDevice());
        camera.Open();

        std::cout << "Using device: " << camera.GetDeviceInfo().GetModelName() << std::endl;

        // Start grabbing
        camera.StartGrabbing(1); // Only grab one frame
        Pylon::CGrabResultPtr ptrGrabResult;

        camera.RetrieveResult(5000, ptrGrabResult, Pylon::TimeoutHandling_ThrowException);

        if (ptrGrabResult->GrabSucceeded()) {
            std::cout << "Image grabbed. Saving to file..." << std::endl;
            saveImage(*ptrGrabResult, "captured_image.png");
            std::cout << "Image saved as captured_image.png" << std::endl;
        } else {
            std::cerr << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
        }

    } catch (const Pylon::GenericException& e) {
        std::cerr << "An exception occurred: " << e.GetDescription() << std::endl;
        return 1;
    }

    Pylon::PylonTerminate();
    return 0;
}
