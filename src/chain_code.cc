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

using namespace std;
using namespace cv;

ChainCode::ChainCode(const vector<vector<Point>>& contours, bool contrary) {
    for (const vector<Point>& contour : contours) {

        // Find top-left value
        int top = 0;
        Point max = contour.front();
        for (unsigned int i = 1; i < contour.size(); i++) {
            if (contour[i].y < max.y || (contour[i].y == max.y && contour[i].x < max.x)) {
                max = contour[i];
                top = i;
            }
        }

        chains.emplace_back(max.y, max.x);
        Chain& chain = chains.back();

        Point prev = max;

        if (contour.size() > 1) {

            if (contrary) {
                for (int i = top - 1 + contour.size(); i >= top; i--) { // OpenCV order is reversed

                    Point cur = contour[i % contour.size()];
                    Point diff = cur - prev;

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
                for (int i = top + 1; i <= top + contour.size(); i++) {

                    Point cur = contour[i % contour.size()];
                    Point diff = cur - prev;

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
        push_back((8 - chain.get_value(i)) & 7);
    }

}

void ChainCode::Chain::AddLeftChain(const RCCode::Elem::Chain& chain) {

    for (int i = chain.value_count - 1; i >= 0; i--) {
        push_back(4 - chain.get_value(i));
    }
}

void ChainCode::AddChain(const RCCode& rccode, vector<bool>& used_elems, unsigned pos) {

    chains.emplace_back(rccode[pos].row, rccode[pos].col);
    Chain& new_chain = chains.back();

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

}

ChainCode::ChainCode(const RCCode& rccode) {
    vector<bool> used_elems(rccode.Size(), false);

    for (unsigned i = 0; i < rccode.Size(); i++) {
        if (!used_elems[i]) {
            AddChain(rccode, used_elems, i);
        }
    }
}



vector<vector<Point>> RCCode::ToContours() {

    vector<vector<Point>> contours;

    int pos = 0;

    while (true) {
        for (; pos < data.size() && data[pos].next == -1; pos++);
        if (pos == data.size())
            break;

        contours.emplace_back();
        vector<Point>& contour = contours.back();
        int x = data[pos].col;
        int y = data[pos].row;

        int lpos = pos;
        while (data[lpos].next != -1) {

            const int next_lpos = data[lpos].next;
            data[lpos].next = -1;

            // Right chain
            for (int i = 0; i < data[lpos].right.value_count; i++) {
                contour.emplace_back(x, y);
                switch (data[lpos].right.get_value(i)) {
                case 0: x++;            break;
                case 1: x++;    y++;    break;
                case 2:         y++;    break;
                case 3: x--;    y++;    break;
                default:                break;
                }
            }

            // Next elem
            lpos = next_lpos;

            // Left chain
            for (int i = data[lpos].left.value_count - 1; i >= 0; i--) {
                contour.emplace_back(x, y);
                switch (data[lpos].left.get_value(i)) {
                case 0: x--;            break;
                case 1: x--;    y--;    break;
                case 2:         y--;    break;
                case 3: x++;    y--;    break;
                default:                break;
                }
            }

        }
    }

    return contours;
}