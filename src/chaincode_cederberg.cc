#include "chaincode_cederberg.h"

#include <fstream>
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

    static constexpr const Template templates[14] = {

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

bool FillLookupTable(string filename = "cederberg_lut.inc", string table_name = "table") {

    ofstream os(filename);
    if (!os) {
        return false;
    }

    os << "static const unsigned short " << table_name << "[512] = {\n";

    for (unsigned short i = 0; i < 512; i++) {

        unsigned short state = TemplateCheck::CheckState(i);

        //os << table_name << "[" << i << "] = " << state << ";\n";
        os << "    " << state << ",\n";

    }

    os << "};\n";

    return true;
}

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

    if (state == 10) {
        // state == 10 is the only single-pixel case
        rccode.AddElem(r, c);
        return;
    }

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

        chains.insert(it, make_pair(rccode.Size() - 1, false));
        chains.insert(it, make_pair(rccode.Size() - 1, true));

        last_found_right = true;
    }

    if (state & MAX_INNER) {
        rccode.AddElem(r, c);

        if (last_found_right) {
            it--;
        }

        chains.insert(it, make_pair(rccode.Size() - 1, true));
        chains.insert(it, make_pair(rccode.Size() - 1, false));

        if (last_found_right) {
            it++;
        }
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

void Cederberg_DRAG::PerformChainCode() {

    list<pair<unsigned, bool>> chains;

    int w = img_.cols;
    int h = img_.rows;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = img_.ptr(1);

#define CONDITION_A     (previous_row_ptr[c - 1])                     
#define CONDITION_B     (previous_row_ptr[c])                                  
#define CONDITION_C     (previous_row_ptr[c + 1])         
#define CONDITION_D     (row_ptr[c - 1])                                       
#define CONDITION_X     (row_ptr[c])                                                    
#define CONDITION_E     (row_ptr[c + 1])                           
#define CONDITION_F     (r + 1 < h && next_row_ptr[c - 1])             
#define CONDITION_G     (r + 1 < h && next_row_ptr[c])                          
#define CONDITION_H     (r + 1 < h && next_row_ptr[c + 1]) 

#define ACTION_1    ProcessPixel(r, c, 0	, rccode, chains, it);
#define ACTION_2    ProcessPixel(r, c, 1	, rccode, chains, it);
#define ACTION_3    ProcessPixel(r, c, 2	, rccode, chains, it);
#define ACTION_4    ProcessPixel(r, c, 3	, rccode, chains, it);
#define ACTION_5    ProcessPixel(r, c, 10	, rccode, chains, it);
#define ACTION_6    ProcessPixel(r, c, 16	, rccode, chains, it);
#define ACTION_7    ProcessPixel(r, c, 17	, rccode, chains, it);
#define ACTION_8    ProcessPixel(r, c, 32	, rccode, chains, it);
#define ACTION_9    ProcessPixel(r, c, 33	, rccode, chains, it);
#define ACTION_10   ProcessPixel(r, c, 48	, rccode, chains, it);
#define ACTION_11   ProcessPixel(r, c, 49	, rccode, chains, it);
#define ACTION_12   ProcessPixel(r, c, 56	, rccode, chains, it);
#define ACTION_13   ProcessPixel(r, c, 64  	, rccode, chains, it);
#define ACTION_14   ProcessPixel(r, c, 65	, rccode, chains, it);
#define ACTION_15   ProcessPixel(r, c, 128  , rccode, chains, it);
#define ACTION_16   ProcessPixel(r, c, 129  , rccode, chains, it);
#define ACTION_17   ProcessPixel(r, c, 144  , rccode, chains, it);
#define ACTION_18   ProcessPixel(r, c, 145  , rccode, chains, it);
#define ACTION_19   ProcessPixel(r, c, 152  , rccode, chains, it);
#define ACTION_20   ProcessPixel(r, c, 192  , rccode, chains, it);
#define ACTION_21   ProcessPixel(r, c, 193  , rccode, chains, it);
#define ACTION_22   ProcessPixel(r, c, 200  , rccode, chains, it);
#define ACTION_23   ProcessPixel(r, c, 256  , rccode, chains, it);
#define ACTION_24   ProcessPixel(r, c, 257  , rccode, chains, it);
#define ACTION_25   ProcessPixel(r, c, 292  , rccode, chains, it);
#define ACTION_26   ProcessPixel(r, c, 293  , rccode, chains, it);
#define ACTION_27   ProcessPixel(r, c, 308  , rccode, chains, it);
#define ACTION_28   ProcessPixel(r, c, 309  , rccode, chains, it);
#define ACTION_29   ProcessPixel(r, c, 512  , rccode, chains, it);
#define ACTION_30   ProcessPixel(r, c, 513  , rccode, chains, it);
#define ACTION_31   ProcessPixel(r, c, 528  , rccode, chains, it);
#define ACTION_32   ProcessPixel(r, c, 529  , rccode, chains, it);
#define ACTION_33   ProcessPixel(r, c, 536  , rccode, chains, it);
#define ACTION_34   ProcessPixel(r, c, 576  , rccode, chains, it);
#define ACTION_35   ProcessPixel(r, c, 577  , rccode, chains, it);
#define ACTION_36   ProcessPixel(r, c, 584  , rccode, chains, it);
#define ACTION_37   ProcessPixel(r, c, 768  , rccode, chains, it);
#define ACTION_38   ProcessPixel(r, c, 769  , rccode, chains, it);
#define ACTION_39   ProcessPixel(r, c, 776  , rccode, chains, it);
#define ACTION_40   ProcessPixel(r, c, 804  , rccode, chains, it);
#define ACTION_41   ProcessPixel(r, c, 805  , rccode, chains, it);
#define ACTION_42   ProcessPixel(r, c, 820  , rccode, chains, it);
#define ACTION_43   ProcessPixel(r, c, 821  , rccode, chains, it);
#define ACTION_44   ProcessPixel(r, c, 828  , rccode, chains, it);
#define ACTION_45   ProcessPixel(r, c, 1024 , rccode, chains, it);
#define ACTION_46   ProcessPixel(r, c, 1025 , rccode, chains, it);
#define ACTION_47   ProcessPixel(r, c, 1060 , rccode, chains, it);
#define ACTION_48   ProcessPixel(r, c, 1061 , rccode, chains, it);
#define ACTION_49   ProcessPixel(r, c, 1076 , rccode, chains, it);
#define ACTION_50   ProcessPixel(r, c, 1077 , rccode, chains, it);
#define ACTION_51   ProcessPixel(r, c, 2048 , rccode, chains, it);
#define ACTION_52   ProcessPixel(r, c, 2064 , rccode, chains, it);
#define ACTION_53   ProcessPixel(r, c, 2072 , rccode, chains, it);
#define ACTION_54   ProcessPixel(r, c, 2112 , rccode, chains, it);
#define ACTION_55   ProcessPixel(r, c, 2120 , rccode, chains, it);
#define ACTION_56   ProcessPixel(r, c, 2304 , rccode, chains, it);
#define ACTION_57   ProcessPixel(r, c, 2312 , rccode, chains, it);
#define ACTION_58   ProcessPixel(r, c, 2340 , rccode, chains, it);
#define ACTION_59   ProcessPixel(r, c, 2356 , rccode, chains, it);
#define ACTION_60   ProcessPixel(r, c, 2364 , rccode, chains, it);
#define ACTION_61   ProcessPixel(r, c, 3072 , rccode, chains, it);
#define ACTION_62   ProcessPixel(r, c, 3080 , rccode, chains, it);
#define ACTION_63   ProcessPixel(r, c, 3108 , rccode, chains, it);
#define ACTION_64   ProcessPixel(r, c, 3124 , rccode, chains, it);
#define ACTION_65   ProcessPixel(r, c, 3132 , rccode, chains, it);

    int r = 0;
    int c = -1;

    list<pair<unsigned, bool>>::iterator& it = chains.begin();
    
    goto tree_0;
    
#include "cederberg_forest_first_line.inc"    

    // Build Raster Scan Chain Code
    for (r = 1; r < h; r++) {

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];

        it = chains.begin();
        
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
REGISTER_CHAINCODEALG(Cederberg_DRAG)
