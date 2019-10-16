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

#ifndef BACCA_CHAIN_CODE_H_
#define BACCA_CHAIN_CODE_H_

#include <vector>
#include <set>

#include <opencv2/imgproc.hpp>


struct RCCode {

    struct Elem  {

        struct Chain {
            std::vector<uint8_t> vals;
            unsigned elem; // index of the corresponding elem in vector
            bool right;

            Chain() {}
            Chain(unsigned elem_, bool right_) : elem(elem_), right(right_) {}
            auto push_back(uint8_t val) { return vals.push_back(val); }
            auto begin() { return vals.begin(); }
            auto begin() const { return vals.begin(); }
            auto end() { return vals.end(); }
            auto end() const { return vals.end(); }
        };

        unsigned row, col;
        Chain left;
        Chain right;
        unsigned next; // vector index of the elem whose left chain is linked to this elem right chain

        Elem() {}
        Elem(unsigned r_, unsigned c_, unsigned elem_) : row(r_), col(c_), left(elem_, false), right(elem_, true), next(elem_) {}

        Chain& operator[](bool right_) {
            if (right_) return right;
            else return left;
        }
        const Chain& operator[](bool right_) const {
            if (right_) return right;
            else return left;
        }
    };

    std::vector<Elem> data;

    void AddElem(unsigned r_, unsigned c_) {
        data.emplace_back(r_, c_, data.size());
    }

    size_t Size() const { return data.size(); }

    Elem& operator[](unsigned pos) { return data[pos]; }
    const Elem& operator[](unsigned pos) const { return data[pos]; }

    void Clean() {
        data.resize(0);
        data.shrink_to_fit();
    }
};

struct ChainCode {

    struct Chain {

        unsigned row, col;
        std::vector<uint8_t> vals;

        Chain() {}
        Chain(unsigned row_, unsigned col_) : row(row_), col(col_) {}

        bool operator==(const Chain& rhs) const;
        bool operator<(const Chain& rhs) const;
        void AddRightChain(const RCCode::Elem::Chain& chain);
        void AddLeftChain(const RCCode::Elem::Chain& chain);
        const uint8_t& operator[](size_t pos) const { return vals[pos]; }
        std::vector<uint8_t>::const_iterator begin() const { return vals.begin(); }
        std::vector<uint8_t>::const_iterator end() const { return vals.end(); }

    };

    std::vector<Chain> chains;

    ChainCode() {}
    ChainCode(const RCCode& rccode);
    ChainCode(const std::vector<std::vector<cv::Point>>& contours, bool contrary = false);

    const Chain& operator[](size_t pos) const { return chains[pos]; }
    std::vector<Chain>::const_iterator begin() const { return chains.begin(); }
    std::vector<Chain>::const_iterator end() const { return chains.end(); }
    void AddChain(const RCCode& rccode, std::vector<bool>& used_elems, unsigned pos);
    bool operator==(const ChainCode& rhs) const {
        return std::set<Chain>(chains.begin(), chains.end()) == std::set<Chain>(rhs.chains.begin(), rhs.chains.end());
    }
    bool operator!=(const ChainCode& rhs) const {
        return !(*this == rhs);
    }

    void Clean() {
        chains.resize(0);
        chains.shrink_to_fit();
    }

};

#endif // BACCA_CHAIN_CODE_H_