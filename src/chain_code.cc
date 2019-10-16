// Copyright(c) 2019 Federico Bolelli, Costantino Grana
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
// *Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and / or other materials provided with the distribution.
//
// * Neither the name of THeBE nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

                    chain.vals.push_back(link);

                    prev = cur;

                }
            }
            else {
                for (int i = top + 1; i <= top + contour.size(); i++) {

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

                    chain.vals.push_back(link);

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
        vals == rhs.vals;
}

bool ChainCode::Chain::operator<(const Chain& rhs) const {
    if (row != rhs.row) {
        return row < rhs.row;
    }
    else if (col != rhs.col) {
        return col < rhs.col;
    }
    else if (vals.size() != rhs.vals.size()) {
        return vals.size() < rhs.vals.size();
    }
    else {
        return vals[0] < rhs.vals[0];
    }
}

void ChainCode::Chain::AddRightChain(const RCCode::Elem::Chain& chain) {

    for (auto val : chain.vals) {
        uint8_t new_val;
        if (val == 0) {
            new_val = 0;
        }
        else {
            new_val = 8 - val;
        }
        vals.push_back(new_val);
    }

}

void ChainCode::Chain::AddLeftChain(const RCCode::Elem::Chain& chain) {

    for (auto it = chain.vals.rbegin(); it != chain.vals.rend(); it++) {
        const uint8_t& val = *it;

        uint8_t new_val = 4 - val;

        vals.push_back(new_val);
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