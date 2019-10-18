#ifndef BACCA_CHAINCODE_CORRECT_H_
#define BACCA_CHAINCODE_CORRECT_H_

#include "chaincode_algorithms.h"

#include <vector>

#include <opencv2/core.hpp>

class ChainCodeCorrect : public ChainCodeAlg {

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


#endif // BACCA_CHAINCODE_CORRECT_H_