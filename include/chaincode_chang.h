// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef BACCA_CHAINCODE_CHANG_H_
#define BACCA_CHAINCODE_CHANG_H_

#include "chaincode_algorithms.h"


class Chang : public ChainCodeAlg {

public:

    cv::Mat1i img_labels_;
    std::vector<std::vector<cv::Point>> contours;

    virtual void PerformChainCode();

    void ContourTracing(int x, int y, int i_label, bool b_external);

    cv::Point2i Tracer(const cv::Point2i& p, int& i_prev, bool& b_isolated);

    virtual void FreeChainCodeData() {
        contours = std::vector<std::vector<cv::Point>>();
        ChainCodeAlg::FreeChainCodeData();
    }
};


#endif // BACCA_CHAINCODE_CHANG_H_
