#pragma once

#include <opencv2/opencv.hpp>
#include <pylon/PylonIncludes.h>
#include <string>
using namespace std;

bool checkAndPrepareSaveDir(const string &path);
string getTimestampedFilename();
bool ensureFreeSpaceOrDeleteOldest(const string &dir, uintmax_t minFreeBytes);
bool saveImage(const string &dir, const cv::Mat &img);

// Convert Pylon image to OpenCV Mat
cv::Mat grabResultToMat(const Pylon::CGrabResultPtr &grabResult);

// Save image to disk
void saveImage(const Pylon::CGrabResultPtr &grabResult,
               const std::string &filename);
