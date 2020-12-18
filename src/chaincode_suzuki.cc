// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include <opencv2/imgproc.hpp>

#include "chaincode_suzuki.h"
#include "register.h"

using namespace std;
using namespace cv;

void Suzuki::PerformChainCode() {
    vector<vector<Point>> cv_contours;
    findContours(img_, cv_contours, RETR_LIST, CHAIN_APPROX_NONE);
    chain_code_ = ChainCode(cv_contours, true);
}

vector<vector<Point>> Suzuki::FindContours() {
    vector<vector<Point>> cv_contours;
    findContours(img_, cv_contours, RETR_LIST, CHAIN_APPROX_NONE);
    return cv_contours;
}
void Suzuki::ConvertToChainCode(const std::vector<std::vector<cv::Point>>& cv_contours) {
    chain_code_ = ChainCode(cv_contours, true);
}

void Suzuki::PerformChainCodeWithSteps() {
    perf_.start();
    vector<vector<Point>> cv_contours = Suzuki::FindContours();
    perf_.stop();
    perf_.store(Step(StepType::ALGORITHM), perf_.last());

    perf_.start();
    Suzuki::ConvertToChainCode(cv_contours);
    perf_.stop();
    perf_.store(Step(StepType::CONVERSION), perf_.last());
}

REGISTER_CHAINCODEALG(Suzuki)
