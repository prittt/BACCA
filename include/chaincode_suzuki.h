// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef BACCA_CHAINCODE_SUZUKI_H_
#define BACCA_CHAINCODE_SUZUKI_H_

#include "chaincode_algorithms.h"

#include <vector>

#include <opencv2/core.hpp>

class Suzuki : public ChainCodeAlg {

private:

    std::vector<std::vector<cv::Point>> FindContours();
    void ConvertToChainCode(const std::vector<std::vector<cv::Point>>& cv_contours);

public:
    virtual void PerformChainCode() override;
    virtual void PerformChainCodeWithSteps() override;

    virtual void FreeChainCodeData() override {
        ChainCodeAlg::FreeChainCodeData();
    }

};


#endif // BACCA_CHAINCODE_SUZUKI_H_