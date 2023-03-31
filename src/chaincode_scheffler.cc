// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include "chaincode_scheffler.h"

#include <fstream>
#include <vector>
#include <array>
#include <memory>
#include <algorithm>

#include <opencv2/imgproc.hpp>

#include "register.h"


/*

  +---+---+---+
  | a | b | c |
  +---+---+---+
  | d | e | f |
  +---+---+---+

  f e d c b a
  5 4 3 2 1 0

*/

using namespace std;
using namespace cv;

#define PIXEL_A     1u
#define PIXEL_B     2u
#define PIXEL_C     4u
#define PIXEL_D     8u
#define PIXEL_E     16u
#define PIXEL_F     32u

#define D0_L        2048u
#define D0_R        1024u
#define D1_L        512u
#define D1_R        256u
#define D2_L        128u
#define D2_R        64u
#define D3_L        32u
#define D3_R        16u
#define MIN_O       8u
#define MIN_I       4u
#define MAX_O       2u
#define MAX_I       1u


namespace {

struct TemplateCheck {

    using Template = const int8_t[6];

    static const Template templates[14];

    static bool TemplateMatch(uint8_t condition, unsigned templ_index) {
        for (uint8_t i = 0; i < 6; i++) {

            if (templates[templ_index][i] != -1) {

                if (((condition >> i) & 1u) != templates[templ_index][i]) {
                    return false;
                }

            }

        }

        return true;
    }

    static uint16_t CondToState(uint8_t condition) {

        uint16_t state = 0;

        // Chains' links
        for (unsigned i = 0; i < 8; i++) {

            if (TemplateMatch(condition, i)) {
                state |= (1 << (11 - i));
            }

        }

        // Min points (outer)
        if (TemplateMatch(condition, 8)) {
            state |= MIN_O;
        }


        // Min points (inner)
        for (unsigned i = 0; i < 2; i++) {

            if (TemplateMatch(condition, i + 9)) {
                state |= MIN_I;
                break;
            }

        }

        // Max points (outer)
        if (TemplateMatch(condition, 11)) {
            state |= MAX_O;
        }

        // Max points (inner)
        for (unsigned i = 0; i < 2; i++) {

            if (TemplateMatch(condition, i + 12)) {
                state |= MAX_I;
                break;
            }

        }

        return state;
    }

};

TemplateCheck::Template TemplateCheck::templates[] = {

    // Chain links
    { 1,  1, -1,
      0,  0, -1,
    },
    { 0,  0, -1,
      1,  1, -1,
    },
    { 1, -1, -1,
      0,  1, -1,
    },
    { 1,  0, -1,
     -1,  1, -1,
    },
    { 0,  1, -1,
      0,  1, -1,
    },
    {-1,  1,  0,
     -1,  1,  0,
    },
    {-1,  0,  1,
     -1,  1, -1,
    },
    {-1, -1,  1,
     -1,  1,  0,
    },

    // Min Points (outer)
    {-1,  1,  0,
      0,  0,  0},

      // Min Points (inner)
      {0,  0,  1,
       1,  1, -1},

      { 1,  0,  1,
       -1,  1, -1},

       // Max Points (outer)
       {0,  0,  0,
        0,  1, -1},

        // Max Points (inner)
        {-1, 1, 1,
          1, 0, 0 },

        {-1, 1, -1,
          1, 0,  1 },

};

bool FillLookupTable(string filename = "scheffler_lut.inc", string table_name = "table") {

    ofstream os(filename);
    if (!os) {
        return false;
    }

    for (uint8_t conditions = 0; conditions < 64; conditions++) {

        const uint16_t state = TemplateCheck::CondToState(conditions);

        os << state << ", ";

    }

    return true;
}


void WriteActionAndRules() {
    ofstream os("action_rules.yaml");

    std::set<uint16_t> states;
    std::array<uint16_t, 64> conditions_to_states;

    for (uint8_t conditions = 0; conditions < 64; conditions++) {

        const uint16_t state = TemplateCheck::CondToState(conditions);
        states.insert(state);
        conditions_to_states[conditions] = state;

    }

    std::vector<uint16_t> sorted_states(states.begin(), states.end());
    os << "actions: [\n";
    for (auto state : sorted_states) {
        os << "    " << state << ",\n";
    }
    os << "]\n";

    os << "rules: [\n";
    for (uint8_t conditions = 0; conditions < 64; conditions++) {

        auto state = conditions_to_states[conditions];
        auto state_iterator = std::lower_bound(sorted_states.begin(), sorted_states.end(), state);
        ptrdiff_t state_pos = state_iterator - sorted_states.begin();
        os << "    [" << state_pos + 1 << "],\n";

    }
    os << "]\n";
}

}

void MergeNodes(RCNode* dst, RCNode* src, RCCode& rccode, bool different_status = false) {
    unsigned int index = src->elem_index;

    // Update node pointer in src max point
    rccode[index].node = dst;

    // Update node pointer in next max points
    unsigned int next_index = rccode[index].next;
    while (next_index != index) {
        index = next_index;
        rccode[index].node = dst;
        next_index = rccode[index].next;
    }

    // Update node pointer in prev max points
    unsigned int prev_index = rccode[index].prev;
    while (prev_index != index) {
        index = prev_index;
        rccode[index].node = dst;
        prev_index = rccode[index].prev;
    }

    // Reparent children of src
    RCNode* new_parent = different_status ? dst->parent : dst;
    for (auto& child : src->children) {
        child.get()->parent = new_parent;
    }
    new_parent->children.insert(new_parent->children.end(),
        make_move_iterator(src->children.begin()),
        make_move_iterator(src->children.end()));

    // Let's re-add this.
    // Nodes once again point to the top-left maxpoint in the contour.
    dst->elem_index = min(dst->elem_index, src->elem_index);

    src->parent->DeleteChild(src);
}

/*
Connect the two chains preceding pos (pos-1 and pos-2).
Erases the two connected chains from the list (vector),
so after this call the outer value of pos may be invalid.
*/
template <bool outer>
inline void ConnectChains(RCCode& rccode, vector<unsigned>& chains, unsigned int pos) {

    // outer: first_it is left and second_it is right
    // inner: first_it is right and second_it is left

    if (outer) {
        rccode[chains[pos - 1]].next = chains[pos - 2];
    }
    else {
        rccode[chains[pos - 2]].next = chains[pos - 1];
    }

    // Remove chains from vector
    chains.erase(chains.begin() + pos - 2, chains.begin() + pos);
}

template <bool outer>
inline void ConnectChainsTopology(RCCode& rccode, vector<unsigned>& chains, unsigned int pos,
    RCNode** object, RCNode** hole) {

    RCNode* current_node = nullptr;
    RCNode* l = rccode[chains[pos - 2]].node;
    RCNode* r = rccode[chains[pos - 1]].node;

    // outer: first_it is left and second_it is right
    // inner: first_it is right and second_it is left

    if (outer) {
        rccode[chains[pos - 1]].next = chains[pos - 2];
        rccode[chains[pos - 2]].prev = chains[pos - 1];
    }
    else {
        rccode[chains[pos - 2]].next = chains[pos - 1];
        rccode[chains[pos - 1]].prev = chains[pos - 2];
    }

    // Remove chains from vector
    chains.erase(chains.begin() + pos - 2, chains.begin() + pos);

    RCNode* x, * y;
    if (outer) {
        y = l;
        x = r;
    }
    else {
        x = l;
        y = r;
    }

    if (l == r) {
        if (l->status == RCNode::Status::potH) {
            l->status = RCNode::Status::H;
        }
        else {
            l->status = RCNode::Status::O;
        }
    }
    else if (l->status == r->status) {
        MergeNodes(l, r, rccode, false);
        current_node = l;
    }
    else {
        if (l->status == RCNode::Status::potO) {
            MergeNodes(y, x, rccode, true);
            current_node = y->parent;
        }
        else {
            MergeNodes(x, y, rccode, true);
            current_node = x->parent;
        }
    }
    if (current_node != nullptr) {
        if (outer) {
            *hole = current_node;
        }
        else {
            *object = current_node;
        }
    }
    else {
        // Save coordinates? It's not clear to me
    }
}

unsigned int ProcessPixelNaive(int r, int c, uint16_t state, RCCode& rccode,
    vector<unsigned>& chains, unsigned int pos, bool& chain_is_left) {

    if (state & D0_L) {
        if (chain_is_left) {
            rccode[chains[pos]].left.push_back(0);
        }
        else {
            rccode[chains[pos - 1]].left.push_back(0);
        }
    }

    if (state & MIN_O) {
        ConnectChains<true>(rccode, chains, pos + 2);
    }

    if (state & MAX_I) {
        rccode.AddElem(r - 1, c);
        if (chain_is_left) {
            chains.insert(chains.begin() + pos - 1, 2, static_cast<int>(rccode.Size()) - 1);
            rccode[chains[pos - 1]].right.push_back(rccode[chains[pos + 1]].right.pop_back());
        }
        else {
            chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        }
    }

    if (state & D0_R) {
        if (chain_is_left) {
            rccode[chains[pos - 1]].right.push_back(0);
        }
        else {
            rccode[chains[pos - 2]].right.push_back(0);
        }
    }

    if (state & D1_L) {
        rccode[chains[pos]].left.push_back(1);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D1_R) {
        rccode[chains[pos]].right.push_back(1);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D2_L) {
        rccode[chains[pos]].left.push_back(2);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D2_R) {
        rccode[chains[pos]].right.push_back(2);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D3_L) {
        rccode[chains[pos]].left.push_back(3);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D3_R) {
        rccode[chains[pos]].right.push_back(3);
        pos++;
        chain_is_left = !chain_is_left;
    }

    if (state & MIN_I) {
        if (chain_is_left) {
            ConnectChains<false>(rccode, chains, pos - 1);
        }
        else {
            ConnectChains<false>(rccode, chains, pos);
        }
        pos -= 2;
    }

    if (state & MAX_O) {
        rccode.AddElem(r, c);
        chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        pos += 2;
    }

    return pos;

}

template <uint16_t state>
unsigned int ProcessPixel(int r, int c, RCCode& rccode,
    vector<unsigned>& chains, unsigned int pos, bool& chain_is_left) {

    if (state & D0_L) {
        if (chain_is_left) {
            rccode[chains[pos]].left.push_back(0);
        }
        else {
            rccode[chains[pos - 1]].left.push_back(0);
        }
    }

    if (state & MIN_O) {
        ConnectChains<true>(rccode, chains, pos + 2);
    }

    if (state & MAX_I) {
        rccode.AddElem(r - 1, c);
        if (chain_is_left) {
            chains.insert(chains.begin() + pos - 1, 2, static_cast<int>(rccode.Size()) - 1);
            rccode[chains[pos - 1]].right.push_back(rccode[chains[pos + 1]].right.pop_back());
        }
        else {
            chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        }
    }

    if (state & D0_R) {
        if (chain_is_left) {
            rccode[chains[pos - 1]].right.push_back(0);
        }
        else {
            rccode[chains[pos - 2]].right.push_back(0);
        }
    }

    if (state & D1_L) {
        rccode[chains[pos]].left.push_back(1);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D1_R) {
        rccode[chains[pos]].right.push_back(1);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D2_L) {
        rccode[chains[pos]].left.push_back(2);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D2_R) {
        rccode[chains[pos]].right.push_back(2);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D3_L) {
        rccode[chains[pos]].left.push_back(3);
        pos++;
        chain_is_left = !chain_is_left;
    }
    if (state & D3_R) {
        rccode[chains[pos]].right.push_back(3);
        pos++;
        chain_is_left = !chain_is_left;
    }

    if (state & MIN_I) {
        if (chain_is_left) {
            ConnectChains<false>(rccode, chains, pos - 1);
        }
        else {
            ConnectChains<false>(rccode, chains, pos);
        }
        pos -= 2;
    }

    if (state & MAX_O) {
        rccode.AddElem(r, c);
        chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        pos += 2;
    }

    return pos;

}


void AddLinkLeft(RCCode& rccode, vector<unsigned>& chains, unsigned int& pos, bool& chain_is_left,
    RCNode** object, RCNode** hole, uint8_t link) {
    rccode[chains[pos]].left.push_back(link);

    RCNode* node = rccode[chains[pos]].node;
    if (node->status == RCNode::Status::potO) {
        *object = node;
    }
    else if (node->status == RCNode::Status::potH) {
        *hole = node->parent->parent;
        *object = node->parent;
    }

    pos++;
    chain_is_left = !chain_is_left;
}

void AddLinkRight(RCCode& rccode, vector<unsigned>& chains, unsigned int& pos, bool& chain_is_left,
    RCNode** object, RCNode** hole, uint8_t link) {
    rccode[chains[pos]].right.push_back(link);

    RCNode* node = rccode[chains[pos]].node;
    if (node->status == RCNode::Status::potH) {
        *hole = node;
    }

    pos++;
    chain_is_left = !chain_is_left;
}

unsigned int ProcessPixelNaiveTopology(int r, int c, uint16_t state, RCCode& rccode,
    vector<unsigned>& chains, unsigned int pos, bool& chain_is_left,
    RCNode** object, RCNode** hole) {

    if (state & D0_L) {
        if (chain_is_left) {
            rccode[chains[pos]].left.push_back(0);
        }
        else {
            rccode[chains[pos - 1]].left.push_back(0);
        }
    }

    if (state & MIN_O) {
        ConnectChainsTopology<true>(rccode, chains, pos + 2, object, hole);
    }

    if (state & MAX_I) {
        RCNode* new_node = (*object)->EmplaceChild(static_cast<int>(rccode.Size()), RCNode::Status::potH);
        rccode.AddElem(r - 1, c, new_node);
        if (chain_is_left) {
            chains.insert(chains.begin() + pos - 1, 2, static_cast<int>(rccode.Size()) - 1);
            rccode[chains[pos - 1]].right.push_back(rccode[chains[pos + 1]].right.pop_back());
        }
        else {
            chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        }
    }

    if (state & D0_R) {
        if (chain_is_left) {
            rccode[chains[pos - 1]].right.push_back(0);
        }
        else {
            rccode[chains[pos - 2]].right.push_back(0);
        }
    }

    if (state & D1_L) {
        AddLinkLeft(rccode, chains, pos, chain_is_left, object, hole, 1);
    }
    if (state & D1_R) {
        AddLinkRight(rccode, chains, pos, chain_is_left, object, hole, 1);
    }
    if (state & D2_L) {
        AddLinkLeft(rccode, chains, pos, chain_is_left, object, hole, 2);
    }
    if (state & D2_R) {
        AddLinkRight(rccode, chains, pos, chain_is_left, object, hole, 2);
    }
    if (state & D3_L) {
        AddLinkLeft(rccode, chains, pos, chain_is_left, object, hole, 3);
    }
    if (state & D3_R) {
        AddLinkRight(rccode, chains, pos, chain_is_left, object, hole, 3);
    }

    if (state & MIN_I) {
        if (chain_is_left) {
            ConnectChainsTopology<false>(rccode, chains, pos - 1, object, hole);
        }
        else {
            ConnectChainsTopology<false>(rccode, chains, pos, object, hole);
        }
        pos -= 2;
    }

    if (state & MAX_O) {
        RCNode* new_node = (*hole)->EmplaceChild(static_cast<int>(rccode.Size()), RCNode::Status::potO);
        *object = new_node;
        rccode.AddElem(r, c, new_node);
        chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        pos += 2;
    }

    return pos;

}


void Scheffler::PerformChainCode() {

    RCCode rccode;

    vector<unsigned> chains;

    int h = img_.rows;
    int w = img_.cols;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    bool chain_is_left = true;

    // Build Raster Scan Chain Code
    for (int r = 0; r < h + 1; r++) {
        unsigned int pos = 0;
        chain_is_left = true;   // TODO verificare

        for (int c = 0; c < w; c++) {

            uint8_t condition = 0;

            if (r > 0 && c > 0 && previous_row_ptr[c - 1])                     condition |= PIXEL_A;
            if (r > 0 && previous_row_ptr[c])                                  condition |= PIXEL_B;
            if (r > 0 && c + 1 < w && previous_row_ptr[c + 1])                 condition |= PIXEL_C;
            if (r < h && c > 0 && row_ptr[c - 1])                              condition |= PIXEL_D;
            if (r < h && row_ptr[c])                                           condition |= PIXEL_E;
            if (r < h && c + 1 < w && row_ptr[c + 1])                          condition |= PIXEL_F;

            uint16_t state = TemplateCheck::CondToState(condition);

            pos = ProcessPixelNaive(r, c, state, rccode, chains, pos, chain_is_left);
        }

        previous_row_ptr = row_ptr;
        row_ptr += img_.step[0];
    }

    RCCodeToChainCode(rccode, chain_code_);
}


void Scheffler_LUT::PerformChainCode() {

    //static const constexpr std::array<uint16_t, 64> StateLUT = {
    //    0, 0, 8, 2056, 0, 0, 0, 2048, 0, 0, 0, 0, 0, 0, 1, 1,
    //    2, 768, 192, 576, 48, 820, 144, 528, 1024, 256, 64, 64, 1076, 308, 16, 16,
    //    0, 0, 0, 2048, 0, 0, 0, 2048, 0, 0, 1, 1, 0, 0, 1, 1,
    //    2, 768, 128, 512, 32, 804, 128, 512, 1024, 256, 0, 0, 1060, 292, 0, 0 };

#include "scheffler_lut.inc"

    RCCode rccode;

    vector<unsigned> chains;

    int h = img_.rows;
    int w = img_.cols;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    bool chain_is_left = true;

    // Build Raster Scan Chain Code
    for (int r = 0; r < h + 1; r++) {
        unsigned int pos = 0;
        chain_is_left = true;   // TODO verificare

        for (int c = 0; c < w; c++) {

            uint8_t condition = 0;

            if (r > 0 && c > 0 && previous_row_ptr[c - 1])                     condition |= PIXEL_A;
            if (r > 0 && previous_row_ptr[c])                                  condition |= PIXEL_B;
            if (r > 0 && c + 1 < w && previous_row_ptr[c + 1])                 condition |= PIXEL_C;
            if (r < h && c > 0 && row_ptr[c - 1])                              condition |= PIXEL_D;
            if (r < h && row_ptr[c])                                           condition |= PIXEL_E;
            if (r < h && c + 1 < w && row_ptr[c + 1])                          condition |= PIXEL_F;

            //const uint16_t state = StateLUT[condition];

            auto processPixelFunction = LUT[condition];
            pos = processPixelFunction(r, c, rccode, chains, pos, chain_is_left);
        }

        previous_row_ptr = row_ptr;
        row_ptr += img_.step[0];
    }

    RCCodeToChainCode(rccode, chain_code_);
}


void Scheffler_LUT_PRED::PerformChainCode() {

#include "scheffler_lut.inc"

    RCCode rccode;

    vector<unsigned> chains;

    int h = img_.rows;
    int w = img_.cols;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    bool chain_is_left = true;

    if (h == 1 || w == 1) {
        for (int r = 0; r < h + 1; r++) {
            unsigned int pos = 0;
            chain_is_left = true;   // TODO verificare

            for (int c = 0; c < w; c++) {

                uint8_t condition = 0;

                if (r > 0 && c > 0 && previous_row_ptr[c - 1])                     condition |= PIXEL_A;
                if (r > 0 && previous_row_ptr[c])                                  condition |= PIXEL_B;
                if (r > 0 && c + 1 < w && previous_row_ptr[c + 1])                 condition |= PIXEL_C;
                if (r < h && c > 0 && row_ptr[c - 1])                              condition |= PIXEL_D;
                if (r < h && row_ptr[c])                                           condition |= PIXEL_E;
                if (r < h && c + 1 < w && row_ptr[c + 1])                          condition |= PIXEL_F;

                //const uint16_t state = StateLUT[condition];

                auto processPixelFunction = LUT[condition];
                pos = processPixelFunction(r, c, rccode, chains, pos, chain_is_left);
            }

            previous_row_ptr = row_ptr;
            row_ptr += img_.step[0];
        }

        RCCodeToChainCode(rccode, chain_code_);
        return;
    }

    // Build Raster Scan Chain Code

    // First Line
    {
        unsigned int pos = 0;
        chain_is_left = true;   // TODO verificare
        uint8_t condition = 0;

        if (row_ptr[0])                  condition |= PIXEL_E;
        if (row_ptr[1])                  condition |= PIXEL_F;

        auto processPixelFunction = LUT[condition];
        pos = processPixelFunction(0, 0, rccode, chains, pos, chain_is_left);

        for (int c = 1; c < w - 1; c++) {

            condition = condition >> 1 & ~7;

            if (row_ptr[c + 1])                          condition |= PIXEL_F;

            processPixelFunction = LUT[condition];
            pos = processPixelFunction(0, c, rccode, chains, pos, chain_is_left);

        }

        condition = condition >> 1 & ~7;
        processPixelFunction = LUT[condition];
        pos = processPixelFunction(0, w - 1, rccode, chains, pos, chain_is_left);

        previous_row_ptr = row_ptr;
        row_ptr += img_.step[0];
    }

    // Middle Lines
    for (int r = 1; r < h; r++) {
        unsigned int pos = 0;
        chain_is_left = true;   // TODO verificare

        uint8_t condition = 0;

        if (previous_row_ptr[0])            condition |= PIXEL_B;
        if (previous_row_ptr[1])            condition |= PIXEL_C;
        if (row_ptr[0])                     condition |= PIXEL_E;
        if (row_ptr[1])                     condition |= PIXEL_F;

        auto processPixelFunction = LUT[condition];
        pos = processPixelFunction(r, 0, rccode, chains, pos, chain_is_left);

        for (int c = 1; c < w - 1; c++) {

            condition = condition >> 1 & ~(PIXEL_C | PIXEL_F);

            if (previous_row_ptr[c + 1])                condition |= PIXEL_C;
            if (row_ptr[c + 1])                         condition |= PIXEL_F;

            processPixelFunction = LUT[condition];
            pos = processPixelFunction(r, c, rccode, chains, pos, chain_is_left);

        }

        condition = condition >> 1 & ~(PIXEL_C | PIXEL_F);
        processPixelFunction = LUT[condition];
        pos = processPixelFunction(r, w - 1, rccode, chains, pos, chain_is_left);

        previous_row_ptr = row_ptr;
        row_ptr += img_.step[0];
    }

    // Last line
    {
        unsigned int pos = 0;
        chain_is_left = true;   // TODO verificare
        uint8_t condition = 0;

        if (previous_row_ptr[0])            condition |= PIXEL_B;
        if (previous_row_ptr[1])            condition |= PIXEL_C;

        auto processPixelFunction = LUT[condition];
        pos = processPixelFunction(0, h, rccode, chains, pos, chain_is_left);

        for (int c = 1; c < w - 1; c++) {

            condition = condition >> 1;

            if (previous_row_ptr[c + 1])            condition |= PIXEL_C;

            processPixelFunction = LUT[condition];
            pos = processPixelFunction(h, c, rccode, chains, pos, chain_is_left);

        }

        condition = condition >> 1;
        processPixelFunction = LUT[condition];
        pos = processPixelFunction(h, w - 1, rccode, chains, pos, chain_is_left);
    }

    RCCodeToChainCode(rccode, chain_code_);
}


void SchefflerTopology::PerformChainCode() {

    with_hierarchy_ = true;

    RCCode rccode(true);

    vector<unsigned> chains;

    int h = img_.rows;
    int w = img_.cols;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    bool chain_is_left = true;

    RCNode* object = nullptr;
    RCNode* hole = rccode.root.get();

    // Build Raster Scan Chain Code
    for (int r = 0; r < h + 1; r++) {
        unsigned int pos = 0;
        chain_is_left = true;

        for (int c = 0; c < w; c++) {

            uint8_t condition = 0;

            if (r > 0 && c > 0 && previous_row_ptr[c - 1])                     condition |= PIXEL_A;
            if (r > 0 && previous_row_ptr[c])                                  condition |= PIXEL_B;
            if (r > 0 && c + 1 < w && previous_row_ptr[c + 1])                 condition |= PIXEL_C;
            if (r < h && c > 0 && row_ptr[c - 1])                              condition |= PIXEL_D;
            if (r < h && row_ptr[c])                                           condition |= PIXEL_E;
            if (r < h && c + 1 < w && row_ptr[c + 1])                          condition |= PIXEL_F;

            uint16_t state = TemplateCheck::CondToState(condition);

            pos = ProcessPixelNaiveTopology(r, c, state, rccode, chains, pos, chain_is_left, &object, &hole);
        }

        previous_row_ptr = row_ptr;
        row_ptr += img_.step[0];
    }

    RCCodeToChainCode(rccode, chain_code_, hierarchy_);

}


void Scheffler_Spaghetti::PerformChainCode() {

    RCCode rccode;

    vector<unsigned> chains;

    int h = img_.rows;
    int w = img_.cols;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    bool chain_is_left = true;

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           

#define ACTION_1	pos = ProcessPixel<0	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_2	pos = ProcessPixel<1	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_3	pos = ProcessPixel<2	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_4	pos = ProcessPixel<8	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_5	pos = ProcessPixel<16	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_6	pos = ProcessPixel<32	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_7	pos = ProcessPixel<48	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_8	pos = ProcessPixel<64	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_9	pos = ProcessPixel<128	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_10	pos = ProcessPixel<144	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_11	pos = ProcessPixel<192	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_12	pos = ProcessPixel<256	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_13	pos = ProcessPixel<292	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_14	pos = ProcessPixel<308	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_15	pos = ProcessPixel<512	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_16	pos = ProcessPixel<528	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_17	pos = ProcessPixel<576	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_18	pos = ProcessPixel<768	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_19	pos = ProcessPixel<804	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_20	pos = ProcessPixel<820	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_21	pos = ProcessPixel<1024	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_22	pos = ProcessPixel<1060	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_23	pos = ProcessPixel<1076	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_24	pos = ProcessPixel<2048	>(r, c, rccode, chains, pos, chain_is_left);
#define ACTION_25	pos = ProcessPixel<2056	>(r, c, rccode, chains, pos, chain_is_left);

    // First line
    int r = 0;
    int c = -1;
    unsigned int pos = 0;

    c = -1;
    goto fl_tree_0;

#include "Scheffler_Spaghetti_first_line_forest_code.inc.h"

    if (h > 1) {
        // Build Raster Scan Chain Code
        for (r = 1; r < h; r++) {
            chain_is_left = true;   // TODO verify this

            previous_row_ptr = row_ptr;
            row_ptr += img_.step[0];
            pos = 0;

            c = -1;
            goto cl_tree_0;

#include "Scheffler_Spaghetti_center_line_forest_code.inc.h"

        }

        chain_is_left = true;   // TODO verify this
        previous_row_ptr = row_ptr;
        row_ptr = nullptr;
        pos = 0;

        c = -1;
        goto bl_tree_0;

#include "Scheffler_Spaghetti_below_line_forest_code.inc.h"

    }

#undef ACTION_1 
#undef ACTION_2 
#undef ACTION_3 
#undef ACTION_4 
#undef ACTION_5 
#undef ACTION_6 
#undef ACTION_7 
#undef ACTION_8 
#undef ACTION_9 
#undef ACTION_10
#undef ACTION_11
#undef ACTION_12
#undef ACTION_13
#undef ACTION_14
#undef ACTION_15
#undef ACTION_16
#undef ACTION_17
#undef ACTION_18
#undef ACTION_19
#undef ACTION_20
#undef ACTION_21
#undef ACTION_22
#undef ACTION_23
#undef ACTION_24
#undef ACTION_25

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E

    RCCodeToChainCode(rccode, chain_code_);
}



#undef D0_L
#undef D0_R
#undef D1_L
#undef D1_R
#undef D2_L
#undef D2_R
#undef D3_L
#undef D3_R
#undef MIN_O
#undef MIN_I
#undef MAX_O
#undef MAX_I

#undef PIXEL_A
#undef PIXEL_B
#undef PIXEL_C
#undef PIXEL_D
#undef PIXEL_E
#undef PIXEL_F


REGISTER_CHAINCODEALG(Scheffler)
REGISTER_CHAINCODEALG(Scheffler_LUT)
REGISTER_CHAINCODEALG(Scheffler_LUT_PRED)
REGISTER_CHAINCODEALG(Scheffler_Spaghetti)
REGISTER_CHAINCODEALG(SchefflerTopology)
