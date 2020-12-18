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


#ifndef BACCA_CHAIN_CODE_H_
#define BACCA_CHAIN_CODE_H_

#include <vector>
#include <set>

#include <opencv2/imgproc.hpp>

#define PREALLOC_ELEMS 5000
//#define PREALLOC_INTS 3

struct RCCode {
    int value_count = 0;
    RCCode () {
        //data.resize(PREALLOC_ELEMS);
    }

    struct Elem  {

        struct Chain {
        private:
            std::vector<uint32_t> internal_values;
            //unsigned elem; // index of the corresponding elem in vector
        public:
            size_t value_count = 0;

            Chain() {
                //internal_values.resize(PREALLOC_INTS);
            }
           // Chain(unsigned elem_) : elem(elem_) {}
            inline void push_back(uint8_t val) {
                if ((value_count & 15) == 0 /*&& value_count / 16 >= PREALLOC_INTS*/) {
                    internal_values.push_back(0);
                }

                internal_values[value_count / 16] |= ((val & 3) << ((value_count & 15) << 1));
                value_count++;
            }
            inline const uint8_t get_value(unsigned index) const {
                int internal_index = index / 16;
                return (internal_values[internal_index] >> ((index % 16) * 2)) & 3;
            }
            //auto begin() { return vals.begin(); }
            //auto begin() const { return vals.begin(); }
            //auto end() { return vals.end(); }
            //auto end() const { return vals.end(); }
        };

        unsigned row, col;
        Chain left;
        Chain right;
        unsigned next; // vector index of the elem whose left chain is linked to this elem right chain

        Elem() {}
        Elem(unsigned r_, unsigned c_, unsigned elem_) : row(r_), col(c_), left(), right(), next(elem_) {}

        //Chain& operator[](bool right_) {
        //    if (right_) return right;
        //    else return left;
        //}
        //const Chain& operator[](bool right_) const {
        //    if (right_) return right;
        //    else return left;
        //}
    };

    std::vector<Elem> data;

    void AddElem(unsigned r_, unsigned c_) {
        /*if (value_count < PREALLOC_ELEMS) {
            // unsigned r_, unsigned c_, unsigned elem_
            data[value_count].row = r_;
            data[value_count].col = c_;
            data[value_count].next = value_count;
        }
        else {*/
            data.emplace_back(r_, c_, data.size());
        /*}*/
        value_count++;
    }

    size_t Size() const { 
        return value_count;
    }

    Elem& operator[](unsigned pos) { return data[pos]; }
    const Elem& operator[](unsigned pos) const { return data[pos]; }

    void Clean() {
        data = std::vector<Elem>();
        //data.resize(0);
        //data.shrink_to_fit();
    }
};

struct ChainCode {

    struct Chain {

        unsigned row, col;
        std::vector<uint32_t> internal_values;

        Chain() {}
        Chain(unsigned row_, unsigned col_) : row(row_), col(col_) {}

        bool operator==(const Chain& rhs) const;
        bool operator<(const Chain& rhs) const;
        void AddRightChain(const RCCode::Elem::Chain& chain);
        void AddLeftChain(const RCCode::Elem::Chain& chain);
        //const uint8_t& operator[](size_t pos) const { return vals[pos]; }
        //std::vector<uint8_t>::const_iterator begin() const { return vals.begin(); }
        //std::vector<uint8_t>::const_iterator end() const { return vals.end(); }

        size_t value_count = 0;

        void push_back(uint8_t val) {
            if (value_count % 8 == 0) {
                internal_values.push_back(0);
            }

            internal_values[internal_values.size() - 1] |= ((val & 15) << ((value_count % 8) * 4));
            value_count++;
        }
        const uint8_t get_value(unsigned index) const {
            int internal_index = index / 8;
            return (internal_values[internal_index] >> ((index % 8) * 4)) & 15;
        }

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
        chains = std::vector<Chain>();
        //chains.resize(0);
        //chains.shrink_to_fit();
    }

};

#endif // BACCA_CHAIN_CODE_H_