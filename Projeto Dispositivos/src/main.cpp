#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/video.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/ximgproc.hpp>

#include "ShipDetector.h"

int idx = 0;

void OnMouseCallback(int mouse_event, int x, int y, int flags, void *param) {
  if (mouse_event == CV_EVENT_LBUTTONDOWN) {
    std::vector<std::vector<cv::Point2f>> *ptr = static_cast<std::vector<std::vector<cv::Point2f>>*>(param);
    (*ptr)[idx].push_back(cv::Point2f(x, y));
  }
}

bool operator >=(const cv::Scalar &a, const cv::Scalar &b) {
  return a(0) >= b(0) && a(1) >= b(1) && a(2) >= b(2);
}

int main(int argc, char* argv[]) {
  cv::VideoCapture capture(0);

  if (!capture.isOpened()) {
    std::cout << "Couldn't find video device." << std::endl;
    system("pause");
    return -1;
  }

  cv::namedWindow("Video");
  cv::namedWindow("Binary");

  detection::ShipDetector detector;
  //std::array<cv::Rect, 2> ROIs;
  detection::CandidatesColors colors;
  bool start = false;
  int currFrame = 1;
  cv::VideoWriter outputVideo;

  /*ROIs[0] = cv::Rect(200, 320, 100, 100);
  ROIs[1] = cv::Rect(200, 20, 100, 100);*/

  colors[0] = cv::Vec3b(0, 255, 255);

  while (true) {
    cv::Mat frame, filtered;
    capture >> frame;
    filtered = cv::Mat::zeros(frame.size(), frame.type());
    cv::Ptr<cv::ximgproc::DTFilter> ptrFilter = cv::ximgproc::createDTFilter(frame, 30, 10);

    ptrFilter->filter(frame, filtered);
    cv::GaussianBlur(filtered, filtered, cv::Size(7, 7), 1.5, 1.5);

    if (!start && currFrame == 10) {
      detector.init(filtered);
      cv::Size frameSize(detector.getMapROI().width, detector.getMapROI().height);

      outputVideo.open("C:/Users/Mateus/Desktop/demo.avi", CV_FOURCC('M', 'J', 'P', 'G'),
                       30, frameSize);

      if (!outputVideo.isOpened()) {
        std::cout << "Bla" << std::endl;
        system("pause");
        return -1;
      }

      start = true;
    } else if (start) {
      filtered = filtered(detector.getMapROI());
      detection::MapNodes nodes = detector.getNodes();

      for (int i = 0; i < nodes.size(); ++i) {
        nodes[i].drawExternalNode(&filtered);
      }

      cv::Mat binImage = detector.thresholdImage(filtered, 5);
      cv::imshow("Binary", binImage);

      std::vector<std::vector<cv::Point2i>> contours;
      std::vector<cv::Vec4i> contoursHierarchy;

      cv::findContours(binImage, contours, contoursHierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE,
                       cv::Point2i(0, 0));
      detection::Candidates ships;
      int numShipsDetected = detector.findShipsBlobs(contours, &ships);

      /*for (int i = 0; i < ROIs.size(); ++i) {
        cv::rectangle(filtered, ROIs[i], cv::Scalar(255, 0, 0));
      }*/


      cv::Scalar thresholdInterval(30, 30, 30), color(64, 180, 204);
      cv::Scalar min = color - thresholdInterval;
      cv::Scalar max = color + thresholdInterval;

      for (int i = 0; i < numShipsDetected; ++i) {
        if (!contours[ships[i]].empty()) {
          cv::Moments shipMoments = cv::moments(contours[ships[i]]);
          cv::Point2i shipCentroid = cv::Point2i(cvRound(shipMoments.m10 / shipMoments.m00),
                                                 cvRound(shipMoments.m01 / shipMoments.m00));
          cv::Rect shipBounds = cv::boundingRect(contours[ships[i]]);
          cv::Scalar mean = cv::mean(filtered(shipBounds), binImage(shipBounds) != 0);

          for (int j = 0; j < nodes.size(); ++j) {
            if (nodes[j].contains(shipCentroid) && mean >= min && max >= mean) {
              std::cout << mean << std::endl;
              cv::rectangle(filtered, shipBounds, cv::Scalar(0, 255, 0));
              break;
            }
          }
        }
      }
    }

    cv::imshow("Video", filtered);
    //outputVideo.write(filtered);
    ++currFrame;

    if (cv::waitKey(30) == 27)
    break;
  }

  capture.release();
  //outputVideo.release();
  cv::destroyAllWindows();

  return 0;
}
