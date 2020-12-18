// Copyright (c) 2020, the BACCA contributors, as 
// shown by the AUTHORS file, plus additional authors
// listed below. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
//
// Additional Authors:
// Maximilian Soechting
// Hasso Plattner Institute
// University of Potsdam, Germany


#include "chain_code.h"

ChainCode::ChainCode(const std::vector<std::vector<cv::Point>>& contours, bool contrary) {
    for (const std::vector<cv::Point>& contour : contours) {

        // Find top-left value
        int top = 0;
        cv::Point max = contour.front();
        for (unsigned int i = 1; i < contour.size(); i++) {
            if (contour[i].y < max.y || (contour[i].y == max.y && contour[i].x < max.x)) {
                max = contour[i];
                top = i;
            }
        }

        Chain chain(max.y, max.x);
        cv::Point prev = max;

        if (contour.size() > 1) {

            if (contrary) {
                for (int i = top - 1 + contour.size(); i >= top; i--) { // OpenCV order is reversed

                    cv::Point cur = contour[i % contour.size()];
                    cv::Point diff = cur - prev;

                    int link;
                    if (diff.x == -1) {
                        link = diff.y + 4;
                    }
                    else if (diff.x == 0) {
                        link = diff.y * 2 + 4;
                    }
                    else if (diff.x == 1) {
                        link = (8 - diff.y) % 8;
                    }

                    chain.push_back(link);

                    prev = cur;

                }
            }
            else {
                for (unsigned int i = top + 1; i <= top + contour.size(); i++) {

                    cv::Point cur = contour[i % contour.size()];
                    cv::Point diff = cur - prev;

                    int link;
                    if (diff.x == -1) {
                        link = diff.y + 4;
                    }
                    else if (diff.x == 0) {
                        link = diff.y * 2 + 4;
                    }
                    else if (diff.x == 1) {
                        link = (8 - diff.y) % 8;
                    }

                    chain.push_back(link);

                    prev = cur;

                }
            }

        }

        chains.push_back(chain);
    }
}

bool ChainCode::Chain::operator==(const Chain& rhs) const {
    return row == rhs.row &&
        col == rhs.col &&
        internal_values == rhs.internal_values;
}

bool ChainCode::Chain::operator<(const Chain& rhs) const {
    if (row != rhs.row) {
        return row < rhs.row;
    }
    else if (col != rhs.col) {
        return col < rhs.col;
    }
    else if (value_count != rhs.value_count) {
        return value_count < rhs.value_count;
    }
    else {
        return get_value(0) < rhs.get_value(0);
    }
}

void ChainCode::Chain::AddRightChain(const RCCode::Elem::Chain& chain) {

    for (unsigned i = 0; i < chain.value_count; i++) {
        uint8_t val = chain.get_value(i);
        uint8_t new_val;
        if (val == 0) {
            new_val = 0;
        }
        else {
            new_val = 8 - val;
        }
        push_back(new_val);
    }

}

void ChainCode::Chain::AddLeftChain(const RCCode::Elem::Chain& chain) {

    for (int i = chain.value_count - 1; i >= 0; i--) {
        uint8_t val = 4 - chain.get_value(i);
        push_back(val);
    }
}

void ChainCode::AddChain(const RCCode& rccode, std::vector<bool>& used_elems, unsigned pos) {

    Chain new_chain(rccode[pos].row, rccode[pos].col);

    new_chain.AddRightChain(rccode[pos].right);

    used_elems[pos] = true;

    while (true) {

        pos = rccode[pos].next;
        new_chain.AddLeftChain(rccode[pos].left);

        if (used_elems[pos]) {
            break;
        }

        new_chain.AddRightChain(rccode[pos].right);

        used_elems[pos] = true;
    }

    chains.push_back(new_chain);
}

ChainCode::ChainCode(const RCCode& rccode) {
    std::vector<bool> used_elems;
    used_elems.resize(rccode.Size(), false);

    for (unsigned i = 0; i < rccode.Size(); i++) {
        if (!used_elems[i]) {
            AddChain(rccode, used_elems, i);
        }
    }
}