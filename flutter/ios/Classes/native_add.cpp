#include <chrono>
#include <fstream>
#include <iostream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <regex>
#include <cstdio>
#include <cstdlib>
#include <opencv.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#ifdef __ANDROID__
#include <android/log.h>

#endif

using namespace cv;
using namespace std;

extern "C" __attribute__((visibility("default"))) __attribute__((used)) void convertImageToGrayImage(char *inputImagePath, char *outputPath, char *tappoint, char *colorhex)

               {



                   // PARSE COORDINATES

                   //vector<std::pair<int, int>> coordinates = parse_coordinates(tappoint);

                   std::vector<std::pair<int, int>> coordinates;
                   std::string str = tappoint;
                   std::regex pattern(R"(\(([-+]?\d+),([-+]?\d+)\))");
                   std::smatch match;
                   std::string::const_iterator iter = str.cbegin();
                   while (std::regex_search(iter, str.cend(), match, pattern))
                   {
                       coordinates.emplace_back(std::stoi(match[1]), std::stoi(match[2]));
                       iter = match.suffix().first;
                   }

                   // FINDING THE DESIRED HSV COLOR CODE
                   Point p1(coordinates[0].first, coordinates[0].second); // todo: this will be read fromn txt file 350,520 for kapi change here

                   int r, g, b;
                   std::sscanf(colorhex, "#%02x%02x%02x", &r, &g, &b);


                   cv::Mat src = cv::Mat(1, 1, CV_8UC3, cv::Scalar(r, g, b));
                   cv::Mat desired;
                   cv::cvtColor(src, desired, cv::COLOR_RGB2HSV);
                   int desired_h = desired.at<cv::Vec3b>(0, 0)[0];
                   int desired_s = desired.at<cv::Vec3b>(0, 0)[1];
                   int desired_v = desired.at<cv::Vec3b>(0, 0)[2];

                   // DIMENSIONS OF INPUT IMAGE
                   cv::Mat img = cv::imread(inputImagePath);

                   // BGR -> HSV changing part
                   cv::Mat HSVImage;
                   cvtColor(img, HSVImage, cv::COLOR_BGR2HSV);
                   cvtColor(img, img, cv::COLOR_BGR2HSV);
                   // BLUR THE SEGMENTED IMAGE  todo maybe this place will come after segmentation
                   blur(HSVImage, HSVImage, Size(30, 30));

                   // COLOR SEGMENTATION
                   cv::Mat hsv = HSVImage.clone();

                   cv::Vec3b pixel = hsv.at<cv::Vec3b>(coordinates[0].second, coordinates[0].first);
                   int hue = pixel[0];
                   int min_sat = pixel[1];
                   int value = pixel[2];

                   int high_hue = std::max((hue - 20) % 180, (hue + 20) % 180);
                   int low_hue = std::min((hue - 20) % 180, (hue - 20) % 180);
                   if (high_hue == low_hue)
                   {
                       high_hue = 180;
                   }

                   if (low_hue < 0)
                   {
                       low_hue = 0;
                   }

                   int black;
                   int ultra_black = 0;
                   cv::Scalar minHSV,
                       maxHSV;
                   if (value < 80 && min_sat < 80)
                   {
                       ultra_black = 1;
                   }

                   if (value < 80)
                   { // black image

                       black = 1;

                       minHSV = cv::Scalar(0, min_sat - 60, value - 40); // bed
                       maxHSV = cv::Scalar(180, 255, value + 40);        // bed was +30
                   }

                   else if (min_sat < 80)
                   {
                       black = 0;
                       minHSV = cv::Scalar(0, min_sat - 60, value - 40); // bed
                       maxHSV = cv::Scalar(180, 255, value + 40);        // bed was +30
                   }
                   else
                   {
                       black = 0;

                       minHSV = cv::Scalar(low_hue, min_sat - 60, 0); // bed
                       maxHSV = cv::Scalar(high_hue, 255, 255);       // bed was +30
                   }


                   cv::Mat maskHSV, resultHSV;
                   cv::inRange(hsv, minHSV, maxHSV, maskHSV);
                   cv::bitwise_and(hsv, hsv, resultHSV, maskHSV);
                   cv::Mat segmented_img;
                   segmented_img = resultHSV.clone();

                   // FINDIND THE RELATED CONTOURS
                   int thresh = 20;
                   Mat segmented_img_gray;
                   cvtColor(segmented_img, segmented_img_gray, COLOR_BGR2GRAY);
                   threshold(segmented_img_gray, segmented_img_gray, thresh, 255, THRESH_BINARY);
                   vector<vector<Point>> contours;
                   vector<Vec4i> hierarchy;
                   findContours(segmented_img_gray, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);

                   Mat drawing = Mat::zeros(segmented_img_gray.size(), CV_8UC3);
                   int poi_cnt;
                   Scalar color = Scalar(255, 255, 255);
                   Scalar line_Color(0, 255, 0);

                   // FIND THE RELEVANT CONTOUR
                   for (size_t i = 0; i < contours.size(); i++)
                   {
                       int result_poly = pointPolygonTest(contours[i], p1, false);
                       // drawContours(drawing, contours, (int)i, color, -1, LINE_8, hierarchy, 0);
                       if (result_poly == 1)
                       {
                           drawContours(drawing, contours, (int)i, color, -1, LINE_8, hierarchy, 0);
                       }
                   }

                   // PIGMENTATION
                   Mat result, result2, background, final_image;

                   cvtColor(drawing, drawing, COLOR_BGR2GRAY);
                   bitwise_and(img, img, result, drawing);
                   cvtColor(result, result, COLOR_BGR2HSV);

                   vector<Mat> hsv_vec;
                   split(result, hsv_vec); // this is an opencv function

                   cv::Mat &h = hsv_vec[0];
                   cv::Mat &s = hsv_vec[1];
                   cv::Mat &v = hsv_vec[2];

                   double minVal;
                   double maxVal;
                   Point minLoc;
                   Point maxLoc;
                   minMaxLoc(s, &minVal, &maxVal, &minLoc, &maxLoc);

                   if (ultra_black)

                   {
                       h.setTo(desired_h);
                       s.setTo(desired_s);
                       // Increase the lightness for pixels that are not black
                       for (int i = 0; i < v.rows; i++)
                       {
                           for (int j = 0; j < v.cols; j++)
                           {
                               if (result.at<cv::Vec3b>(i, j) != cv::Vec3b(0, 0, 0))
                               {
                                   v.at<uchar>(i, j) = v.at<uchar>(i, j) + 150;
                               }
                           }
                       }
                   }
                   else if (black)
                   {
                       h.setTo(desired_h);
                       s.setTo(desired_s);
                       // Increase the lightness for pixels that are not black
                       for (int i = 0; i < v.rows; i++)
                       {
                           for (int j = 0; j < v.cols; j++)
                           {
                               if (result.at<cv::Vec3b>(i, j) != cv::Vec3b(0, 0, 0))
                               {
                                   v.at<uchar>(i, j) = v.at<uchar>(i, j) + 60;
                               }
                           }
                       }
                   }
                   else
                   {
                       h.setTo(desired_h);
                       s.setTo(desired_s);
                   }

                   // h.setTo(desired_h, v > 1);
                   // // s = 62;
                   // // s.setTo(142, v > 1);
                   // s = s / maxVal;
                   // // s = s+0.2;
                   // s = s * desired_s;
                   // v = v + 20;
                   merge(hsv_vec, result);

                   // BACKGROUND
                   cvtColor(result, result, COLOR_HSV2BGR);
                   cvtColor(result, result2, COLOR_BGR2GRAY);

                   threshold(result2, result2, 20, 255, THRESH_BINARY_INV);
                   bitwise_and(img, img, background, result2);
                   cvtColor(background, background, cv::COLOR_HSV2BGR); // or rgb

                   final_image = result + background;

                   // RETURNING
                   cvtColor(segmented_img, segmented_img, COLOR_HSV2BGR);
                   imwrite(outputPath, final_image); // then compare withy img
}
