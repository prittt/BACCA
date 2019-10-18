#include <opencv2/imgproc.hpp>

#include "chaincode_correct.h"
#include "register.h"

using namespace std;
using namespace cv;

void ChainCodeCorrect::PerformChainCode() {
    vector<vector<Point>> cv_contours;
    findContours(img_, cv_contours, RETR_LIST, CHAIN_APPROX_NONE);
    chain_code_ = ChainCode(cv_contours, true);
}

vector<vector<Point>> ChainCodeCorrect::FindContours() {
    vector<vector<Point>> cv_contours;
    findContours(img_, cv_contours, RETR_LIST, CHAIN_APPROX_NONE);
    return cv_contours;
}
void ChainCodeCorrect::ConvertToChainCode(const std::vector<std::vector<cv::Point>>& cv_contours) {
    chain_code_ = ChainCode(cv_contours, true);
}

void ChainCodeCorrect::PerformChainCodeWithSteps() {
    perf_.start();
    vector<vector<Point>> cv_contours = ChainCodeCorrect::FindContours();
    perf_.stop();
    perf_.store(Step(StepType::ALGORITHM), perf_.last());

    perf_.start();
    ChainCodeCorrect::ConvertToChainCode(cv_contours);
    perf_.stop();
    perf_.store(Step(StepType::CONVERSION), perf_.last());
}

REGISTER_CHAINCODEALG(ChainCodeCorrect)
