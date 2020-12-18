// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include "chaincode_chang.h"
#include "register.h"


using namespace std;
using namespace cv;

void Chang::PerformChainCode() {

    img_labels_ = cv::Mat1i(img_.size(), 0);

    int n_labels_ = 0;

    for (int y = 0; y < img_.rows; y++) {
        const unsigned char* const img_row = img_.ptr<unsigned char>(y);
        unsigned int* const img_labels_row = img_labels_.ptr<unsigned int>(y);
        for (int x = 0; x < img_.cols; x++) {

            if (img_row[x] > 0) {
                // Case 1
                if (img_labels_row[x] == 0 && (x == 0 || img_row[x - 1] == 0)) {
                    n_labels_++;
                    ContourTracing(x, y, n_labels_, true);
                    continue;
                }
                // Case 2
                else if (x < img_.cols - 1 && img_row[x + 1] == 0 && img_labels_row[x + 1] != -1) {
                    if (img_labels_row[x] == 0) {
                        // Current pixel unlabeled
                        // Assing label of left pixel
                        ContourTracing(x, y, img_labels_row[x - 1], false);
                    }
                    else {
                        ContourTracing(x, y, img_labels_row[x], false);
                    }
                    continue;
                }
                // case 3
                else if (img_labels_row[x] == 0) {
                    img_labels_row[x] = img_labels_row[x - 1];
                }
            }
        }
    }

    // Reassign to contour background value (0)
    //for (int r = 0; r < img_labels_.rows; ++r) {
    //    unsigned* const img_labels_row = img_labels_.ptr<unsigned>(r);
    //    for (int c = 0; c < img_labels_.cols; ++c) {
    //        if (img_labels_row[c] == -1)
    //            img_labels_row[c] = 0;
    //    }
    //}

    n_labels_++; // To count also background label

    chain_code_ = ChainCode(contours, false);

}


cv::Point2i Chang::Tracer(const cv::Point2i& p, int& i_prev, bool& b_isolated) {

    int i_first, i_next;

    // Find the direction to be analyzed
    i_first = i_next = (i_prev + 2) % 8;

    cv::Point2i crd_next;
    do {
        switch (i_next) {
        case 0: crd_next = p + cv::Point2i(1, 0); break;
        case 1: crd_next = p + cv::Point2i(1, 1); break;
        case 2: crd_next = p + cv::Point2i(0, 1); break;
        case 3: crd_next = p + cv::Point2i(-1, 1); break;
        case 4: crd_next = p + cv::Point2i(-1, 0); break;
        case 5: crd_next = p + cv::Point2i(-1, -1); break;
        case 6: crd_next = p + cv::Point2i(0, -1); break;
        case 7: crd_next = p + cv::Point2i(1, -1); break;
        }

        if (crd_next.y >= 0 && crd_next.x >= 0 && crd_next.y < img_.rows && crd_next.x < img_.cols) {
            if (img_(crd_next.y, crd_next.x) > 0) {
                i_prev = (i_next + 4) % 8;
                return crd_next;
            }
            else
                img_labels_(crd_next.y, crd_next.x) = -1;
        }

        i_next = (i_next + 1) % 8;
    } while (i_next != i_first);

    b_isolated = true;
    return p;
}

void Chang::ContourTracing(int x, int y, int i_label, bool b_external) {

    cv::Point2i s(x, y), T, crd_next_point, crd_cur_point;

    // The current point is labeled 
    img_labels_(s.y, s.x) = i_label;

    bool b_isolated = false;
    int i_previous_contour_point;
    if (b_external)
        i_previous_contour_point = 6;
    else
        i_previous_contour_point = 7;

    // Chain code adding
    //chain_code_.chains.emplace_back(s.y, s.x);
    //ChainCode::Chain& chain = chain_code_.chains.back();
    contours.emplace_back();
    vector<Point>& contour = contours.back();

    // First call to Tracer
    crd_next_point = T = Tracer(s, i_previous_contour_point, b_isolated);
    crd_cur_point = s;

    if (b_isolated) {
        contour.push_back(s);
        return;
    }

    do {
        // Chain code adding
        contour.push_back(crd_cur_point);

        crd_cur_point = crd_next_point;

        img_labels_(crd_cur_point.y, crd_cur_point.x) = i_label;
        crd_next_point = Tracer(crd_cur_point, i_previous_contour_point, b_isolated);
    } while (!(crd_cur_point == s && crd_next_point == T));
}


REGISTER_CHAINCODEALG(Chang)