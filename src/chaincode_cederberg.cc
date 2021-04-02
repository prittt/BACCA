// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include "chaincode_cederberg.h"

#include <fstream>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "register.h"


/*

    a | b | c
   ---+---+---
    d | x | e
   ---+---+---
    f | g | h

    h g f e x d c b a
    8 7 6 5 4 3 2 1 0

*/


using namespace std;
using namespace cv;

#define PIXEL_A     1u
#define PIXEL_B     2u
#define PIXEL_C     4u
#define PIXEL_D     8u
#define PIXEL_X     16u
#define PIXEL_E     32u
#define PIXEL_F     64u
#define PIXEL_G     128u
#define PIXEL_H     256u

#define D0A             2048u
#define D0B             1024u
#define D1A             512u
#define D1B             256u
#define D2A             128u
#define D2B             64u
#define D3A             32u
#define D3B             16u
#define MIN_OUTER       8u
#define MIN_INNER       4u
#define MAX_OUTER       2u
#define MAX_INNER       1u

struct TemplateCheck {

    using Template = const int8_t[9];

    static const Template templates[14];

    static bool TemplateMatch(unsigned short condition, unsigned templ) {
        for (uint8_t i = 0; i < 9; i++) {

            if (templates[templ][i] != -1) {

                if (((condition >> i) & 1u) != templates[templ][i]) {
                    return false;
                }

            }

        }

        return true;
    }

    static unsigned short CheckState(unsigned short condition) {

        unsigned short state = 0;

        // Chains' links
        for (unsigned i = 0; i < 8; i++) {

            if (TemplateMatch(condition, i)) {
                state |= (1 << (11 - i));
            }

        }

        // Min points (outer)
        if (TemplateMatch(condition, 8)) {
            state |= MIN_OUTER;
        }


        // Min points (inner)
        for (unsigned i = 0; i < 2; i++) {

            if (TemplateMatch(condition, i + 9)) {
                state |= MIN_INNER;
                break;
            }

        }

        // Max points (outer)
        if (TemplateMatch(condition, 11)) {
            state |= MAX_OUTER;
        }

        // Max points (inner)
        for (unsigned i = 0; i < 2; i++) {

            if (TemplateMatch(condition, i + 12)) {
                state |= MAX_INNER;
                break;
            }

        }

        return state;
    }

};
TemplateCheck::Template TemplateCheck::templates[] = {

        // Chain links
        {-1, -1, -1,
          1,  1, -1,
          0,  0, -1,
        },
        { 0,  0, -1,
          1,  1, -1,
         -1, -1, -1,
        },
        { 1, -1, -1,
          0,  1, -1,
         -1, -1, -1
        },
        { 1,  0, -1,
         -1,  1, -1,
         -1, -1, -1
        },
        { 0,  1, -1,
          0,  1, -1,
         -1, -1, -1
        },
        {-1,  1,  0,
         -1,  1,  0,
         -1, -1, -1
        },
        {-1,  0,  1,
         -1,  1, -1,
         -1, -1, -1
        },
        {-1, -1,  1,
         -1,  1,  0,
         -1, -1, -1
        },

        // Min Points (outer)
        {-1, -1, -1,
         -1,  1,  0,
          0,  0,  0
        },

        // Min Points (inner)
        {0, 0, 1, 1, 1, -1, -1, -1, -1},
        {1, 0, 1, -1, 1, -1, -1, -1, -1},

        // Max Points (outer)
        {0,  0,  0,
         0,  1, -1,
        -1, -1, -1
        },

        // Max Points (inner)
        { -1, -1, -1, -1, 1, 1, 1, 0, 0 },
        { -1, -1, -1, -1, 1, -1, 1, 0, 1 },

    };

bool FillLookupTable(string filename = "cederberg_lut.inc", string table_name = "table") {

    ofstream os(filename);
    if (!os) {
        return false;
    }

    os << "static unsigned int (*" << table_name << "[512]) (int r, int c, RCCode & rccode, vector<unsigned> & chains, unsigned int pos) = {\n";

    for (unsigned short i = 0; i < 512; i++) {

        unsigned short state = TemplateCheck::CheckState(i);

        os << "    " << "ProcessPixel<" << state << ">,\n";

    }

    os << "};\n";

    return true;
}

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

unsigned int ProcessPixelNaive(int r, int c, unsigned short state, RCCode& rccode, vector<unsigned>& chains, unsigned int pos) {

    if (state == 10) {
        // state == 10 is the only single-pixel case
        rccode.AddElem(r, c);
        return pos;
    }

    uint8_t links_found;

    switch (state) {
    case 0:     links_found = 0; break;
    case 1:     links_found = 0; break;
    case 2:     links_found = 0; break;
    case 3:     links_found = 0; break;
    case 10:    links_found = 0; break;
    case 16:    links_found = 1; break;
    case 17:    links_found = 1; break;
    case 32:    links_found = 1; break;
    case 33:    links_found = 1; break;
    case 48:    links_found = 2; break;
    case 49:    links_found = 2; break;
    case 56:    links_found = 2; break;
    case 64:    links_found = 1; break;
    case 65:    links_found = 1; break;
    case 128:   links_found = 1; break;
    case 129:   links_found = 1; break;
    case 144:   links_found = 2; break;
    case 145:   links_found = 2; break;
    case 152:   links_found = 2; break;
    case 192:   links_found = 2; break;
    case 193:   links_found = 2; break;
    case 200:   links_found = 2; break;
    case 256:   links_found = 1; break;
    case 257:   links_found = 1; break;
    case 292:   links_found = 2; break;
    case 293:   links_found = 2; break;
    case 308:   links_found = 3; break;
    case 309:   links_found = 3; break;
    case 512:   links_found = 1; break;
    case 513:   links_found = 1; break;
    case 528:   links_found = 2; break;
    case 529:   links_found = 2; break;
    case 536:   links_found = 2; break;
    case 576:   links_found = 2; break;
    case 577:   links_found = 2; break;
    case 584:   links_found = 2; break;
    case 768:   links_found = 2; break;
    case 769:   links_found = 2; break;
    case 776:   links_found = 2; break;
    case 804:   links_found = 3; break;
    case 805:   links_found = 3; break;
    case 820:   links_found = 4; break;
    case 821:   links_found = 4; break;
    case 828:   links_found = 4; break;
    case 1024:  links_found = 1; break;
    case 1025:  links_found = 1; break;
    case 1060:  links_found = 2; break;
    case 1061:  links_found = 2; break;
    case 1076:  links_found = 3; break;
    case 1077:  links_found = 3; break;
    case 2048:  links_found = 1; break;
    case 2064:  links_found = 2; break;
    case 2072:  links_found = 2; break;
    case 2112:  links_found = 2; break;
    case 2120:  links_found = 2; break;
    case 2304:  links_found = 2; break;
    case 2312:  links_found = 2; break;
    case 2340:  links_found = 3; break;
    case 2356:  links_found = 4; break;
    case 2364:  links_found = 4; break;
    case 3072:  links_found = 2; break;
    case 3080:  links_found = 2; break;
    case 3108:  links_found = 3; break;
    case 3124:  links_found = 4; break;
    case 3132:  links_found = 4; break;
    default:    return pos;      break;
    }

    bool last_found_right = false;

    if (state & MAX_OUTER) {
        rccode.AddElem(r, c);

        chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);

        pos += 2;

        last_found_right = true;
    }
    else {

        if ((state & D0A) && (state & D0B)) {
            pos -= 2;
        }
        else if ((state & D0A) || (state & D0B)) {
            pos--;
        }

        // Chains' links
        if (state & D0A) {
            rccode[chains[pos]].left.push_back(0);
            last_found_right = false;
            pos++;
        }

        if (state & D0B) {
            rccode[chains[pos]].right.push_back(0);
            last_found_right = true;
            pos++;
        }

        if (state & D1A) {
            rccode[chains[pos]].left.push_back(1);
            last_found_right = false;
            pos++;
        }

        if (state & D1B) {
            rccode[chains[pos]].right.push_back(1);
            last_found_right = true;
            pos++;
        }

        if (state & D2A) {
            rccode[chains[pos]].left.push_back(2);
            last_found_right = false;
            pos++;
        }

        if (state & D2B) {
            rccode[chains[pos]].right.push_back(2);
            last_found_right = true;
            pos++;
        }

        if (state & D3A) {
            rccode[chains[pos]].left.push_back(3);
            last_found_right = false;
            pos++;
        }

        if (state & D3B) {
            rccode[chains[pos]].right.push_back(3);
            last_found_right = true;
            pos++;
        }

        // Min points
        if ((state & MIN_INNER) && (state & MIN_OUTER)) {
            // 4 chains met, connect 2nd and 3rd, then 1st and 4th.
            ConnectChains<false>(rccode, chains, pos - 1);
            ConnectChains<true>(rccode, chains, pos - 2);
            pos -= 4;
        }
        else if (state & MIN_OUTER) {
            // 2 chains met, connect them
            ConnectChains<true>(rccode, chains, pos);
            pos -= 2;
        }
        else if (state & MIN_INNER) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains<false>(rccode, chains, pos);
            }
            else if (links_found == 3) {
                if ((state & D3A) && (state & D3B)) {
                    // 3 chains met, connect 1st and 2nd
                    ConnectChains<false>(rccode, chains, pos - 1);
                }
                else {
                    // 3 chains met, connect 2nd and 3rd
                    ConnectChains<false>(rccode, chains, pos);
                }
            }
            else {
                // 4 chains met, connect 2nd and 3rd
                ConnectChains<false>(rccode, chains, pos - 1);
            }
            pos -= 2;
        }

    }

    if (state & MAX_INNER) {
        rccode.AddElem(r, c);

        if (last_found_right) {
            chains.insert(chains.begin() + pos - 1, 2, static_cast<int>(rccode.Size()) - 1);
        }
        else {
            chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        }

        pos += 2;
    }

    return pos;

}

template <unsigned short state>
inline unsigned int ProcessPixel(int r, int c, RCCode& rccode, vector<unsigned>& chains, unsigned int pos) {

    if (state == 10) {
        // state == 10 is the only single-pixel case
        rccode.AddElem(r, c);
        return pos;
    }

    uint8_t links_found;

    switch (state) {
    case 0:     links_found = 0; break;
    case 1:     links_found = 0; break;
    case 2:     links_found = 0; break;
    case 3:     links_found = 0; break;
    case 10:    links_found = 0; break;
    case 16:    links_found = 1; break;
    case 17:    links_found = 1; break;
    case 32:    links_found = 1; break;
    case 33:    links_found = 1; break;
    case 48:    links_found = 2; break;
    case 49:    links_found = 2; break;
    case 56:    links_found = 2; break;
    case 64:    links_found = 1; break;
    case 65:    links_found = 1; break;
    case 128:   links_found = 1; break;
    case 129:   links_found = 1; break;
    case 144:   links_found = 2; break;
    case 145:   links_found = 2; break;
    case 152:   links_found = 2; break;
    case 192:   links_found = 2; break;
    case 193:   links_found = 2; break;
    case 200:   links_found = 2; break;
    case 256:   links_found = 1; break;
    case 257:   links_found = 1; break;
    case 292:   links_found = 2; break;
    case 293:   links_found = 2; break;
    case 308:   links_found = 3; break;
    case 309:   links_found = 3; break;
    case 512:   links_found = 1; break;
    case 513:   links_found = 1; break;
    case 528:   links_found = 2; break;
    case 529:   links_found = 2; break;
    case 536:   links_found = 2; break;
    case 576:   links_found = 2; break;
    case 577:   links_found = 2; break;
    case 584:   links_found = 2; break;
    case 768:   links_found = 2; break;
    case 769:   links_found = 2; break;
    case 776:   links_found = 2; break;
    case 804:   links_found = 3; break;
    case 805:   links_found = 3; break;
    case 820:   links_found = 4; break;
    case 821:   links_found = 4; break;
    case 828:   links_found = 4; break;
    case 1024:  links_found = 1; break;
    case 1025:  links_found = 1; break;
    case 1060:  links_found = 2; break;
    case 1061:  links_found = 2; break;
    case 1076:  links_found = 3; break;
    case 1077:  links_found = 3; break;
    case 2048:  links_found = 1; break;
    case 2064:  links_found = 2; break;
    case 2072:  links_found = 2; break;
    case 2112:  links_found = 2; break;
    case 2120:  links_found = 2; break;
    case 2304:  links_found = 2; break;
    case 2312:  links_found = 2; break;
    case 2340:  links_found = 3; break;
    case 2356:  links_found = 4; break;
    case 2364:  links_found = 4; break;
    case 3072:  links_found = 2; break;
    case 3080:  links_found = 2; break;
    case 3108:  links_found = 3; break;
    case 3124:  links_found = 4; break;
    case 3132:  links_found = 4; break;
    default:    return pos;      break;
    }

    bool last_found_right = false;

    if (state & MAX_OUTER) {
        rccode.AddElem(r, c);

        chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);

        pos += 2;

        last_found_right = true;
    }
    else {

        if ((state & D0A) && (state & D0B)) {
            pos -= 2;
        }
        else if ((state & D0A) || (state & D0B)) {
            pos--;
        }

        // Chains' links
        if (state & D0A) {
            rccode[chains[pos]].left.push_back(0);
            last_found_right = false;
            pos++;
        }

        if (state & D0B) {
            rccode[chains[pos]].right.push_back(0);
            last_found_right = true;
            pos++;
        }

        if (state & D1A) {
            rccode[chains[pos]].left.push_back(1);
            last_found_right = false;
            pos++;
        }

        if (state & D1B) {
            rccode[chains[pos]].right.push_back(1);
            last_found_right = true;
            pos++;
        }

        if (state & D2A) {
            rccode[chains[pos]].left.push_back(2);
            last_found_right = false;
            pos++;
        }

        if (state & D2B) {
            rccode[chains[pos]].right.push_back(2);
            last_found_right = true;
            pos++;
        }

        if (state & D3A) {
            rccode[chains[pos]].left.push_back(3);
            last_found_right = false;
            pos++;
        }

        if (state & D3B) {
            rccode[chains[pos]].right.push_back(3);
            last_found_right = true;
            pos++;
        }

        // Min points
        if ((state & MIN_INNER) && (state & MIN_OUTER)) {
            // 4 chains met, connect 2nd and 3rd, then 1st and 4th.
            ConnectChains<false>(rccode, chains, pos - 1);
            ConnectChains<true>(rccode, chains, pos - 2);
            pos -= 4;
        }
        else if (state & MIN_OUTER) {
            // 2 chains met, connect them
            ConnectChains<true>(rccode, chains, pos);
            pos -= 2;
        }
        else if (state & MIN_INNER) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains<false>(rccode, chains, pos);
            }
            else if (links_found == 3) {
                if ((state & D3A) && (state & D3B)) {
                    // 3 chains met, connect 1st and 2nd
                    ConnectChains<false>(rccode, chains, pos - 1);
                }
                else {
                    // 3 chains met, connect 2nd and 3rd
                    ConnectChains<false>(rccode, chains, pos);
                }
            }
            else {
                // 4 chains met, connect 2nd and 3rd
                ConnectChains<false>(rccode, chains, pos - 1);
            }
            pos -= 2;
        }

    }

    if (state & MAX_INNER) {
        rccode.AddElem(r, c);

        if (last_found_right) {
            chains.insert(chains.begin() + pos - 1, 2, static_cast<int>(rccode.Size()) - 1);
        }
        else {
            chains.insert(chains.begin() + pos, 2, static_cast<int>(rccode.Size()) - 1);
        }

        pos += 2;
    }

    return pos;
}

void Cederberg::PerformChainCode() {

    RCCode rccode;

    vector<unsigned> chains;

    int h = img_.rows;
    int w = img_.cols;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < h; r++) {
        unsigned int pos = 0;

        for (int c = 0; c < w; c++) {

            unsigned short condition = 0;

            if (r > 0 && c > 0 && previous_row_ptr[c - 1])               condition |= PIXEL_A;
            if (r > 0 && previous_row_ptr[c])                            condition |= PIXEL_B;
            if (r > 0 && c + 1 < w && previous_row_ptr[c + 1])           condition |= PIXEL_C;
            if (c > 0 && row_ptr[c - 1])                                 condition |= PIXEL_D;
            if (row_ptr[c])                                              condition |= PIXEL_X;
            if (c + 1 < w && row_ptr[c + 1])                             condition |= PIXEL_E;
            if (r + 1 < h && c > 0 && next_row_ptr[c - 1])               condition |= PIXEL_F;
            if (r + 1 < h && next_row_ptr[c])                            condition |= PIXEL_G;
            if (r + 1 < h && c + 1 < w && next_row_ptr[c + 1])           condition |= PIXEL_H;

            unsigned short state = TemplateCheck::CheckState(condition);

            pos = ProcessPixelNaive(r, c, state, rccode, chains, pos);
        }

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    RCCodeToChainCode(rccode, chain_code_);
}

void Cederberg_LUT::PerformChainCode() {

    RCCode rccode;

#include "cederberg_lut.inc"

    vector<unsigned> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < img_.rows; r++) {
        unsigned int pos = 0;

        for (int c = 0; c < img_.cols; c++) {

            unsigned short condition = 0;

            if (r > 0 && c > 0 && previous_row_ptr[c - 1])                      condition |= PIXEL_A;
            if (r > 0 && previous_row_ptr[c])                                   condition |= PIXEL_B;
            if (r > 0 && c + 1 < img_.cols && previous_row_ptr[c + 1])           condition |= PIXEL_C;
            if (c > 0 && row_ptr[c - 1])                                        condition |= PIXEL_D;
            if (row_ptr[c])                                                     condition |= PIXEL_X;
            if (c + 1 < img_.cols && row_ptr[c + 1])                             condition |= PIXEL_E;
            if (r + 1 < img_.rows && c > 0 && next_row_ptr[c - 1])               condition |= PIXEL_F;
            if (r + 1 < img_.rows && next_row_ptr[c])                            condition |= PIXEL_G;
            if (r + 1 < img_.rows && c + 1 < img_.cols && next_row_ptr[c + 1])    condition |= PIXEL_H;

            pos = table[condition](r, c, rccode, chains, pos);
        }

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    RCCodeToChainCode(rccode, chain_code_);
}

void Cederberg_LUT_PRED::PerformChainCode() {

    RCCode rccode;

#include "cederberg_lut.inc"

    vector<unsigned> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < img_.rows; r++) {
        unsigned int pos = 0;

        unsigned short condition = 0;

        // First column
        if (r > 0 && previous_row_ptr[0])                           condition |= PIXEL_B;
        if (r > 0 && 1 < img_.cols && previous_row_ptr[1])           condition |= PIXEL_C;
        if (row_ptr[0])                                             condition |= PIXEL_X;
        if (1 < img_.cols && row_ptr[1])                             condition |= PIXEL_E;
        if (r + 1 < img_.rows && next_row_ptr[0])                    condition |= PIXEL_G;
        if (r + 1 < img_.rows && 1 < img_.cols && next_row_ptr[1])    condition |= PIXEL_H;

        pos = table[condition](r, 0, rccode, chains, pos);

        // Middle columns
        for (int c = 1; c < img_.cols - 1; c++) {

            condition >>= 1;
            condition &= ~(PIXEL_C | PIXEL_E | PIXEL_H);

            if (r > 0 && previous_row_ptr[c + 1])           condition |= PIXEL_C;
            if (row_ptr[c + 1])                             condition |= PIXEL_E;
            if (r + 1 < img_.rows && next_row_ptr[c + 1])    condition |= PIXEL_H;

            pos = table[condition](r, c, rccode, chains, pos);
        }

        // Last column
        condition >>= 1;
        condition &= ~(PIXEL_C | PIXEL_E | PIXEL_H);

        pos = table[condition](r, img_.cols - 1, rccode, chains, pos);

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    RCCodeToChainCode(rccode, chain_code_);
}

void Cederberg_DRAG::PerformChainCode() {

    RCCode rccode;

    int w = img_.cols;
    int h = img_.rows;

    vector<unsigned> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           
#define CONDITION_F     (r + 1 < h && next_row_ptr[c - 1])             
#define CONDITION_G     (r + 1 < h && next_row_ptr[c])                          
#define CONDITION_H     (r + 1 < h && next_row_ptr[c + 1]) 

#define ACTION_1    pos = ProcessPixel<0	  >(r, c, rccode, chains, pos);
#define ACTION_2    pos = ProcessPixel<1	  >(r, c, rccode, chains, pos);
#define ACTION_3    pos = ProcessPixel<2	  >(r, c, rccode, chains, pos);
#define ACTION_4    pos = ProcessPixel<3	  >(r, c, rccode, chains, pos);
#define ACTION_5    pos = ProcessPixel<10  >(r, c, rccode, chains, pos);
#define ACTION_6    pos = ProcessPixel<16  >(r, c, rccode, chains, pos);
#define ACTION_7    pos = ProcessPixel<17  >(r, c, rccode, chains, pos);
#define ACTION_8    pos = ProcessPixel<32  >(r, c, rccode, chains, pos);
#define ACTION_9    pos = ProcessPixel<33  >(r, c, rccode, chains, pos);
#define ACTION_10   pos = ProcessPixel<48  >(r, c, rccode, chains, pos);
#define ACTION_11   pos = ProcessPixel<49  >(r, c, rccode, chains, pos);
#define ACTION_12   pos = ProcessPixel<56  >(r, c, rccode, chains, pos);
#define ACTION_13   pos = ProcessPixel<64  >(r, c, rccode, chains, pos);
#define ACTION_14   pos = ProcessPixel<65  >(r, c, rccode, chains, pos);
#define ACTION_15   pos = ProcessPixel<128 >(r, c, rccode, chains, pos);
#define ACTION_16   pos = ProcessPixel<129 >(r, c, rccode, chains, pos);
#define ACTION_17   pos = ProcessPixel<144 >(r, c, rccode, chains, pos);
#define ACTION_18   pos = ProcessPixel<145 >(r, c, rccode, chains, pos);
#define ACTION_19   pos = ProcessPixel<152 >(r, c, rccode, chains, pos);
#define ACTION_20   pos = ProcessPixel<192 >(r, c, rccode, chains, pos);
#define ACTION_21   pos = ProcessPixel<193 >(r, c, rccode, chains, pos);
#define ACTION_22   pos = ProcessPixel<200 >(r, c, rccode, chains, pos);
#define ACTION_23   pos = ProcessPixel<256 >(r, c, rccode, chains, pos);
#define ACTION_24   pos = ProcessPixel<257 >(r, c, rccode, chains, pos);
#define ACTION_25   pos = ProcessPixel<292 >(r, c, rccode, chains, pos);
#define ACTION_26   pos = ProcessPixel<293 >(r, c, rccode, chains, pos);
#define ACTION_27   pos = ProcessPixel<308 >(r, c, rccode, chains, pos);
#define ACTION_28   pos = ProcessPixel<309 >(r, c, rccode, chains, pos);
#define ACTION_29   pos = ProcessPixel<512 >(r, c, rccode, chains, pos);
#define ACTION_30   pos = ProcessPixel<513 >(r, c, rccode, chains, pos);
#define ACTION_31   pos = ProcessPixel<528 >(r, c, rccode, chains, pos);
#define ACTION_32   pos = ProcessPixel<529 >(r, c, rccode, chains, pos);
#define ACTION_33   pos = ProcessPixel<536 >(r, c, rccode, chains, pos);
#define ACTION_34   pos = ProcessPixel<576 >(r, c, rccode, chains, pos);
#define ACTION_35   pos = ProcessPixel<577 >(r, c, rccode, chains, pos);
#define ACTION_36   pos = ProcessPixel<584 >(r, c, rccode, chains, pos);
#define ACTION_37   pos = ProcessPixel<768 >(r, c, rccode, chains, pos);
#define ACTION_38   pos = ProcessPixel<769 >(r, c, rccode, chains, pos);
#define ACTION_39   pos = ProcessPixel<776 >(r, c, rccode, chains, pos);
#define ACTION_40   pos = ProcessPixel<804 >(r, c, rccode, chains, pos);
#define ACTION_41   pos = ProcessPixel<805 >(r, c, rccode, chains, pos);
#define ACTION_42   pos = ProcessPixel<820 >(r, c, rccode, chains, pos);
#define ACTION_43   pos = ProcessPixel<821 >(r, c, rccode, chains, pos);
#define ACTION_44   pos = ProcessPixel<828 >(r, c, rccode, chains, pos);
#define ACTION_45   pos = ProcessPixel<1024>(r, c, rccode, chains, pos);
#define ACTION_46   pos = ProcessPixel<1025>(r, c, rccode, chains, pos);
#define ACTION_47   pos = ProcessPixel<1060>(r, c, rccode, chains, pos);
#define ACTION_48   pos = ProcessPixel<1061>(r, c, rccode, chains, pos);
#define ACTION_49   pos = ProcessPixel<1076>(r, c, rccode, chains, pos);
#define ACTION_50   pos = ProcessPixel<1077>(r, c, rccode, chains, pos);
#define ACTION_51   pos = ProcessPixel<2048>(r, c, rccode, chains, pos);
#define ACTION_52   pos = ProcessPixel<2064>(r, c, rccode, chains, pos);
#define ACTION_53   pos = ProcessPixel<2072>(r, c, rccode, chains, pos);
#define ACTION_54   pos = ProcessPixel<2112>(r, c, rccode, chains, pos);
#define ACTION_55   pos = ProcessPixel<2120>(r, c, rccode, chains, pos);
#define ACTION_56   pos = ProcessPixel<2304>(r, c, rccode, chains, pos);
#define ACTION_57   pos = ProcessPixel<2312>(r, c, rccode, chains, pos);
#define ACTION_58   pos = ProcessPixel<2340>(r, c, rccode, chains, pos);
#define ACTION_59   pos = ProcessPixel<2356>(r, c, rccode, chains, pos);
#define ACTION_60   pos = ProcessPixel<2364>(r, c, rccode, chains, pos);
#define ACTION_61   pos = ProcessPixel<3072>(r, c, rccode, chains, pos);
#define ACTION_62   pos = ProcessPixel<3080>(r, c, rccode, chains, pos);
#define ACTION_63   pos = ProcessPixel<3108>(r, c, rccode, chains, pos);
#define ACTION_64   pos = ProcessPixel<3124>(r, c, rccode, chains, pos);
#define ACTION_65   pos = ProcessPixel<3132>(r, c, rccode, chains, pos);

    int r = 0;
    int c = -1;

    unsigned int pos = 0;

    goto tree_0;

#include "cederberg_forest_first_line.inc"    

    // Build Raster Scan Chain Code
    for (r = 1; r < h; r++) {

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];

        pos = 0;

        c = -1;
        goto tree_5;

#include "cederberg_forest.inc"

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
#undef ACTION_26
#undef ACTION_27
#undef ACTION_28
#undef ACTION_29
#undef ACTION_30
#undef ACTION_31
#undef ACTION_32
#undef ACTION_33
#undef ACTION_34
#undef ACTION_35
#undef ACTION_36
#undef ACTION_37
#undef ACTION_38
#undef ACTION_39
#undef ACTION_40
#undef ACTION_41
#undef ACTION_42
#undef ACTION_43
#undef ACTION_44
#undef ACTION_45
#undef ACTION_46
#undef ACTION_47
#undef ACTION_48
#undef ACTION_49
#undef ACTION_50
#undef ACTION_51
#undef ACTION_52
#undef ACTION_53
#undef ACTION_54
#undef ACTION_55
#undef ACTION_56
#undef ACTION_57
#undef ACTION_58
#undef ACTION_59
#undef ACTION_60
#undef ACTION_61
#undef ACTION_62
#undef ACTION_63
#undef ACTION_64
#undef ACTION_65

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E
#undef CONDITION_F
#undef CONDITION_G
#undef CONDITION_H

    }

    RCCodeToChainCode(rccode, chain_code_);
}

void Cederberg_DRAG::PerformChainCodeWithSteps() {

    perf_.start();
    RCCode rccode = Cederberg_DRAG::PerformRCCode();
    perf_.stop();
    perf_.store(Step(StepType::ALGORITHM), perf_.last());

    perf_.start();
    Cederberg_DRAG::ConvertToChainCode(rccode);
    perf_.stop();
    perf_.store(Step(StepType::CONVERSION), perf_.last());

}

RCCode Cederberg_DRAG::PerformRCCode() {

    RCCode rccode;

    vector<unsigned> chains;

    int w = img_.cols;
    int h = img_.rows;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           
#define CONDITION_F     (r + 1 < h && next_row_ptr[c - 1])             
#define CONDITION_G     (r + 1 < h && next_row_ptr[c])                          
#define CONDITION_H     (r + 1 < h && next_row_ptr[c + 1]) 

#define ACTION_1    pos = ProcessPixel<0	 >(r, c, rccode, chains, pos);
#define ACTION_2    pos = ProcessPixel<1	 >(r, c, rccode, chains, pos);
#define ACTION_3    pos = ProcessPixel<2	 >(r, c, rccode, chains, pos);
#define ACTION_4    pos = ProcessPixel<3	 >(r, c, rccode, chains, pos);
#define ACTION_5    pos = ProcessPixel<10	 >(r, c, rccode, chains, pos);
#define ACTION_6    pos = ProcessPixel<16	 >(r, c, rccode, chains, pos);
#define ACTION_7    pos = ProcessPixel<17	 >(r, c, rccode, chains, pos);
#define ACTION_8    pos = ProcessPixel<32	 >(r, c, rccode, chains, pos);
#define ACTION_9    pos = ProcessPixel<33	 >(r, c, rccode, chains, pos);
#define ACTION_10   pos = ProcessPixel<48	 >(r, c, rccode, chains, pos);
#define ACTION_11   pos = ProcessPixel<49	 >(r, c, rccode, chains, pos);
#define ACTION_12   pos = ProcessPixel<56	 >(r, c, rccode, chains, pos);
#define ACTION_13   pos = ProcessPixel<64  >(r, c, rccode, chains, pos);
#define ACTION_14   pos = ProcessPixel<65	 >(r, c, rccode, chains, pos);
#define ACTION_15   pos = ProcessPixel<128 >(r, c, rccode, chains, pos);
#define ACTION_16   pos = ProcessPixel<129 >(r, c, rccode, chains, pos);
#define ACTION_17   pos = ProcessPixel<144 >(r, c, rccode, chains, pos);
#define ACTION_18   pos = ProcessPixel<145 >(r, c, rccode, chains, pos);
#define ACTION_19   pos = ProcessPixel<152 >(r, c, rccode, chains, pos);
#define ACTION_20   pos = ProcessPixel<192 >(r, c, rccode, chains, pos);
#define ACTION_21   pos = ProcessPixel<193 >(r, c, rccode, chains, pos);
#define ACTION_22   pos = ProcessPixel<200 >(r, c, rccode, chains, pos);
#define ACTION_23   pos = ProcessPixel<256 >(r, c, rccode, chains, pos);
#define ACTION_24   pos = ProcessPixel<257 >(r, c, rccode, chains, pos);
#define ACTION_25   pos = ProcessPixel<292 >(r, c, rccode, chains, pos);
#define ACTION_26   pos = ProcessPixel<293 >(r, c, rccode, chains, pos);
#define ACTION_27   pos = ProcessPixel<308 >(r, c, rccode, chains, pos);
#define ACTION_28   pos = ProcessPixel<309 >(r, c, rccode, chains, pos);
#define ACTION_29   pos = ProcessPixel<512 >(r, c, rccode, chains, pos);
#define ACTION_30   pos = ProcessPixel<513 >(r, c, rccode, chains, pos);
#define ACTION_31   pos = ProcessPixel<528 >(r, c, rccode, chains, pos);
#define ACTION_32   pos = ProcessPixel<529 >(r, c, rccode, chains, pos);
#define ACTION_33   pos = ProcessPixel<536 >(r, c, rccode, chains, pos);
#define ACTION_34   pos = ProcessPixel<576 >(r, c, rccode, chains, pos);
#define ACTION_35   pos = ProcessPixel<577 >(r, c, rccode, chains, pos);
#define ACTION_36   pos = ProcessPixel<584 >(r, c, rccode, chains, pos);
#define ACTION_37   pos = ProcessPixel<768 >(r, c, rccode, chains, pos);
#define ACTION_38   pos = ProcessPixel<769 >(r, c, rccode, chains, pos);
#define ACTION_39   pos = ProcessPixel<776 >(r, c, rccode, chains, pos);
#define ACTION_40   pos = ProcessPixel<804 >(r, c, rccode, chains, pos);
#define ACTION_41   pos = ProcessPixel<805 >(r, c, rccode, chains, pos);
#define ACTION_42   pos = ProcessPixel<820 >(r, c, rccode, chains, pos);
#define ACTION_43   pos = ProcessPixel<821 >(r, c, rccode, chains, pos);
#define ACTION_44   pos = ProcessPixel<828 >(r, c, rccode, chains, pos);
#define ACTION_45   pos = ProcessPixel<1024>(r, c, rccode, chains, pos);
#define ACTION_46   pos = ProcessPixel<1025>(r, c, rccode, chains, pos);
#define ACTION_47   pos = ProcessPixel<1060>(r, c, rccode, chains, pos);
#define ACTION_48   pos = ProcessPixel<1061>(r, c, rccode, chains, pos);
#define ACTION_49   pos = ProcessPixel<1076>(r, c, rccode, chains, pos);
#define ACTION_50   pos = ProcessPixel<1077>(r, c, rccode, chains, pos);
#define ACTION_51   pos = ProcessPixel<2048>(r, c, rccode, chains, pos);
#define ACTION_52   pos = ProcessPixel<2064>(r, c, rccode, chains, pos);
#define ACTION_53   pos = ProcessPixel<2072>(r, c, rccode, chains, pos);
#define ACTION_54   pos = ProcessPixel<2112>(r, c, rccode, chains, pos);
#define ACTION_55   pos = ProcessPixel<2120>(r, c, rccode, chains, pos);
#define ACTION_56   pos = ProcessPixel<2304>(r, c, rccode, chains, pos);
#define ACTION_57   pos = ProcessPixel<2312>(r, c, rccode, chains, pos);
#define ACTION_58   pos = ProcessPixel<2340>(r, c, rccode, chains, pos);
#define ACTION_59   pos = ProcessPixel<2356>(r, c, rccode, chains, pos);
#define ACTION_60   pos = ProcessPixel<2364>(r, c, rccode, chains, pos);
#define ACTION_61   pos = ProcessPixel<3072>(r, c, rccode, chains, pos);
#define ACTION_62   pos = ProcessPixel<3080>(r, c, rccode, chains, pos);
#define ACTION_63   pos = ProcessPixel<3108>(r, c, rccode, chains, pos);
#define ACTION_64   pos = ProcessPixel<3124>(r, c, rccode, chains, pos);
#define ACTION_65   pos = ProcessPixel<3132>(r, c, rccode, chains, pos);

    int r = 0;
    int c = -1;

    unsigned int pos = 0;

    goto tree_0;

#include "cederberg_forest_first_line.inc"    

    // Build Raster Scan Chain Code
    for (r = 1; r < h; r++) {

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];

        pos = 0;

        c = -1;
        goto tree_5;

#include "cederberg_forest.inc"

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
#undef ACTION_26
#undef ACTION_27
#undef ACTION_28
#undef ACTION_29
#undef ACTION_30
#undef ACTION_31
#undef ACTION_32
#undef ACTION_33
#undef ACTION_34
#undef ACTION_35
#undef ACTION_36
#undef ACTION_37
#undef ACTION_38
#undef ACTION_39
#undef ACTION_40
#undef ACTION_41
#undef ACTION_42
#undef ACTION_43
#undef ACTION_44
#undef ACTION_45
#undef ACTION_46
#undef ACTION_47
#undef ACTION_48
#undef ACTION_49
#undef ACTION_50
#undef ACTION_51
#undef ACTION_52
#undef ACTION_53
#undef ACTION_54
#undef ACTION_55
#undef ACTION_56
#undef ACTION_57
#undef ACTION_58
#undef ACTION_59
#undef ACTION_60
#undef ACTION_61
#undef ACTION_62
#undef ACTION_63
#undef ACTION_64
#undef ACTION_65

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E
#undef CONDITION_F
#undef CONDITION_G
#undef CONDITION_H

    }

    return rccode;
}

void Cederberg_DRAG::ConvertToChainCode(const RCCode& rccode) {
    RCCodeToChainCode(rccode, chain_code_);
}

void Cederberg_Spaghetti::PerformChainCode() {

	RCCode rccode;

	int w = img_.cols;
	int h = img_.rows;

	vector<unsigned> chains;

	const unsigned char* previous_row_ptr = nullptr;
	const unsigned char* row_ptr = img_.ptr(0);
	const unsigned char* next_row_ptr = row_ptr + img_.step[0];

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           
#define CONDITION_F     (next_row_ptr[c - 1])             
#define CONDITION_G     (next_row_ptr[c])                          
#define CONDITION_H     (next_row_ptr[c + 1]) 

#define ACTION_1    pos = ProcessPixel<0	  >(r, c, rccode, chains, pos);
#define ACTION_2    pos = ProcessPixel<1	  >(r, c, rccode, chains, pos);
#define ACTION_3    pos = ProcessPixel<2	  >(r, c, rccode, chains, pos);
#define ACTION_4    pos = ProcessPixel<3	  >(r, c, rccode, chains, pos);
#define ACTION_5    pos = ProcessPixel<10  >(r, c, rccode, chains, pos);
#define ACTION_6    pos = ProcessPixel<16  >(r, c, rccode, chains, pos);
#define ACTION_7    pos = ProcessPixel<17  >(r, c, rccode, chains, pos);
#define ACTION_8    pos = ProcessPixel<32  >(r, c, rccode, chains, pos);
#define ACTION_9    pos = ProcessPixel<33  >(r, c, rccode, chains, pos);
#define ACTION_10   pos = ProcessPixel<48  >(r, c, rccode, chains, pos);
#define ACTION_11   pos = ProcessPixel<49  >(r, c, rccode, chains, pos);
#define ACTION_12   pos = ProcessPixel<56  >(r, c, rccode, chains, pos);
#define ACTION_13   pos = ProcessPixel<64  >(r, c, rccode, chains, pos);
#define ACTION_14   pos = ProcessPixel<65  >(r, c, rccode, chains, pos);
#define ACTION_15   pos = ProcessPixel<128 >(r, c, rccode, chains, pos);
#define ACTION_16   pos = ProcessPixel<129 >(r, c, rccode, chains, pos);
#define ACTION_17   pos = ProcessPixel<144 >(r, c, rccode, chains, pos);
#define ACTION_18   pos = ProcessPixel<145 >(r, c, rccode, chains, pos);
#define ACTION_19   pos = ProcessPixel<152 >(r, c, rccode, chains, pos);
#define ACTION_20   pos = ProcessPixel<192 >(r, c, rccode, chains, pos);
#define ACTION_21   pos = ProcessPixel<193 >(r, c, rccode, chains, pos);
#define ACTION_22   pos = ProcessPixel<200 >(r, c, rccode, chains, pos);
#define ACTION_23   pos = ProcessPixel<256 >(r, c, rccode, chains, pos);
#define ACTION_24   pos = ProcessPixel<257 >(r, c, rccode, chains, pos);
#define ACTION_25   pos = ProcessPixel<292 >(r, c, rccode, chains, pos);
#define ACTION_26   pos = ProcessPixel<293 >(r, c, rccode, chains, pos);
#define ACTION_27   pos = ProcessPixel<308 >(r, c, rccode, chains, pos);
#define ACTION_28   pos = ProcessPixel<309 >(r, c, rccode, chains, pos);
#define ACTION_29   pos = ProcessPixel<512 >(r, c, rccode, chains, pos);
#define ACTION_30   pos = ProcessPixel<513 >(r, c, rccode, chains, pos);
#define ACTION_31   pos = ProcessPixel<528 >(r, c, rccode, chains, pos);
#define ACTION_32   pos = ProcessPixel<529 >(r, c, rccode, chains, pos);
#define ACTION_33   pos = ProcessPixel<536 >(r, c, rccode, chains, pos);
#define ACTION_34   pos = ProcessPixel<576 >(r, c, rccode, chains, pos);
#define ACTION_35   pos = ProcessPixel<577 >(r, c, rccode, chains, pos);
#define ACTION_36   pos = ProcessPixel<584 >(r, c, rccode, chains, pos);
#define ACTION_37   pos = ProcessPixel<768 >(r, c, rccode, chains, pos);
#define ACTION_38   pos = ProcessPixel<769 >(r, c, rccode, chains, pos);
#define ACTION_39   pos = ProcessPixel<776 >(r, c, rccode, chains, pos);
#define ACTION_40   pos = ProcessPixel<804 >(r, c, rccode, chains, pos);
#define ACTION_41   pos = ProcessPixel<805 >(r, c, rccode, chains, pos);
#define ACTION_42   pos = ProcessPixel<820 >(r, c, rccode, chains, pos);
#define ACTION_43   pos = ProcessPixel<821 >(r, c, rccode, chains, pos);
#define ACTION_44   pos = ProcessPixel<828 >(r, c, rccode, chains, pos);
#define ACTION_45   pos = ProcessPixel<1024>(r, c, rccode, chains, pos);
#define ACTION_46   pos = ProcessPixel<1025>(r, c, rccode, chains, pos);
#define ACTION_47   pos = ProcessPixel<1060>(r, c, rccode, chains, pos);
#define ACTION_48   pos = ProcessPixel<1061>(r, c, rccode, chains, pos);
#define ACTION_49   pos = ProcessPixel<1076>(r, c, rccode, chains, pos);
#define ACTION_50   pos = ProcessPixel<1077>(r, c, rccode, chains, pos);
#define ACTION_51   pos = ProcessPixel<2048>(r, c, rccode, chains, pos);
#define ACTION_52   pos = ProcessPixel<2064>(r, c, rccode, chains, pos);
#define ACTION_53   pos = ProcessPixel<2072>(r, c, rccode, chains, pos);
#define ACTION_54   pos = ProcessPixel<2112>(r, c, rccode, chains, pos);
#define ACTION_55   pos = ProcessPixel<2120>(r, c, rccode, chains, pos);
#define ACTION_56   pos = ProcessPixel<2304>(r, c, rccode, chains, pos);
#define ACTION_57   pos = ProcessPixel<2312>(r, c, rccode, chains, pos);
#define ACTION_58   pos = ProcessPixel<2340>(r, c, rccode, chains, pos);
#define ACTION_59   pos = ProcessPixel<2356>(r, c, rccode, chains, pos);
#define ACTION_60   pos = ProcessPixel<2364>(r, c, rccode, chains, pos);
#define ACTION_61   pos = ProcessPixel<3072>(r, c, rccode, chains, pos);
#define ACTION_62   pos = ProcessPixel<3080>(r, c, rccode, chains, pos);
#define ACTION_63   pos = ProcessPixel<3108>(r, c, rccode, chains, pos);
#define ACTION_64   pos = ProcessPixel<3124>(r, c, rccode, chains, pos);
#define ACTION_65   pos = ProcessPixel<3132>(r, c, rccode, chains, pos);

	if (h == 1) {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		c = -1;
		goto sl_tree_0;

#include "Cederberg_Spaghetti_single_line_forest_code.inc.h"

	}
	else {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		goto fl_tree_0;

#include "Cederberg_Spaghetti_first_line_forest_code.inc.h"    

		// Build Raster Scan Chain Code
		for (r = 1; r < h - 1; r++) {

			previous_row_ptr = row_ptr;
			row_ptr = next_row_ptr;
			next_row_ptr += img_.step[0];

			pos = 0;

			c = -1;
			goto cl_tree_0;

#include "Cederberg_Spaghetti_center_line_forest_code.inc.h"

		}

		previous_row_ptr = row_ptr;
		row_ptr = next_row_ptr;
		next_row_ptr += img_.step[0];

		pos = 0;

		c = -1;
		goto ll_tree_0;

#include "Cederberg_Spaghetti_last_line_forest_code.inc.h"

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
#undef ACTION_26
#undef ACTION_27
#undef ACTION_28
#undef ACTION_29
#undef ACTION_30
#undef ACTION_31
#undef ACTION_32
#undef ACTION_33
#undef ACTION_34
#undef ACTION_35
#undef ACTION_36
#undef ACTION_37
#undef ACTION_38
#undef ACTION_39
#undef ACTION_40
#undef ACTION_41
#undef ACTION_42
#undef ACTION_43
#undef ACTION_44
#undef ACTION_45
#undef ACTION_46
#undef ACTION_47
#undef ACTION_48
#undef ACTION_49
#undef ACTION_50
#undef ACTION_51
#undef ACTION_52
#undef ACTION_53
#undef ACTION_54
#undef ACTION_55
#undef ACTION_56
#undef ACTION_57
#undef ACTION_58
#undef ACTION_59
#undef ACTION_60
#undef ACTION_61
#undef ACTION_62
#undef ACTION_63
#undef ACTION_64
#undef ACTION_65

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E
#undef CONDITION_F
#undef CONDITION_G
#undef CONDITION_H


    RCCodeToChainCode(rccode, chain_code_);
}


void Cederberg_Spaghetti_FREQ_All::PerformChainCode() {

	RCCode rccode;

	int w = img_.cols;
	int h = img_.rows;

	vector<unsigned> chains;

	const unsigned char* previous_row_ptr = nullptr;
	const unsigned char* row_ptr = img_.ptr(0);
	const unsigned char* next_row_ptr = row_ptr + img_.step[0];

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           
#define CONDITION_F     (next_row_ptr[c - 1])             
#define CONDITION_G     (next_row_ptr[c])                          
#define CONDITION_H     (next_row_ptr[c + 1]) 

#define ACTION_1    pos = ProcessPixel<0	  >(r, c, rccode, chains, pos);
#define ACTION_2    pos = ProcessPixel<1	  >(r, c, rccode, chains, pos);
#define ACTION_3    pos = ProcessPixel<2	  >(r, c, rccode, chains, pos);
#define ACTION_4    pos = ProcessPixel<3	  >(r, c, rccode, chains, pos);
#define ACTION_5    pos = ProcessPixel<10  >(r, c, rccode, chains, pos);
#define ACTION_6    pos = ProcessPixel<16  >(r, c, rccode, chains, pos);
#define ACTION_7    pos = ProcessPixel<17  >(r, c, rccode, chains, pos);
#define ACTION_8    pos = ProcessPixel<32  >(r, c, rccode, chains, pos);
#define ACTION_9    pos = ProcessPixel<33  >(r, c, rccode, chains, pos);
#define ACTION_10   pos = ProcessPixel<48  >(r, c, rccode, chains, pos);
#define ACTION_11   pos = ProcessPixel<49  >(r, c, rccode, chains, pos);
#define ACTION_12   pos = ProcessPixel<56  >(r, c, rccode, chains, pos);
#define ACTION_13   pos = ProcessPixel<64  >(r, c, rccode, chains, pos);
#define ACTION_14   pos = ProcessPixel<65  >(r, c, rccode, chains, pos);
#define ACTION_15   pos = ProcessPixel<128 >(r, c, rccode, chains, pos);
#define ACTION_16   pos = ProcessPixel<129 >(r, c, rccode, chains, pos);
#define ACTION_17   pos = ProcessPixel<144 >(r, c, rccode, chains, pos);
#define ACTION_18   pos = ProcessPixel<145 >(r, c, rccode, chains, pos);
#define ACTION_19   pos = ProcessPixel<152 >(r, c, rccode, chains, pos);
#define ACTION_20   pos = ProcessPixel<192 >(r, c, rccode, chains, pos);
#define ACTION_21   pos = ProcessPixel<193 >(r, c, rccode, chains, pos);
#define ACTION_22   pos = ProcessPixel<200 >(r, c, rccode, chains, pos);
#define ACTION_23   pos = ProcessPixel<256 >(r, c, rccode, chains, pos);
#define ACTION_24   pos = ProcessPixel<257 >(r, c, rccode, chains, pos);
#define ACTION_25   pos = ProcessPixel<292 >(r, c, rccode, chains, pos);
#define ACTION_26   pos = ProcessPixel<293 >(r, c, rccode, chains, pos);
#define ACTION_27   pos = ProcessPixel<308 >(r, c, rccode, chains, pos);
#define ACTION_28   pos = ProcessPixel<309 >(r, c, rccode, chains, pos);
#define ACTION_29   pos = ProcessPixel<512 >(r, c, rccode, chains, pos);
#define ACTION_30   pos = ProcessPixel<513 >(r, c, rccode, chains, pos);
#define ACTION_31   pos = ProcessPixel<528 >(r, c, rccode, chains, pos);
#define ACTION_32   pos = ProcessPixel<529 >(r, c, rccode, chains, pos);
#define ACTION_33   pos = ProcessPixel<536 >(r, c, rccode, chains, pos);
#define ACTION_34   pos = ProcessPixel<576 >(r, c, rccode, chains, pos);
#define ACTION_35   pos = ProcessPixel<577 >(r, c, rccode, chains, pos);
#define ACTION_36   pos = ProcessPixel<584 >(r, c, rccode, chains, pos);
#define ACTION_37   pos = ProcessPixel<768 >(r, c, rccode, chains, pos);
#define ACTION_38   pos = ProcessPixel<769 >(r, c, rccode, chains, pos);
#define ACTION_39   pos = ProcessPixel<776 >(r, c, rccode, chains, pos);
#define ACTION_40   pos = ProcessPixel<804 >(r, c, rccode, chains, pos);
#define ACTION_41   pos = ProcessPixel<805 >(r, c, rccode, chains, pos);
#define ACTION_42   pos = ProcessPixel<820 >(r, c, rccode, chains, pos);
#define ACTION_43   pos = ProcessPixel<821 >(r, c, rccode, chains, pos);
#define ACTION_44   pos = ProcessPixel<828 >(r, c, rccode, chains, pos);
#define ACTION_45   pos = ProcessPixel<1024>(r, c, rccode, chains, pos);
#define ACTION_46   pos = ProcessPixel<1025>(r, c, rccode, chains, pos);
#define ACTION_47   pos = ProcessPixel<1060>(r, c, rccode, chains, pos);
#define ACTION_48   pos = ProcessPixel<1061>(r, c, rccode, chains, pos);
#define ACTION_49   pos = ProcessPixel<1076>(r, c, rccode, chains, pos);
#define ACTION_50   pos = ProcessPixel<1077>(r, c, rccode, chains, pos);
#define ACTION_51   pos = ProcessPixel<2048>(r, c, rccode, chains, pos);
#define ACTION_52   pos = ProcessPixel<2064>(r, c, rccode, chains, pos);
#define ACTION_53   pos = ProcessPixel<2072>(r, c, rccode, chains, pos);
#define ACTION_54   pos = ProcessPixel<2112>(r, c, rccode, chains, pos);
#define ACTION_55   pos = ProcessPixel<2120>(r, c, rccode, chains, pos);
#define ACTION_56   pos = ProcessPixel<2304>(r, c, rccode, chains, pos);
#define ACTION_57   pos = ProcessPixel<2312>(r, c, rccode, chains, pos);
#define ACTION_58   pos = ProcessPixel<2340>(r, c, rccode, chains, pos);
#define ACTION_59   pos = ProcessPixel<2356>(r, c, rccode, chains, pos);
#define ACTION_60   pos = ProcessPixel<2364>(r, c, rccode, chains, pos);
#define ACTION_61   pos = ProcessPixel<3072>(r, c, rccode, chains, pos);
#define ACTION_62   pos = ProcessPixel<3080>(r, c, rccode, chains, pos);
#define ACTION_63   pos = ProcessPixel<3108>(r, c, rccode, chains, pos);
#define ACTION_64   pos = ProcessPixel<3124>(r, c, rccode, chains, pos);
#define ACTION_65   pos = ProcessPixel<3132>(r, c, rccode, chains, pos);

	if (h == 1) {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		c = -1;
		goto sl_tree_0;

#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical-classical_single_line_forest_code.inc.h"

	}
	else {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		goto fl_tree_0;
			

#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical-classical_first_line_forest_code.inc.h"

		// Build Raster Scan Chain Code
		for (r = 1; r < h - 1; r++) {

			previous_row_ptr = row_ptr;
			row_ptr = next_row_ptr;
			next_row_ptr += img_.step[0];

			pos = 0;

			c = -1;
			goto cl_tree_0;
				
#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical-classical_center_line_forest_code.inc.h"

		}

		previous_row_ptr = row_ptr;
		row_ptr = next_row_ptr;
		next_row_ptr += img_.step[0];

		pos = 0;

		c = -1;
		goto ll_tree_0;

#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical-classical_last_line_forest_code.inc.h"

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
#undef ACTION_26
#undef ACTION_27
#undef ACTION_28
#undef ACTION_29
#undef ACTION_30
#undef ACTION_31
#undef ACTION_32
#undef ACTION_33
#undef ACTION_34
#undef ACTION_35
#undef ACTION_36
#undef ACTION_37
#undef ACTION_38
#undef ACTION_39
#undef ACTION_40
#undef ACTION_41
#undef ACTION_42
#undef ACTION_43
#undef ACTION_44
#undef ACTION_45
#undef ACTION_46
#undef ACTION_47
#undef ACTION_48
#undef ACTION_49
#undef ACTION_50
#undef ACTION_51
#undef ACTION_52
#undef ACTION_53
#undef ACTION_54
#undef ACTION_55
#undef ACTION_56
#undef ACTION_57
#undef ACTION_58
#undef ACTION_59
#undef ACTION_60
#undef ACTION_61
#undef ACTION_62
#undef ACTION_63
#undef ACTION_64
#undef ACTION_65

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E
#undef CONDITION_F
#undef CONDITION_G
#undef CONDITION_H


    RCCodeToChainCode(rccode, chain_code_);
}


void Cederberg_Tree::PerformChainCode() {

	RCCode rccode;

	int w = img_.cols;
	int h = img_.rows;

	vector<unsigned> chains;

	const unsigned char* previous_row_ptr = nullptr;
	const unsigned char* row_ptr = img_.ptr(0);
	const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    {
#define CONDITION_A     (r > 0 && c > 0 && previous_row_ptr[c - 1])                     
#define CONDITION_B     (r > 0 && previous_row_ptr[c])                                  
#define CONDITION_C     (r > 0 && c + 1 < w && previous_row_ptr[c + 1])         
#define CONDITION_D     (c > 0 && row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (c + 1 < w && row_ptr[c + 1])                           
#define CONDITION_F     (r + 1 < h && c > 0 && r + 1 < h && next_row_ptr[c - 1])             
#define CONDITION_G     (r + 1 < h && next_row_ptr[c])                          
#define CONDITION_H     (r + 1 < h && c + 1 < w && next_row_ptr[c + 1])

#define ACTION_1    pos = ProcessPixel<0   >(r, c, rccode, chains, pos);
#define ACTION_2    pos = ProcessPixel<1   >(r, c, rccode, chains, pos);
#define ACTION_3    pos = ProcessPixel<2   >(r, c, rccode, chains, pos);
#define ACTION_4    pos = ProcessPixel<3   >(r, c, rccode, chains, pos);
#define ACTION_5    pos = ProcessPixel<10  >(r, c, rccode, chains, pos);
#define ACTION_6    pos = ProcessPixel<16  >(r, c, rccode, chains, pos);
#define ACTION_7    pos = ProcessPixel<17  >(r, c, rccode, chains, pos);
#define ACTION_8    pos = ProcessPixel<32  >(r, c, rccode, chains, pos);
#define ACTION_9    pos = ProcessPixel<33  >(r, c, rccode, chains, pos);
#define ACTION_10   pos = ProcessPixel<48  >(r, c, rccode, chains, pos);
#define ACTION_11   pos = ProcessPixel<49  >(r, c, rccode, chains, pos);
#define ACTION_12   pos = ProcessPixel<56  >(r, c, rccode, chains, pos);
#define ACTION_13   pos = ProcessPixel<64  >(r, c, rccode, chains, pos);
#define ACTION_14   pos = ProcessPixel<65  >(r, c, rccode, chains, pos);
#define ACTION_15   pos = ProcessPixel<128 >(r, c, rccode, chains, pos);
#define ACTION_16   pos = ProcessPixel<129 >(r, c, rccode, chains, pos);
#define ACTION_17   pos = ProcessPixel<144 >(r, c, rccode, chains, pos);
#define ACTION_18   pos = ProcessPixel<145 >(r, c, rccode, chains, pos);
#define ACTION_19   pos = ProcessPixel<152 >(r, c, rccode, chains, pos);
#define ACTION_20   pos = ProcessPixel<192 >(r, c, rccode, chains, pos);
#define ACTION_21   pos = ProcessPixel<193 >(r, c, rccode, chains, pos);
#define ACTION_22   pos = ProcessPixel<200 >(r, c, rccode, chains, pos);
#define ACTION_23   pos = ProcessPixel<256 >(r, c, rccode, chains, pos);
#define ACTION_24   pos = ProcessPixel<257 >(r, c, rccode, chains, pos);
#define ACTION_25   pos = ProcessPixel<292 >(r, c, rccode, chains, pos);
#define ACTION_26   pos = ProcessPixel<293 >(r, c, rccode, chains, pos);
#define ACTION_27   pos = ProcessPixel<308 >(r, c, rccode, chains, pos);
#define ACTION_28   pos = ProcessPixel<309 >(r, c, rccode, chains, pos);
#define ACTION_29   pos = ProcessPixel<512 >(r, c, rccode, chains, pos);
#define ACTION_30   pos = ProcessPixel<513 >(r, c, rccode, chains, pos);
#define ACTION_31   pos = ProcessPixel<528 >(r, c, rccode, chains, pos);
#define ACTION_32   pos = ProcessPixel<529 >(r, c, rccode, chains, pos);
#define ACTION_33   pos = ProcessPixel<536 >(r, c, rccode, chains, pos);
#define ACTION_34   pos = ProcessPixel<576 >(r, c, rccode, chains, pos);
#define ACTION_35   pos = ProcessPixel<577 >(r, c, rccode, chains, pos);
#define ACTION_36   pos = ProcessPixel<584 >(r, c, rccode, chains, pos);
#define ACTION_37   pos = ProcessPixel<768 >(r, c, rccode, chains, pos);
#define ACTION_38   pos = ProcessPixel<769 >(r, c, rccode, chains, pos);
#define ACTION_39   pos = ProcessPixel<776 >(r, c, rccode, chains, pos);
#define ACTION_40   pos = ProcessPixel<804 >(r, c, rccode, chains, pos);
#define ACTION_41   pos = ProcessPixel<805 >(r, c, rccode, chains, pos);
#define ACTION_42   pos = ProcessPixel<820 >(r, c, rccode, chains, pos);
#define ACTION_43   pos = ProcessPixel<821 >(r, c, rccode, chains, pos);
#define ACTION_44   pos = ProcessPixel<828 >(r, c, rccode, chains, pos);
#define ACTION_45   pos = ProcessPixel<1024>(r, c, rccode, chains, pos);
#define ACTION_46   pos = ProcessPixel<1025>(r, c, rccode, chains, pos);
#define ACTION_47   pos = ProcessPixel<1060>(r, c, rccode, chains, pos);
#define ACTION_48   pos = ProcessPixel<1061>(r, c, rccode, chains, pos);
#define ACTION_49   pos = ProcessPixel<1076>(r, c, rccode, chains, pos);
#define ACTION_50   pos = ProcessPixel<1077>(r, c, rccode, chains, pos);
#define ACTION_51   pos = ProcessPixel<2048>(r, c, rccode, chains, pos);
#define ACTION_52   pos = ProcessPixel<2064>(r, c, rccode, chains, pos);
#define ACTION_53   pos = ProcessPixel<2072>(r, c, rccode, chains, pos);
#define ACTION_54   pos = ProcessPixel<2112>(r, c, rccode, chains, pos);
#define ACTION_55   pos = ProcessPixel<2120>(r, c, rccode, chains, pos);
#define ACTION_56   pos = ProcessPixel<2304>(r, c, rccode, chains, pos);
#define ACTION_57   pos = ProcessPixel<2312>(r, c, rccode, chains, pos);
#define ACTION_58   pos = ProcessPixel<2340>(r, c, rccode, chains, pos);
#define ACTION_59   pos = ProcessPixel<2356>(r, c, rccode, chains, pos);
#define ACTION_60   pos = ProcessPixel<2364>(r, c, rccode, chains, pos);
#define ACTION_61   pos = ProcessPixel<3072>(r, c, rccode, chains, pos);
#define ACTION_62   pos = ProcessPixel<3080>(r, c, rccode, chains, pos);
#define ACTION_63   pos = ProcessPixel<3108>(r, c, rccode, chains, pos);
#define ACTION_64   pos = ProcessPixel<3124>(r, c, rccode, chains, pos);
#define ACTION_65   pos = ProcessPixel<3132>(r, c, rccode, chains, pos);
    }

	for (int r = 0; r < h; r++) {
		unsigned int pos = 0;

		for (int c = 0; c < w; c++) {
#include "Cederberg_Tree_tree_code.inc.h"
		}

		previous_row_ptr = row_ptr;
		row_ptr = next_row_ptr;
		next_row_ptr += img_.step[0];
	}

    {
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
#undef ACTION_26
#undef ACTION_27
#undef ACTION_28
#undef ACTION_29
#undef ACTION_30
#undef ACTION_31
#undef ACTION_32
#undef ACTION_33
#undef ACTION_34
#undef ACTION_35
#undef ACTION_36
#undef ACTION_37
#undef ACTION_38
#undef ACTION_39
#undef ACTION_40
#undef ACTION_41
#undef ACTION_42
#undef ACTION_43
#undef ACTION_44
#undef ACTION_45
#undef ACTION_46
#undef ACTION_47
#undef ACTION_48
#undef ACTION_49
#undef ACTION_50
#undef ACTION_51
#undef ACTION_52
#undef ACTION_53
#undef ACTION_54
#undef ACTION_55
#undef ACTION_56
#undef ACTION_57
#undef ACTION_58
#undef ACTION_59
#undef ACTION_60
#undef ACTION_61
#undef ACTION_62
#undef ACTION_63
#undef ACTION_64
#undef ACTION_65

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E
#undef CONDITION_F
#undef CONDITION_G
#undef CONDITION_H
    }

    RCCodeToChainCode(rccode, chain_code_);
}


void Cederberg_Spaghetti_FREQ_AllNoClassical::PerformChainCode() {

	RCCode rccode;

	int w = img_.cols;
	int h = img_.rows;

	vector<unsigned> chains;

	const unsigned char* previous_row_ptr = nullptr;
	const unsigned char* row_ptr = img_.ptr(0);
	const unsigned char* next_row_ptr = row_ptr + img_.step[0];

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           
#define CONDITION_F     (next_row_ptr[c - 1])             
#define CONDITION_G     (next_row_ptr[c])                          
#define CONDITION_H     (next_row_ptr[c + 1]) 

#define ACTION_1    pos = ProcessPixel<0	  >(r, c, rccode, chains, pos);
#define ACTION_2    pos = ProcessPixel<1	  >(r, c, rccode, chains, pos);
#define ACTION_3    pos = ProcessPixel<2	  >(r, c, rccode, chains, pos);
#define ACTION_4    pos = ProcessPixel<3	  >(r, c, rccode, chains, pos);
#define ACTION_5    pos = ProcessPixel<10  >(r, c, rccode, chains, pos);
#define ACTION_6    pos = ProcessPixel<16  >(r, c, rccode, chains, pos);
#define ACTION_7    pos = ProcessPixel<17  >(r, c, rccode, chains, pos);
#define ACTION_8    pos = ProcessPixel<32  >(r, c, rccode, chains, pos);
#define ACTION_9    pos = ProcessPixel<33  >(r, c, rccode, chains, pos);
#define ACTION_10   pos = ProcessPixel<48  >(r, c, rccode, chains, pos);
#define ACTION_11   pos = ProcessPixel<49  >(r, c, rccode, chains, pos);
#define ACTION_12   pos = ProcessPixel<56  >(r, c, rccode, chains, pos);
#define ACTION_13   pos = ProcessPixel<64  >(r, c, rccode, chains, pos);
#define ACTION_14   pos = ProcessPixel<65  >(r, c, rccode, chains, pos);
#define ACTION_15   pos = ProcessPixel<128 >(r, c, rccode, chains, pos);
#define ACTION_16   pos = ProcessPixel<129 >(r, c, rccode, chains, pos);
#define ACTION_17   pos = ProcessPixel<144 >(r, c, rccode, chains, pos);
#define ACTION_18   pos = ProcessPixel<145 >(r, c, rccode, chains, pos);
#define ACTION_19   pos = ProcessPixel<152 >(r, c, rccode, chains, pos);
#define ACTION_20   pos = ProcessPixel<192 >(r, c, rccode, chains, pos);
#define ACTION_21   pos = ProcessPixel<193 >(r, c, rccode, chains, pos);
#define ACTION_22   pos = ProcessPixel<200 >(r, c, rccode, chains, pos);
#define ACTION_23   pos = ProcessPixel<256 >(r, c, rccode, chains, pos);
#define ACTION_24   pos = ProcessPixel<257 >(r, c, rccode, chains, pos);
#define ACTION_25   pos = ProcessPixel<292 >(r, c, rccode, chains, pos);
#define ACTION_26   pos = ProcessPixel<293 >(r, c, rccode, chains, pos);
#define ACTION_27   pos = ProcessPixel<308 >(r, c, rccode, chains, pos);
#define ACTION_28   pos = ProcessPixel<309 >(r, c, rccode, chains, pos);
#define ACTION_29   pos = ProcessPixel<512 >(r, c, rccode, chains, pos);
#define ACTION_30   pos = ProcessPixel<513 >(r, c, rccode, chains, pos);
#define ACTION_31   pos = ProcessPixel<528 >(r, c, rccode, chains, pos);
#define ACTION_32   pos = ProcessPixel<529 >(r, c, rccode, chains, pos);
#define ACTION_33   pos = ProcessPixel<536 >(r, c, rccode, chains, pos);
#define ACTION_34   pos = ProcessPixel<576 >(r, c, rccode, chains, pos);
#define ACTION_35   pos = ProcessPixel<577 >(r, c, rccode, chains, pos);
#define ACTION_36   pos = ProcessPixel<584 >(r, c, rccode, chains, pos);
#define ACTION_37   pos = ProcessPixel<768 >(r, c, rccode, chains, pos);
#define ACTION_38   pos = ProcessPixel<769 >(r, c, rccode, chains, pos);
#define ACTION_39   pos = ProcessPixel<776 >(r, c, rccode, chains, pos);
#define ACTION_40   pos = ProcessPixel<804 >(r, c, rccode, chains, pos);
#define ACTION_41   pos = ProcessPixel<805 >(r, c, rccode, chains, pos);
#define ACTION_42   pos = ProcessPixel<820 >(r, c, rccode, chains, pos);
#define ACTION_43   pos = ProcessPixel<821 >(r, c, rccode, chains, pos);
#define ACTION_44   pos = ProcessPixel<828 >(r, c, rccode, chains, pos);
#define ACTION_45   pos = ProcessPixel<1024>(r, c, rccode, chains, pos);
#define ACTION_46   pos = ProcessPixel<1025>(r, c, rccode, chains, pos);
#define ACTION_47   pos = ProcessPixel<1060>(r, c, rccode, chains, pos);
#define ACTION_48   pos = ProcessPixel<1061>(r, c, rccode, chains, pos);
#define ACTION_49   pos = ProcessPixel<1076>(r, c, rccode, chains, pos);
#define ACTION_50   pos = ProcessPixel<1077>(r, c, rccode, chains, pos);
#define ACTION_51   pos = ProcessPixel<2048>(r, c, rccode, chains, pos);
#define ACTION_52   pos = ProcessPixel<2064>(r, c, rccode, chains, pos);
#define ACTION_53   pos = ProcessPixel<2072>(r, c, rccode, chains, pos);
#define ACTION_54   pos = ProcessPixel<2112>(r, c, rccode, chains, pos);
#define ACTION_55   pos = ProcessPixel<2120>(r, c, rccode, chains, pos);
#define ACTION_56   pos = ProcessPixel<2304>(r, c, rccode, chains, pos);
#define ACTION_57   pos = ProcessPixel<2312>(r, c, rccode, chains, pos);
#define ACTION_58   pos = ProcessPixel<2340>(r, c, rccode, chains, pos);
#define ACTION_59   pos = ProcessPixel<2356>(r, c, rccode, chains, pos);
#define ACTION_60   pos = ProcessPixel<2364>(r, c, rccode, chains, pos);
#define ACTION_61   pos = ProcessPixel<3072>(r, c, rccode, chains, pos);
#define ACTION_62   pos = ProcessPixel<3080>(r, c, rccode, chains, pos);
#define ACTION_63   pos = ProcessPixel<3108>(r, c, rccode, chains, pos);
#define ACTION_64   pos = ProcessPixel<3124>(r, c, rccode, chains, pos);
#define ACTION_65   pos = ProcessPixel<3132>(r, c, rccode, chains, pos);

	if (h == 1) {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		c = -1;
		goto sl_tree_0;

#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical_single_line_forest_code.inc.h"

	}
	else {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		goto fl_tree_0;

			
#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical_first_line_forest_code.inc.h"   

		// Build Raster Scan Chain Code
		for (r = 1; r < h - 1; r++) {

			previous_row_ptr = row_ptr;
			row_ptr = next_row_ptr;
			next_row_ptr += img_.step[0];

			pos = 0;

			c = -1;
			goto cl_tree_0;

#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical_center_line_forest_code.inc.h"
		}

		previous_row_ptr = row_ptr;
		row_ptr = next_row_ptr;
		next_row_ptr += img_.step[0];

		pos = 0;

		c = -1;
		goto ll_tree_0;

#include "Cederberg_Spaghetti_FREQ_fingerprints-hamlet-3dpes-xdocs-tobacco800-mirflickr-medical_last_line_forest_code.inc.h"

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
#undef ACTION_26
#undef ACTION_27
#undef ACTION_28
#undef ACTION_29
#undef ACTION_30
#undef ACTION_31
#undef ACTION_32
#undef ACTION_33
#undef ACTION_34
#undef ACTION_35
#undef ACTION_36
#undef ACTION_37
#undef ACTION_38
#undef ACTION_39
#undef ACTION_40
#undef ACTION_41
#undef ACTION_42
#undef ACTION_43
#undef ACTION_44
#undef ACTION_45
#undef ACTION_46
#undef ACTION_47
#undef ACTION_48
#undef ACTION_49
#undef ACTION_50
#undef ACTION_51
#undef ACTION_52
#undef ACTION_53
#undef ACTION_54
#undef ACTION_55
#undef ACTION_56
#undef ACTION_57
#undef ACTION_58
#undef ACTION_59
#undef ACTION_60
#undef ACTION_61
#undef ACTION_62
#undef ACTION_63
#undef ACTION_64
#undef ACTION_65

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E
#undef CONDITION_F
#undef CONDITION_G
#undef CONDITION_H


    RCCodeToChainCode(rccode, chain_code_);
}


void Cederberg_Spaghetti_FREQ_Hamlet::PerformChainCode() {

	RCCode rccode;

	int w = img_.cols;
	int h = img_.rows;

	vector<unsigned> chains;

	const unsigned char* previous_row_ptr = nullptr;
	const unsigned char* row_ptr = img_.ptr(0);
	const unsigned char* next_row_ptr = row_ptr + img_.step[0];

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           
#define CONDITION_F     (next_row_ptr[c - 1])             
#define CONDITION_G     (next_row_ptr[c])                          
#define CONDITION_H     (next_row_ptr[c + 1]) 

#define ACTION_1    pos = ProcessPixel<0	  >(r, c, rccode, chains, pos);
#define ACTION_2    pos = ProcessPixel<1	  >(r, c, rccode, chains, pos);
#define ACTION_3    pos = ProcessPixel<2	  >(r, c, rccode, chains, pos);
#define ACTION_4    pos = ProcessPixel<3	  >(r, c, rccode, chains, pos);
#define ACTION_5    pos = ProcessPixel<10  >(r, c, rccode, chains, pos);
#define ACTION_6    pos = ProcessPixel<16  >(r, c, rccode, chains, pos);
#define ACTION_7    pos = ProcessPixel<17  >(r, c, rccode, chains, pos);
#define ACTION_8    pos = ProcessPixel<32  >(r, c, rccode, chains, pos);
#define ACTION_9    pos = ProcessPixel<33  >(r, c, rccode, chains, pos);
#define ACTION_10   pos = ProcessPixel<48  >(r, c, rccode, chains, pos);
#define ACTION_11   pos = ProcessPixel<49  >(r, c, rccode, chains, pos);
#define ACTION_12   pos = ProcessPixel<56  >(r, c, rccode, chains, pos);
#define ACTION_13   pos = ProcessPixel<64  >(r, c, rccode, chains, pos);
#define ACTION_14   pos = ProcessPixel<65  >(r, c, rccode, chains, pos);
#define ACTION_15   pos = ProcessPixel<128 >(r, c, rccode, chains, pos);
#define ACTION_16   pos = ProcessPixel<129 >(r, c, rccode, chains, pos);
#define ACTION_17   pos = ProcessPixel<144 >(r, c, rccode, chains, pos);
#define ACTION_18   pos = ProcessPixel<145 >(r, c, rccode, chains, pos);
#define ACTION_19   pos = ProcessPixel<152 >(r, c, rccode, chains, pos);
#define ACTION_20   pos = ProcessPixel<192 >(r, c, rccode, chains, pos);
#define ACTION_21   pos = ProcessPixel<193 >(r, c, rccode, chains, pos);
#define ACTION_22   pos = ProcessPixel<200 >(r, c, rccode, chains, pos);
#define ACTION_23   pos = ProcessPixel<256 >(r, c, rccode, chains, pos);
#define ACTION_24   pos = ProcessPixel<257 >(r, c, rccode, chains, pos);
#define ACTION_25   pos = ProcessPixel<292 >(r, c, rccode, chains, pos);
#define ACTION_26   pos = ProcessPixel<293 >(r, c, rccode, chains, pos);
#define ACTION_27   pos = ProcessPixel<308 >(r, c, rccode, chains, pos);
#define ACTION_28   pos = ProcessPixel<309 >(r, c, rccode, chains, pos);
#define ACTION_29   pos = ProcessPixel<512 >(r, c, rccode, chains, pos);
#define ACTION_30   pos = ProcessPixel<513 >(r, c, rccode, chains, pos);
#define ACTION_31   pos = ProcessPixel<528 >(r, c, rccode, chains, pos);
#define ACTION_32   pos = ProcessPixel<529 >(r, c, rccode, chains, pos);
#define ACTION_33   pos = ProcessPixel<536 >(r, c, rccode, chains, pos);
#define ACTION_34   pos = ProcessPixel<576 >(r, c, rccode, chains, pos);
#define ACTION_35   pos = ProcessPixel<577 >(r, c, rccode, chains, pos);
#define ACTION_36   pos = ProcessPixel<584 >(r, c, rccode, chains, pos);
#define ACTION_37   pos = ProcessPixel<768 >(r, c, rccode, chains, pos);
#define ACTION_38   pos = ProcessPixel<769 >(r, c, rccode, chains, pos);
#define ACTION_39   pos = ProcessPixel<776 >(r, c, rccode, chains, pos);
#define ACTION_40   pos = ProcessPixel<804 >(r, c, rccode, chains, pos);
#define ACTION_41   pos = ProcessPixel<805 >(r, c, rccode, chains, pos);
#define ACTION_42   pos = ProcessPixel<820 >(r, c, rccode, chains, pos);
#define ACTION_43   pos = ProcessPixel<821 >(r, c, rccode, chains, pos);
#define ACTION_44   pos = ProcessPixel<828 >(r, c, rccode, chains, pos);
#define ACTION_45   pos = ProcessPixel<1024>(r, c, rccode, chains, pos);
#define ACTION_46   pos = ProcessPixel<1025>(r, c, rccode, chains, pos);
#define ACTION_47   pos = ProcessPixel<1060>(r, c, rccode, chains, pos);
#define ACTION_48   pos = ProcessPixel<1061>(r, c, rccode, chains, pos);
#define ACTION_49   pos = ProcessPixel<1076>(r, c, rccode, chains, pos);
#define ACTION_50   pos = ProcessPixel<1077>(r, c, rccode, chains, pos);
#define ACTION_51   pos = ProcessPixel<2048>(r, c, rccode, chains, pos);
#define ACTION_52   pos = ProcessPixel<2064>(r, c, rccode, chains, pos);
#define ACTION_53   pos = ProcessPixel<2072>(r, c, rccode, chains, pos);
#define ACTION_54   pos = ProcessPixel<2112>(r, c, rccode, chains, pos);
#define ACTION_55   pos = ProcessPixel<2120>(r, c, rccode, chains, pos);
#define ACTION_56   pos = ProcessPixel<2304>(r, c, rccode, chains, pos);
#define ACTION_57   pos = ProcessPixel<2312>(r, c, rccode, chains, pos);
#define ACTION_58   pos = ProcessPixel<2340>(r, c, rccode, chains, pos);
#define ACTION_59   pos = ProcessPixel<2356>(r, c, rccode, chains, pos);
#define ACTION_60   pos = ProcessPixel<2364>(r, c, rccode, chains, pos);
#define ACTION_61   pos = ProcessPixel<3072>(r, c, rccode, chains, pos);
#define ACTION_62   pos = ProcessPixel<3080>(r, c, rccode, chains, pos);
#define ACTION_63   pos = ProcessPixel<3108>(r, c, rccode, chains, pos);
#define ACTION_64   pos = ProcessPixel<3124>(r, c, rccode, chains, pos);
#define ACTION_65   pos = ProcessPixel<3132>(r, c, rccode, chains, pos);

	if (h == 1) {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		c = -1;
		goto sl_tree_0;


#include "Cederberg_Spaghetti_FREQ_hamlet_single_line_forest_code.inc.h"

	}
	else {
		int r = 0;
		int c = -1;

		unsigned int pos = 0;

		goto fl_tree_0;

#include "Cederberg_Spaghetti_FREQ_hamlet_first_line_forest_code.inc.h"    

		// Build Raster Scan Chain Code
		for (r = 1; r < h - 1; r++) {

			previous_row_ptr = row_ptr;
			row_ptr = next_row_ptr;
			next_row_ptr += img_.step[0];

			pos = 0;

			c = -1;
			goto cl_tree_0;

#include "Cederberg_Spaghetti_FREQ_hamlet_center_line_forest_code.inc.h"

		}

		previous_row_ptr = row_ptr;
		row_ptr = next_row_ptr;
		next_row_ptr += img_.step[0];

		pos = 0;

		c = -1;
		goto ll_tree_0;

#include "Cederberg_Spaghetti_FREQ_hamlet_last_line_forest_code.inc.h"

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
#undef ACTION_26
#undef ACTION_27
#undef ACTION_28
#undef ACTION_29
#undef ACTION_30
#undef ACTION_31
#undef ACTION_32
#undef ACTION_33
#undef ACTION_34
#undef ACTION_35
#undef ACTION_36
#undef ACTION_37
#undef ACTION_38
#undef ACTION_39
#undef ACTION_40
#undef ACTION_41
#undef ACTION_42
#undef ACTION_43
#undef ACTION_44
#undef ACTION_45
#undef ACTION_46
#undef ACTION_47
#undef ACTION_48
#undef ACTION_49
#undef ACTION_50
#undef ACTION_51
#undef ACTION_52
#undef ACTION_53
#undef ACTION_54
#undef ACTION_55
#undef ACTION_56
#undef ACTION_57
#undef ACTION_58
#undef ACTION_59
#undef ACTION_60
#undef ACTION_61
#undef ACTION_62
#undef ACTION_63
#undef ACTION_64
#undef ACTION_65

#undef CONDITION_A
#undef CONDITION_B
#undef CONDITION_C
#undef CONDITION_D
#undef CONDITION_X
#undef CONDITION_E
#undef CONDITION_F
#undef CONDITION_G
#undef CONDITION_H


    RCCodeToChainCode(rccode, chain_code_);
}



#undef D0A         
#undef D0B         
#undef D1A         
#undef D1B         
#undef D2A         
#undef D2B         
#undef D3A         
#undef D3B         
#undef MIN_OUTER   
#undef MIN_INNER   
#undef MAX_OUTER   
#undef MAX_INNER   

#undef PIXEL_A
#undef PIXEL_B
#undef PIXEL_C
#undef PIXEL_D
#undef PIXEL_X
#undef PIXEL_E
#undef PIXEL_F
#undef PIXEL_G
#undef PIXEL_H

REGISTER_CHAINCODEALG(Cederberg)
REGISTER_CHAINCODEALG(Cederberg_LUT)
REGISTER_CHAINCODEALG(Cederberg_LUT_PRED)
REGISTER_CHAINCODEALG(Cederberg_DRAG)
REGISTER_CHAINCODEALG(Cederberg_Tree)
REGISTER_CHAINCODEALG(Cederberg_Spaghetti)
REGISTER_CHAINCODEALG(Cederberg_Spaghetti_FREQ_All)
REGISTER_CHAINCODEALG(Cederberg_Spaghetti_FREQ_AllNoClassical)
REGISTER_CHAINCODEALG(Cederberg_Spaghetti_FREQ_Hamlet)
