// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include "chaincode_scheffler.h"

#include <fstream>
#include <vector>
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

    RCNode *x, *y;
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

            if (r == 238 && c == 293) {
                int x = 0;
            }

            pos = ProcessPixelNaiveTopology(r, c, state, rccode, chains, pos, chain_is_left, &object, &hole);
        }

        previous_row_ptr = row_ptr;
        row_ptr += img_.step[0];
    }

    RCCodeToChainCode(rccode, chain_code_, hierarchy_);

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
REGISTER_CHAINCODEALG(SchefflerTopology)
