// save_triggered_frames_mt_multi_consumer.cpp

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <opencv2/opencv.hpp>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <filesystem>
#include <thread>
#include <atomic>
#include <queue>
#include <vector>
#include <optional>
#include <pthread.h>
#include <sched.h>
#include <cstring>
#include <cerrno>
using namespace Pylon;
using namespace Basler_UniversalCameraParams;
using namespace GenApi;
using namespace std;
constexpr size_t QUEUE_CAPACITY = 128;
constexpr size_t LOG_QUEUE_SIZE = 512;
struct ImageItem {
    cv::Mat image;
    int trigger_number;
    std::string imName;
};

struct Slot {
    std::atomic<bool> flag;
    std::string value;
};

class LogQueue {
public:
    LogQueue() : head(0), tail(0) {
        for (size_t i = 0; i < LOG_QUEUE_SIZE; ++i) {
            slots[i].flag.store(false, std::memory_order_relaxed);
        }
    }

    bool push(const std::string& message) {
        size_t index = tail.fetch_add(1, std::memory_order_relaxed) % LOG_QUEUE_SIZE;
        while (slots[index].flag.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        slots[index].value = message;
        slots[index].flag.store(true, std::memory_order_release);
        return true;
    }

    std::optional<std::string> pop() {
        size_t index = head.load(std::memory_order_relaxed) % LOG_QUEUE_SIZE;
        if (!slots[index].flag.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        std::string result = std::move(slots[index].value);
        slots[index].flag.store(false, std::memory_order_release);
        head.fetch_add(1, std::memory_order_relaxed);

        return result;
    }

    bool empty() const {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }

private:
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
    Slot slots[LOG_QUEUE_SIZE];
};





class ImageQueue {
public:
    ImageQueue() : head(0), tail(0) {}

    bool push(const ImageItem& item) {
        size_t current_tail = tail.load(std::memory_order_relaxed);
        size_t next_tail = increment(current_tail);

        if (next_tail == head.load(std::memory_order_acquire)) {
            return false;
        }

        buffer[current_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    std::optional<ImageItem> pop() {
        size_t current_head;

        while (true) {
            current_head = head.load(std::memory_order_acquire);

            if (current_head == tail.load(std::memory_order_acquire)) {
                return std::nullopt;
            }

            size_t next_head = increment(current_head);
            if (head.compare_exchange_weak(current_head, next_head, std::memory_order_acquire, std::memory_order_relaxed)) {
                return buffer[current_head];
            }
        }
    }

    bool empty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

private:
    size_t increment(size_t idx) const {
        return (idx + 1) % QUEUE_CAPACITY;
    }

    std::atomic<size_t> head;
    std::atomic<size_t> tail;
    ImageItem buffer[QUEUE_CAPACITY];
};


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
void writerThreadFunc(ImageQueue& queue, const std::string& saveDir, std::atomic<bool>& done, LogQueue& logQueue) {
    while (!done.load() || !queue.empty()) {
        auto item_opt = queue.pop();
        if (item_opt) {
            auto& item = *item_opt;
            if (!cv::imwrite(item.imName, item.image)) {
                logQueue.push("Failed to write image");
            } else {
                logQueue.push("Saved: " + item.imName);
            }
        } else {
            std::this_thread::yield();
        }
    }
}

void loggerThreadFunc(LogQueue& logQueue, std::ofstream& logfile, std::atomic<bool>& done) {
    while (true) {
        auto msg = logQueue.pop();
        if (msg) {
            logfile << *msg << std::endl;
            cout << *msg << std::endl;
        } else {
            if (done.load() && logQueue.empty()) {
                break;
            }
            std::this_thread::yield();
        }
    }
    logfile.close();
}




int main() {
  // Initialize Pylon runtime before using any Pylon methods



    PylonInitialize();
    int exitCode = 0;
    string time = getFormattedTimestamp();
    string SAVE_DIR = "/home/sarv-pi/RGB/" + time;
    if (!std::filesystem::exists(SAVE_DIR)) {
        if (!std::filesystem::create_directory(SAVE_DIR)) {
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
  atomic<bool> done{false};
  std::thread logger;
  std::vector<std::thread> writers;
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
    ImageQueue imageQueue;
    LogQueue logQueue;

    // Auto target brightness 0.5
    // Auto function minimize gain
    // Gain lower limit 0
    // Gain upper limit 6.54
    // Exposure time lower limit 10
    // Exposure time upper limit 100000

    // End camera parameters

    // Use the LatestImageOnly strategy so we always get the newest frame
    camera.StartGrabbing(GrabStrategy_LatestImageOnly);

    cout << "Waiting for hardware trigger on Line3. Saving each frame as TIFF..."
        << endl;
    logger = std::thread(loggerThreadFunc, ref(logQueue), ref(logfile), ref(done));
    for(int i=0; i < 3; i++){
        writers.emplace_back(std::thread(writerThreadFunc, ref(imageQueue), SAVE_DIR, ref(done), ref(logQueue)));
    }
    pthread_t main_thread = pthread_self();
    int policy = SCHED_FIFO;
    sched_param sch_params; 
    sch_params.sched_priority = 25;
    if (pthread_setschedparam(main_thread, policy, &sch_params) != 0) {
        std::cerr << "Failed to set main thread priority: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Main thread priority set to max (" << sch_params.sched_priority << ") under policy SCHED_RR" << std::endl;
    }
    // std::thread writer1(writerThreadFunc, ref(imageQueue), SAVE_DIR, ref(done), ref(logQueue));
    // std::thread writer2(writerThreadFunc, ref(imageQueue), SAVE_DIR, ref(done), ref(logQueue));
    
    CGrabResultPtr ptrGrabResult;
    int frameIndex = 0;
    while (camera.IsGrabbing()) {
        camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);
        if (ptrGrabResult->GrabSucceeded()) {
            cv::Mat img(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC1, const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(ptrGrabResult->GetBuffer())));
            cv::cvtColor(img, img, cv::COLOR_BayerRG2RGB);
            ImageItem item;
            item.image = img.clone();
            item.trigger_number = frameIndex++;
            std::ostringstream ss;
            ss << SAVE_DIR << "/" << getFormattedTimestamp() << "_rgb_#" << item.trigger_number << ".tiff";
            item.imName = ss.str();
            while (!imageQueue.push(item)) {
                std::this_thread::yield(); 
            }
        }
        else{
          cerr << "ERROR: Grab failed (" << ptrGrabResult->GetErrorCode()
             << "): " << ptrGrabResult->GetErrorDescription() << endl;
        }
    }
    done.store(true);
    for (auto& t : writers) {
        if (t.joinable()) {
            t.join();
        }
    }
    if (logger.joinable()){
        logger.join();
    }
    camera.StopGrabbing();
    camera.Close();
    } catch (const GenericException &e) {
        // Error handling
        cerr << "An exception occurred: " << e.GetDescription() << endl;
        exitCode = 1;
    }
    done.store(true);
    for (auto& t : writers) {
        if (t.joinable()) {
            t.join();
        }
    }
    if (logger.joinable()){
        logger.join();
    }
    logfile.close();
    PylonTerminate();
    return exitCode;
}

