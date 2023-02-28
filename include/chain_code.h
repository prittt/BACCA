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
#include <memory>
#include <algorithm>
#include <numeric>

#include <opencv2/imgproc.hpp>

#define PREALLOC_ELEMS 5000
//#define PREALLOC_INTS 3

struct RCNode {

    enum class Status { O, H, potO, potH, none };

    int elem_index;
    RCNode* parent = nullptr;
    std::vector<std::unique_ptr<RCNode>> children;
    Status status = Status::none;

    RCNode(int elem_index_) : elem_index(elem_index_) {}
    RCNode(int elem_index_, RCNode* parent_) : elem_index(elem_index_), parent(parent_) {}
    RCNode(int elem_index_, RCNode* parent_, Status status_) :
        elem_index(elem_index_), parent(parent_), status(status_) {}

    RCNode* EmplaceChild(int elem_index_, Status status_) {
        children.push_back(std::move(std::make_unique<RCNode>(elem_index_, this, status_)));
        return children.back().get();
    }

    void DeleteChild(RCNode* child) {
        auto is_child = [child](std::unique_ptr<RCNode>& n) {return n.get() == child; };

        auto child_it = std::find_if(std::begin(children), std::end(children), is_child);
        assert(child_it != std::end(children));

        children.erase(child_it);
    }
};

struct RCCode {

    struct Chain {
    private:
        std::vector<uint32_t> internal_values;
        //unsigned elem; // index of the corresponding elem in vector
    public:
        unsigned int value_count = 0;

        Chain() {
            //internal_values.resize(PREALLOC_INTS);
        }
        // Chain(unsigned elem_) : elem(elem_) {}
        inline void push_back(uint8_t val) {
            if ((value_count & 15) == 0 /*&& value_count / 16 >= PREALLOC_INTS*/) { // x & 15 is a "clever" way of doing x % 16
                internal_values.push_back(0);
            }

            internal_values[value_count / 16] |= ((val & 3) << ((value_count & 15) * 2));
            value_count++;
        }

        // TODO try to optimize removal and successive addition
        inline uint8_t pop_back() {
            value_count--;
            const unsigned int index = value_count / 16;
            uint8_t res = (internal_values[index] >> ((value_count & 15) * 2)) & 3;
            // zero the two bits of the popped value
            internal_values[index] &= ~(3 << ((value_count & 15) * 2));
            return res;
        }

        inline const uint8_t get_value(unsigned index) const {
            return (internal_values[index / 16] >> ((index % 16) * 2)) & 3;
        }
        //auto begin() { return vals.begin(); }
        //auto begin() const { return vals.begin(); }
        //auto end() { return vals.end(); }
        //auto end() const { return vals.end(); }
    };

    struct MaxPoint  {        

        unsigned row, col;
        Chain left;
        Chain right;
        unsigned next;  // vector index of the elem whose left chain is linked to this elem right chain
        unsigned prev;  // vector index of the elem whose right chain is linked to this elem left chain
                        // prev field is only necessary when hierarchy is needed,
                        // it is not known if prev field is bad for performance when it is not used
        RCNode* node = nullptr; // it is not known if this added field is bad for performance when it is not used

        // MaxPoint() {}
        MaxPoint(unsigned r_, unsigned c_, unsigned elem_, RCNode* node_) : 
            row(r_), col(c_), left(), right(), next(elem_), prev(elem_), node(node_) {}
        MaxPoint(unsigned r_, unsigned c_, unsigned elem_) : MaxPoint(r_, c_, elem_, nullptr) {}

    };

    void AddElem(unsigned r_, unsigned c_) {
        data.emplace_back(r_, c_, static_cast<unsigned int>(data.size()));
        value_count++;
    }
    void AddElem(unsigned r_, unsigned c_, RCNode* node_) {
        data.emplace_back(r_, c_, static_cast<unsigned int>(data.size()), node_);
        value_count++;
    }


    size_t Size() const { 
        return value_count;
    }

    MaxPoint& operator[](unsigned pos) { return data[pos]; }
    const MaxPoint& operator[](unsigned pos) const { return data[pos]; }

    void Clean() {
        data = std::vector<MaxPoint>();
        root = nullptr;
    }

    RCCode(bool retrieve_topology = false) {
        if (retrieve_topology) {
            root = std::make_unique<RCNode>(-1);
        }
    }

    int value_count = 0;
    std::vector<MaxPoint> data;
    std::unique_ptr<RCNode> root;
};


struct ChainCode {

    struct Chain {

        unsigned row = 0, col = 0;
        std::vector<uint32_t> internal_values;

        Chain() = default;
        Chain(unsigned row_, unsigned col_) : row(row_), col(col_) {}

        bool operator==(const Chain& rhs) const;
        bool operator<(const Chain& rhs) const;
        void AddRightChain(const RCCode::Chain& chain);
        void AddLeftChain(const RCCode::Chain& chain);
        //const uint8_t& operator[](size_t pos) const { return vals[pos]; }
        //std::vector<uint8_t>::const_iterator begin() const { return vals.begin(); }
        //std::vector<uint8_t>::const_iterator end() const { return vals.end(); }

        size_t value_count = 0;

        void push_back(uint8_t val) {
            if (value_count % 8 == 0) {
                internal_values.push_back(0);
            }

            internal_values[internal_values.size() - 1] |= ((val & 15) << ((value_count & 7) * 4));
            value_count++;
        }
        const uint8_t get_value(unsigned index) const {
            int internal_index = index / 8;
            return (internal_values[internal_index] >> ((index & 7) * 4)) & 15;
        }

    };

    std::vector<Chain> chains;

    ChainCode() = default;
    ChainCode(const std::vector<std::vector<cv::Point>>& contours, bool contrary = false);

    const Chain& operator[](size_t pos) const { return chains[pos]; }
    std::vector<Chain>::const_iterator begin() const { return chains.begin(); }
    std::vector<Chain>::const_iterator end() const { return chains.end(); }
    void AddChain(const RCCode& rccode, std::vector<int>& used_elems, unsigned pos);
    bool operator==(const ChainCode& rhs) const {
        return chains == rhs.chains;
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


void RCCodeToChainCode(const RCCode& rccode, ChainCode& chcode, std::vector<cv::Vec4i>& hierarchy);
void RCCodeToChainCode(const RCCode& rccode, ChainCode& chcode);

bool CheckHierarchy(const std::vector<cv::Vec4i>& hierarchy);

void SortChains(ChainCode& chcode);
void SortChains(ChainCode& chcode, std::vector<cv::Vec4i>& hierarchy);





#endif // BACCA_CHAIN_CODE_H_