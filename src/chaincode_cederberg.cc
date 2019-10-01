#include "chaincode_cederberg.h"

#include <list>

#include <opencv2/imgproc.hpp>

#include "register.h"

using namespace std;
using namespace cv;


#define PIXEL_H     1u
#define PIXEL_G     2u
#define PIXEL_F     4u
#define PIXEL_E     8u
#define PIXEL_D     16u
#define PIXEL_C     32u
#define PIXEL_B     64u
#define PIXEL_A     128u
#define PIXEL_X     256u

#define D0A             1u
#define D0B             2u
#define D1A             4u
#define D1B             8u
#define D2A             16u
#define D2B             32u
#define D3A             64u
#define D3B             128u
#define MIN_OUTER       256u
#define MIN_INNER       512u
#define MAX_OUTER       1024u
#define MAX_INNER       2048u
#define SINGLE_PIXEL    4096u

struct TemplateCheck {

    using Template = const int8_t[9];

    static constexpr const Template templates[33] = {

        // Chain links
        { 0,  0, -1,
          1,  1, -1,
         -1, -1, -1,
        },
        {-1, -1, -1,
          1,  1, -1,
          0,  0, -1,
        },
        { 1,  0, -1,
         -1,  1, -1,
         -1, -1, -1
        },
        { 1, -1, -1,
          0,  1, -1,
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
        {-1, -1, 1, 1, 1, 0, 0, 0, 0},
        {1, -1, 1, 0, 1, 0, 0, 0, 0},
        {-1, 1, 0, 1, 1, 0, 0, 0, 0},
        {0, 1, 1, 0, 1, 0, 0, 0, 0},
        {1, 1, 0, 0, 1, 0, 0, 0, 0},
        {1, 0, 0, 1, 1, 0, 0, 0, 0},
        {0, 0, 1, 0, 1, 0, 0, 0, 0},
        {0, 1, 0, 0, 1, 0, 0, 0, 0},
        {1, 0, 0, 0, 1, 0, 0, 0, 0},
        {0, 0, 0, 1, 1, 0, 0, 0, 0},

        // Min Points (inner)
        {0, 0, 1, 1, 1, -1, -1, -1, -1},
        {1, 0, 1, -1, 1, -1, -1, -1, -1},

        // Max Points (outer)
        {0,  0,  0,
         0,  1,  1,
         1, -1, -1
        },
        {0,  0,  0,
         0,  1,  0,
         1, -1,  1
        },
        {0,  0,  0,
         0,  1,  1,
         0,  1, -1
        },
        {0,  0,  0,
         0,  1,  0,
         1,  1,  0
        },
        {0,  0,  0,
         0,  1,  0,
         0,  1,  1
        },
        {0,  0,  0,
         0,  1,  1,
         0,  0,  1
        },
        {0,  0,  0,
         0,  1,  0,
         1,  0,  0
        },
        {0,  0,  0,
         0,  1,  0,
         0,  1,  0
        },
        {0,  0,  0,
         0,  1,  0,
         0,  0,  1
        },
        {0,  0,  0,
         0,  1,  1,
         0,  0,  0
        },

        // Max Points (inner)
        { -1, -1, -1, -1, 1, 1, 1, 0, 0 },
        { -1, -1, -1, -1, 1, -1, 1, 0, 1 },

        // Single pixel
        {0, 0, 0,
         0, 1, 0,
         0, 0, 0}

    };

    static bool TemplateMatch(unsigned short condition, unsigned templ) {
        for (uint8_t cond_i = 7, templ_i = 0; templ_i < 9; templ_i++) {

            if (templates[templ][templ_i] != -1) {

                if (templ_i == 4) {
                    // x
                    if (((condition >> 8) & 1u) != templates[templ][templ_i]) {
                        return false;
                    }
                }

                else {
                    // abcdefgh
                    if (((condition >> cond_i) & 1u) != templates[templ][templ_i]) {
                        return false;
                    }
                }
            }

            if (templ_i != 4) {
                cond_i--;
            }
        }

        return true;
    }

    static unsigned short CheckState(unsigned short condition) {

        unsigned short state = 0;

        // Chains' links
        for (unsigned i = 0; i < 8; i++) {

            if (TemplateMatch(condition, i)) {
                state |= (1 << i);
            }

        }

        // Min points (outer)
        for (unsigned i = 0; i < 10; i++) {

            if (TemplateMatch(condition, i + 8)) {
                state |= MIN_OUTER;
                break;
            }

        }

        // Min points (inner)
        for (unsigned i = 0; i < 2; i++) {

            if (TemplateMatch(condition, i + 18)) {
                state |= MIN_INNER;
                break;
            }

        }

        // Max points (outer)
        for (unsigned i = 0; i < 10; i++) {

            if (TemplateMatch(condition, i + 20)) {
                state |= MAX_OUTER;
                break;
            }

        }

        // Max points (inner)
        for (unsigned i = 0; i < 2; i++) {

            if (TemplateMatch(condition, i + 30)) {
                state |= MAX_INNER;
                break;
            }

        }

        // Single pixel
        if (TemplateMatch(condition, 32)) {
            state |= SINGLE_PIXEL;
        }

        return state;
    }

};

void ConnectChains(RCCode& rccode, list<pair<unsigned, bool>>& chains, list<pair<unsigned, bool>>::iterator& it) {

    list<pair<unsigned, bool>>::iterator first_it = it;
    first_it--;
    list<pair<unsigned, bool>>::iterator second_it = first_it;
    second_it--;

    if ((*first_it).second) {
        rccode[(*first_it).first].next = (*second_it).first;
    }
    else {
        rccode[(*second_it).first].next = (*first_it).first;
    }

    // Remove chains from list
    chains.erase(second_it, it);
}

void ProcessPixel(int r, int c, unsigned short state, RCCode& rccode, list<pair<unsigned, bool>>& chains, list<pair<unsigned, bool>>::iterator& it) {

    bool last_found_right = false;

    if ((state & D0A) || (state & D0B)) {
        it--;
    }
    if ((state & D0A) && (state & D0B)) {
        it--;
    }

    // Chains' links
    uint8_t links_found = 0;

    if (state & D0A) {
        rccode[(*it).first][(*it).second].push_back(0);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    if (state & D0B) {
        rccode[(*it).first][(*it).second].push_back(0);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    if (state & D1A) {
        rccode[(*it).first][(*it).second].push_back(1);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    if (state & D1B) {
        rccode[(*it).first][(*it).second].push_back(1);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    if (state & D2A) {
        rccode[(*it).first][(*it).second].push_back(2);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    if (state & D2B) {
        rccode[(*it).first][(*it).second].push_back(2);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    if (state & D3A) {
        rccode[(*it).first][(*it).second].push_back(3);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    if (state & D3B) {
        rccode[(*it).first][(*it).second].push_back(3);
        links_found++;
        last_found_right = (*it).second;
        it++;
    }

    // Min points
    if ((state & MIN_INNER) && (state & MIN_OUTER)) {
        list<pair<unsigned, bool>>::iterator fourth_chain = it;
        fourth_chain--;
        ConnectChains(rccode, chains, fourth_chain);
        ConnectChains(rccode, chains, it);
    }
    else if ((state & MIN_INNER) || (state & MIN_OUTER)) {
        if (links_found == 2) {
            ConnectChains(rccode, chains, it);
        }
        else if (links_found == 3) {
            if ((state & D3A) && (state & D3B)) {
                // Connect first two chains met
                list<pair<unsigned, bool>>::iterator third_chain = it;
                third_chain--;
                ConnectChains(rccode, chains, third_chain);
            }
            else {
                // Connect last two chains met
                ConnectChains(rccode, chains, it);
            }
        }
        else {
            list<pair<unsigned, bool>>::iterator fourth_chain = it;
            fourth_chain--;
            ConnectChains(rccode, chains, fourth_chain);
        }
    }

    if (state & MAX_OUTER) {
        rccode.AddElem(r, c);

        chains.insert(it, make_pair<unsigned, bool>(rccode.Size() - 1, false));
        chains.insert(it, make_pair<unsigned, bool>(rccode.Size() - 1, true));

        last_found_right = true;
    }

    if (state & MAX_INNER) {
        rccode.AddElem(r, c);

        if (last_found_right) {
            it--;
        }

        chains.insert(it, make_pair<unsigned, bool>(static_cast<unsigned>(rccode.Size() - 1), true));
        chains.insert(it, make_pair<unsigned, bool>(static_cast<unsigned>(rccode.Size() - 1), false));

        if (last_found_right) {
            it++;
        }
    }

    if (state & SINGLE_PIXEL) {
        rccode.AddElem(r, c);
    }

}

void Cederberg::PerformChainCode() {

    list<pair<unsigned, bool>> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < img_.rows; r++) {
        list<pair<unsigned, bool>>::iterator& it = chains.begin();

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

            unsigned short state = TemplateCheck::CheckState(condition);

            ProcessPixel(r, c, state, rccode, chains, it);
        }

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    chain_code_ = ChainCode(rccode);
}

void Cederberg_LUT::PerformChainCode() {

#include "cederberg_lut.inc"

    list<pair<unsigned, bool>> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < img_.rows; r++) {
        list<pair<unsigned, bool>>::iterator& it = chains.begin();

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

            unsigned short state = table[condition];

            ProcessPixel(r, c, state, rccode, chains, it);
        }

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    chain_code_ = ChainCode(rccode);
}

void Cederberg_LUT_PRED::PerformChainCode() {

#include "cederberg_lut.inc"

    list<pair<unsigned, bool>> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < img_.rows; r++) {
        list<pair<unsigned, bool>>::iterator& it = chains.begin();

        unsigned short condition = 0;
        unsigned short old_condition = 0;

        // First column
        if (r > 0 && previous_row_ptr[0])                           condition |= PIXEL_B;
        if (r > 0 && 1 < img_.cols && previous_row_ptr[1])           condition |= PIXEL_C;
        if (row_ptr[0])                                             condition |= PIXEL_X;
        if (1 < img_.cols && row_ptr[1])                             condition |= PIXEL_E;
        if (r + 1 < img_.rows && next_row_ptr[0])                    condition |= PIXEL_G;
        if (r + 1 < img_.rows && 1 < img_.cols && next_row_ptr[1])    condition |= PIXEL_H;

        unsigned short state = table[condition];
        ProcessPixel(r, 0, state, rccode, chains, it);
        old_condition = condition;

        // Middle columns
        for (int c = 1; c < img_.cols - 1; c++) {

            condition = 0;
            condition |= ((old_condition & PIXEL_B) << 1);  // A <- B
            condition |= ((old_condition & PIXEL_X) >> 4);  // D <- X
            condition |= ((old_condition & PIXEL_G) << 1);  // F <- G
            condition |= ((old_condition & PIXEL_C) << 1);  // B <- C
            condition |= ((old_condition & PIXEL_E) << 5);  // X <- E
            condition |= ((old_condition & PIXEL_H) << 1);  // G <- H

            if (r > 0 && previous_row_ptr[c + 1])           condition |= PIXEL_C;
            if (row_ptr[c + 1])                             condition |= PIXEL_E;
            if (r + 1 < img_.rows && next_row_ptr[c + 1])    condition |= PIXEL_H;

            state = table[condition];
            ProcessPixel(r, c, state, rccode, chains, it);
            old_condition = condition;
        }

        // Last column
        condition = 0;
        condition |= ((old_condition & PIXEL_B) << 1);  // A <- B
        condition |= ((old_condition & PIXEL_X) >> 4);  // D <- X
        condition |= ((old_condition & PIXEL_G) << 1);  // F <- G
        condition |= ((old_condition & PIXEL_C) << 1);  // B <- C
        condition |= ((old_condition & PIXEL_E) << 5);  // X <- E
        condition |= ((old_condition & PIXEL_H) << 1);  // G <- H

        state = table[condition];
        ProcessPixel(r, img_.cols - 1, state, rccode, chains, it);

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    chain_code_ = ChainCode(rccode);
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
#undef SINGLE_PIXEL

#undef PIXEL_H
#undef PIXEL_G
#undef PIXEL_F
#undef PIXEL_E
#undef PIXEL_D
#undef PIXEL_C
#undef PIXEL_B
#undef PIXEL_A
#undef PIXEL_X

REGISTER_CHAINCODEALG(Cederberg)
REGISTER_CHAINCODEALG(Cederberg_LUT)
REGISTER_CHAINCODEALG(Cederberg_LUT_PRED)
