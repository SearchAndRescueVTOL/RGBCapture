#include "../include/image_utils.hpp"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;
using namespace std;

using namespace Pylon;

bool checkAndPrepareSaveDir(const string &path) {
  fs::path dir(path);
  if (!fs::exists(dir) || !fs::is_directory(dir)) {
    std::cerr << "ERROR: Save path does not exist or is not a directory: "
              << path << std::endl;
    return false;
  }
  return true;
}

std::string getTimestampedFilename() {
  auto now = std::chrono::system_clock::now();
  std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm *parts = std::localtime(&now_time);

  std::ostringstream ss;
  ss << (1900 + parts->tm_year) << "-" << std::setw(2) << std::setfill('0')
     << (1 + parts->tm_mon) << "-" << std::setw(2) << std::setfill('0')
     << parts->tm_mday << "_" << std::setw(2) << std::setfill('0')
     << parts->tm_hour << std::setw(2) << std::setfill('0') << parts->tm_min
     << std::setw(2) << std::setfill('0') << parts->tm_sec << ".png";
  return ss.str();
}

bool ensureFreeSpaceOrDeleteOldest(const std::string &dir,
                                   uintmax_t minFreeBytes) {
  auto spaceInfo = fs::space(dir);
  if (spaceInfo.available >= minFreeBytes) {
    return true;
  }

  std::vector<fs::directory_entry> images;
  for (const auto &entry : fs::directory_iterator(dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".png") {
      images.push_back(entry);
    }
  }

  if (images.empty()) {
    std::cerr << "WARNING: Not enough space and no images to delete."
              << std::endl;
    return false;
  }

  std::sort(images.begin(), images.end(),
            [](const fs::directory_entry &a, const fs::directory_entry &b) {
              return fs::last_write_time(a) < fs::last_write_time(b);
            });

  fs::remove(images.front());
  std::cout << "Deleted oldest image: " << images.front().path() << std::endl;
  return true;
}

bool saveImage(const std::string &dir, const cv::Mat &img) {
  std::string filename = dir + "/" + getTimestampedFilename();
  return cv::imwrite(filename, img);
}

cv::Mat grabResultToMat(const CGrabResultPtr &grabResult) {
  CImageFormatConverter formatConverter;
  formatConverter.OutputPixelFormat = PixelType_BGR8packed;

  CPylonImage pylonImage;
  formatConverter.Convert(pylonImage, grabResult);

  return cv::Mat(grabResult->GetHeight(), grabResult->GetWidth(), CV_8UC3,
                 (uint8_t *)pylonImage.GetBuffer());
}

void saveImage(const CGrabResultPtr &grabResult, const std::string &filename) {
  cv::Mat img = grabResultToMat(grabResult);
  cv::imwrite(filename, img);
}
