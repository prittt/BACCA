#include <opencv2/imgproc.hpp>

#include "chaincode_correct.h"
#include "register.h"

using namespace std;
using namespace cv;

void ChainCodeCorrect::PerformChainCode() {

    vector<vector<Point>> cv_contours;
    findContours(img_, cv_contours, RETR_LIST, CHAIN_APPROX_NONE);
    chain_code_ = ChainCode(cv_contours);

}

REGISTER_CHAINCODEALG(ChainCodeCorrect)
