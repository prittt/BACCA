#include "chaincode_cederberg.h"

#include <fstream>
#include <list>

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

    static constexpr const Template templates[14] = {

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
    first_it--;

    if ((*second_it).second) {
        rccode[(*second_it).first].next = (*first_it).first;
    }
    else {
        rccode[(*first_it).first].next = (*second_it).first;
    }

    // Remove chains from list
    chains.erase(first_it, it);
}

void ConnectChains(RCCode& rccode, list<pair<unsigned, bool>>& chains, list<pair<unsigned, bool>>::iterator& it, bool outer) {

    // outer: first_it is left and second_it is right
    // inner: first_it is right and second_it is left
    list<pair<unsigned, bool>>::iterator first_it = it;
    first_it--;
    list<pair<unsigned, bool>>::iterator second_it = first_it;
    first_it--;

    if (outer) {
        rccode[(*second_it).first].next = (*first_it).first;
    }
    else {
        rccode[(*first_it).first].next = (*second_it).first;
    }

    // Remove chains from list
    chains.erase(first_it, it);
}

// Without information about left/right in list of chains
void ConnectChains(RCCode& rccode, list<unsigned>& chains, list<unsigned>::iterator& it, bool outer) {

    // outer: first_it is left and second_it is right
    // inner: first_it is right and second_it is left
    list<unsigned>::iterator first_it = it;
    first_it--;
    list<unsigned>::iterator second_it = first_it;
    first_it--;

    if (outer) {
        rccode[*second_it].next = *first_it;
    }
    else {
        rccode[*first_it].next = *second_it;
    }

    // Remove chains from list
    chains.erase(first_it, it);
}

void ConnectChains_2(RCCode& rccode, int* new_chains, int i1, int i2) {

    if (new_chains[i1] % 2 == 1) {
        rccode[new_chains[i1] / 2].next = new_chains[i2] / 2;
    }
    else {
        rccode[new_chains[i2] / 2].next = new_chains[i1] / 2;
    }

    // Remove chains from list
    new_chains[i1] = -1;
    new_chains[i2] = -1;
}


void ProcessPixelNaive(int r, int c, unsigned short state, RCCode& rccode, list<unsigned>& chains, list<unsigned>::iterator& it) {

    if (state == 10) {
        // state == 10 is the only single-pixel case
        rccode.AddElem(r, c);
        return;
    }

    bool last_found_right = false;

    if (state & MAX_OUTER) {
        rccode.AddElem(r, c);

        chains.insert(it, rccode.Size() - 1);
        chains.insert(it, rccode.Size() - 1);

        last_found_right = true;
    }
    else {

        uint8_t links_found = 0;

        if ((state & D0A) || (state & D0B)) {
            it--;
        }
        if ((state & D0A) && (state & D0B)) {
            it--;
        }

        // Chains' links
        if (state & D0A) {
            rccode[*it][false].push_back(0);
            last_found_right = false;
            it++;
            links_found++;
        }

        if (state & D0B) {
            rccode[*it][true].push_back(0);
            last_found_right = true;
            it++;
            links_found++;
        }

        if (state & D1A) {
            rccode[*it][false].push_back(1);
            last_found_right = false;
            it++;
            links_found++;
        }

        if (state & D1B) {
            rccode[*it][true].push_back(1);
            last_found_right = true;
            it++;
            links_found++;
        }

        if (state & D2A) {
            rccode[*it][false].push_back(2);
            last_found_right = false;
            it++;
            links_found++;
        }

        if (state & D2B) {
            rccode[*it][true].push_back(2);
            last_found_right = true;
            it++;
            links_found++;
        }

        if (state & D3A) {
            rccode[*it][false].push_back(3);
            last_found_right = false;
            it++;
            links_found++;
        }

        if (state & D3B) {
            rccode[*it][true].push_back(3);
            last_found_right = true;
            it++;
            links_found++;
        }

        // Min points
        if ((state & MIN_INNER) && (state & MIN_OUTER)) {
            // 4 chains met, connect 2nd and 3rd, then 1st and 4th.
            list<unsigned>::iterator fourth_chain = it;
            fourth_chain--;
            ConnectChains(rccode, chains, fourth_chain, false);
            ConnectChains(rccode, chains, it, true);
        }
        else if (state & MIN_OUTER) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains(rccode, chains, it, true);
            }
        }
        else if (state & MIN_INNER) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains(rccode, chains, it, false);
            }
            else if (links_found == 3) {
                if ((state & D3A) && (state & D3B)) {
                    // 3 chains met, connect 1st and 2nd
                    list<unsigned>::iterator third_chain = it;
                    third_chain--;
                    ConnectChains(rccode, chains, third_chain, false);
                }
                else {
                    // 3 chains met, connect 2nd and 3rd
                    ConnectChains(rccode, chains, it, false);
                }
            }
            else {
                // 4 chains met, connect 2nd and 3rd
                list<unsigned>::iterator fourth_chain = it;
                fourth_chain--;
                ConnectChains(rccode, chains, fourth_chain, false);
            }
        }

    }

    if (state & MAX_INNER) {
        rccode.AddElem(r, c);

        if (last_found_right) {
            it--;
        }

        chains.insert(it, rccode.Size() - 1);
        chains.insert(it, rccode.Size() - 1);

        if (last_found_right) {
            it++;
        }
    }

}
/*
void ProcessPixel(int r, int c, unsigned short state, uint8_t links_found, RCCode& rccode, list<pair<unsigned, bool>>& chains, list<pair<unsigned, bool>>::iterator& it) {

    if (state == 10) {
        // state == 10 is the only single-pixel case
        rccode.AddElem(r, c);
        return;
    }

    bool last_found_right = false;

    if (state & MAX_OUTER) {
        rccode.AddElem(r, c);

        chains.insert(it, make_pair(rccode.Size() - 1, false));
        chains.insert(it, make_pair(rccode.Size() - 1, true));

        last_found_right = true;
    }
    else {

        if ((state & D0A) || (state & D0B)) {
            it--;
        }
        if ((state & D0A) && (state & D0B)) {
            it--;
        }

        // Chains' links
        if (state & D0A) {
            rccode[(*it).first][false].push_back(0);
            last_found_right = false;
            it++;
        }

        if (state & D0B) {
            rccode[(*it).first][true].push_back(0);
            last_found_right = true;
            it++;
        }

        if (state & D1A) {
            rccode[(*it).first][false].push_back(1);
            last_found_right = false;
            it++;
        }

        if (state & D1B) {
            rccode[(*it).first][true].push_back(1);
            last_found_right = true;
            it++;
        }

        if (state & D2A) {
            rccode[(*it).first][false].push_back(2);
            last_found_right = false;
            it++;
        }

        if (state & D2B) {
            rccode[(*it).first][true].push_back(2);
            last_found_right = true;
            it++;
        }

        if (state & D3A) {
            rccode[(*it).first][false].push_back(3);
            last_found_right = false;
            it++;
        }

        if (state & D3B) {
            rccode[(*it).first][true].push_back(3);
            last_found_right = true;
            it++;
        }

        // Min points
        if ((state & MIN_INNER) && (state & MIN_OUTER)) {
            // 4 chains met, connect 2nd and 3rd, then 1st and 4th.
            list<pair<unsigned, bool>>::iterator fourth_chain = it;
            fourth_chain--;
            ConnectChains(rccode, chains, fourth_chain, false);
            ConnectChains(rccode, chains, it, true);
        }
        else if (state & MIN_OUTER) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains(rccode, chains, it, true);
            }
        }
        else if (state & MIN_INNER) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains(rccode, chains, it, false);
            }
            else if (links_found == 3) {
                if ((state & D3A) && (state & D3B)) {
                    // 3 chains met, connect 1st and 2nd
                    list<pair<unsigned, bool>>::iterator third_chain = it;
                    third_chain--;
                    ConnectChains(rccode, chains, third_chain, false);
                }
                else {
                    // 3 chains met, connect 2nd and 3rd
                    ConnectChains(rccode, chains, it, false);
                }
            }
            else {
                // 4 chains met, connect 2nd and 3rd
                list<pair<unsigned, bool>>::iterator fourth_chain = it;
                fourth_chain--;
                ConnectChains(rccode, chains, fourth_chain, false);
            }
        }

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
*/
// Without information about left/right in list of chains
inline void ProcessPixel(int r, int c, unsigned short state, uint8_t links_found, RCCode& rccode, list<unsigned>& chains, list<unsigned>::iterator& it) {

    if (state == 10) {
        // state == 10 is the only single-pixel case
        rccode.AddElem(r, c);
        return;
    }

    bool last_found_right = false;

    if (state & MAX_OUTER) {
        rccode.AddElem(r, c);

        chains.insert(it, rccode.Size() - 1);
        chains.insert(it, rccode.Size() - 1);

        last_found_right = true;
    }
    else {

        if ((state & D0A) || (state & D0B)) {
            it--;
        }
        if ((state & D0A) && (state & D0B)) {
            it--;
        }

        // Chains' links
        if (state & D0A) {
            rccode[*it][false].push_back(0);
            last_found_right = false;
            it++;
        }

        if (state & D0B) {
            rccode[*it][true].push_back(0);
            last_found_right = true;
            it++;
        }

        if (state & D1A) {
            rccode[*it][false].push_back(1);
            last_found_right = false;
            it++;
        }

        if (state & D1B) {
            rccode[*it][true].push_back(1);
            last_found_right = true;
            it++;
        }

        if (state & D2A) {
            rccode[*it][false].push_back(2);
            last_found_right = false;
            it++;
        }

        if (state & D2B) {
            rccode[*it][true].push_back(2);
            last_found_right = true;
            it++;
        }

        if (state & D3A) {
            rccode[*it][false].push_back(3);
            last_found_right = false;
            it++;
        }

        if (state & D3B) {
            rccode[*it][true].push_back(3);
            last_found_right = true;
            it++;
        }

        // Min points
        if ((state & MIN_INNER) && (state & MIN_OUTER)) {
            // 4 chains met, connect 2nd and 3rd, then 1st and 4th.
            list<unsigned>::iterator fourth_chain = it;
            fourth_chain--;
            ConnectChains(rccode, chains, fourth_chain, false);
            ConnectChains(rccode, chains, it, true);
        }
        else if (state & MIN_OUTER) {
            // 2 chains met, connect them
            ConnectChains(rccode, chains, it, true);
        }
        else if (state & MIN_INNER) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains(rccode, chains, it, false);
            }
            else if (links_found == 3) {
                if ((state & D3A) && (state & D3B)) {
                    // 3 chains met, connect 1st and 2nd
                    list<unsigned>::iterator third_chain = it;
                    third_chain--;
                    ConnectChains(rccode, chains, third_chain, false);
                }
                else {
                    // 3 chains met, connect 2nd and 3rd
                    ConnectChains(rccode, chains, it, false);
                }
            }
            else {
                // 4 chains met, connect 2nd and 3rd
                list<unsigned>::iterator fourth_chain = it;
                fourth_chain--;
                ConnectChains(rccode, chains, fourth_chain, false);
            }
        }

    }

    if (state & MAX_INNER) {
        rccode.AddElem(r, c);

        if (last_found_right) {
            it--;
        }

        chains.insert(it, rccode.Size() - 1);
        chains.insert(it, rccode.Size() - 1);

        if (last_found_right) {
            it++;
        }
    }

}

unsigned NextChain(vector<int>& chains, int i) {
    i++;
    int k = i;
    while (i < chains.size() && chains[i] == -1) {
        i++;
    }
    if (i - k > 12) {
        int h = 0;
    }
    return i;
}

unsigned PrevChain(vector<int>& chains, int i) {
    i--;
    while (chains[i] == -1) {
        i--;
    }
    return i;
}

void ProcessPixel_2(int r, int c, unsigned short state, RCCode& rccode, vector<int>& chains, vector<int>& next_chains, int& it) {

    // sinistra pari, destra dispari

    if (state == 10) {
        // state == 10 is the only single-pixel case
        rccode.AddElem(r, c);
        return;
    }

    int new_chains[4] = { -1, -1, -1, -1 };

    uint8_t links_found = 0;

    bool last_found_right = false;

    if (state & MAX_OUTER) {
        rccode.AddElem(r, c);

        new_chains[0] = (rccode.Size() - 1) * 2;
        new_chains[3] = (rccode.Size() - 1) * 2 + 1;

        last_found_right = true;
        links_found = 2;
    }
    else {

        int i_0 = c * 4;
        int i = c * 4 - 5;

        if ((state & D0A) || (state & D0B)) {
            i_0 = PrevChain(next_chains, i_0);
        }
        if ((state & D0A) && (state & D0B)) {
            i_0 = PrevChain(next_chains, i_0);
        }

        // Chains' links
        if (state & D0A) {
            rccode[next_chains[i_0] / 2][false].push_back(0);
            last_found_right = false;
            new_chains[links_found] = next_chains[i_0];
            next_chains[i_0] = -1;
            links_found++;
            if (state & D0B) {
                i_0 = NextChain(next_chains, i_0);
            }
        }

        if (state & D0B) {
            rccode[next_chains[i_0] / 2][true].push_back(0);
            last_found_right = true;
            new_chains[links_found] = next_chains[i_0];
            next_chains[i_0] = -1;
            links_found++;
        }

        if (state & D1A) {
            i = NextChain(chains, i);
            rccode[chains[i] / 2][false].push_back(1);
            last_found_right = false;
            new_chains[links_found] = chains[i];
            chains[i] = -1;
            links_found++;
        }

        if (state & D1B) {
            i = NextChain(chains, i);
            rccode[chains[i] / 2][true].push_back(1);
            last_found_right = true;
            new_chains[links_found] = chains[i];
            chains[i] = -1;
            links_found++;
        }

        if (state & D2A) {
            i = NextChain(chains, i);
            rccode[chains[i] / 2][false].push_back(2);
            last_found_right = false;
            new_chains[links_found] = chains[i];
            chains[i] = -1;
            links_found++;
        }

        if (state & D2B) {
            i = NextChain(chains, i);
            rccode[chains[i] / 2][true].push_back(2);
            last_found_right = true;
            new_chains[links_found] = chains[i];
            chains[i] = -1;
            links_found++;
        }

        if (state & D3A) {
            i = NextChain(chains, i);
            rccode[chains[i] / 2][false].push_back(3);
            last_found_right = false;
            new_chains[links_found] = chains[i];
            chains[i] = -1;
            links_found++;
        }

        if (state & D3B) {
            i = NextChain(chains, i);
            rccode[chains[i] / 2][true].push_back(3);
            last_found_right = true;
            new_chains[links_found] = chains[i];
            chains[i] = -1;
            links_found++;
        }

        // Min points
        if ((state & MIN_INNER) && (state & MIN_OUTER)) {
            // 4 chains met, connect 2nd and 3rd, then 1st and 4th.
            ConnectChains_2(rccode, new_chains, 1, 2);
            ConnectChains_2(rccode, new_chains, 0, 3);
            links_found = 0;
        }
        else if ((state & MIN_INNER) || (state & MIN_OUTER)) {
            if (links_found == 2) {
                // 2 chains met, connect them
                ConnectChains_2(rccode, new_chains, 0, 1);
                links_found = 0;
            }
            else if (links_found == 3) {
                if ((state & D3A) && (state & D3B)) {
                    // 3 chains met, connect 1st and 2nd
                    ConnectChains_2(rccode, new_chains, 0, 1);
                    new_chains[0] = new_chains[2];
                    new_chains[2] = -1;
                }
                else {
                    // 3 chains met, connect 2nd and 3rd
                    ConnectChains_2(rccode, new_chains, 1, 2);
                }
                links_found = 1;
            }
            else {
                // 4 chains met, connect 2nd and 3rd
                ConnectChains_2(rccode, new_chains, 1, 2);
                new_chains[1] = new_chains[3];
                new_chains[3] = -1;
                links_found = 2;
            }
        }

    }

    if (state & MAX_INNER) {
        rccode.AddElem(r, c);

        if (links_found == 0 || (state & MAX_INNER)) {
            new_chains[1] = (rccode.Size() - 1) * 2 + 1;
            new_chains[2] = (rccode.Size() - 1) * 2;
        }
        else if (links_found == 1) {
            new_chains[1] = (rccode.Size() - 1) * 2 + 1;
            new_chains[2] = (rccode.Size() - 1) * 2;
            if (last_found_right) {
                new_chains[3] = new_chains[0];
                new_chains[0] = -1;
            }
        }
        else if (links_found == 2) {
            if (last_found_right) {
                new_chains[3] = new_chains[1];
                new_chains[1] = (rccode.Size() - 1) * 2 + 1;
                new_chains[2] = (rccode.Size() - 1) * 2;
            }
            else {
                new_chains[2] = (rccode.Size() - 1) * 2 + 1;
                new_chains[3] = (rccode.Size() - 1) * 2;
            }
        }

        links_found += 2;
    }


    memcpy(next_chains.data() + c * 4, new_chains, 4 * sizeof(int));

}


void Cederberg::PerformChainCode() {

    list<unsigned> chains;

    int h = img_.rows;
    int w = img_.cols;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < h; r++) {
        list<unsigned>::iterator& it = chains.begin();

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

            ProcessPixelNaive(r, c, state, rccode, chains, it);
        }

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    chain_code_ = ChainCode(rccode);
}

void Cederberg_LUT::PerformChainCode() {

#include "cederberg_lut.inc"

    list<unsigned> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < img_.rows; r++) {
        list<unsigned>::iterator& it = chains.begin();

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

            ProcessPixelNaive(r, c, state, rccode, chains, it);
        }

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    chain_code_ = ChainCode(rccode);
}

void Cederberg_LUT_PRED::PerformChainCode() {

#include "cederberg_lut.inc"

    list<unsigned> chains;

    const unsigned char* previous_row_ptr = nullptr;
    const unsigned char* row_ptr = img_.ptr(0);
    const unsigned char* next_row_ptr = row_ptr + img_.step[0];

    // Build Raster Scan Chain Code
    for (int r = 0; r < img_.rows; r++) {
        list<unsigned>::iterator& it = chains.begin();

        unsigned short condition = 0;

        // First column
        if (r > 0 && previous_row_ptr[0])                           condition |= PIXEL_B;
        if (r > 0 && 1 < img_.cols && previous_row_ptr[1])           condition |= PIXEL_C;
        if (row_ptr[0])                                             condition |= PIXEL_X;
        if (1 < img_.cols && row_ptr[1])                             condition |= PIXEL_E;
        if (r + 1 < img_.rows && next_row_ptr[0])                    condition |= PIXEL_G;
        if (r + 1 < img_.rows && 1 < img_.cols && next_row_ptr[1])    condition |= PIXEL_H;

        unsigned short state = table[condition];
        ProcessPixelNaive(r, 0, state, rccode, chains, it);

        // Middle columns
        for (int c = 1; c < img_.cols - 1; c++) {

            condition >>= 1;
            condition &= ~(PIXEL_C | PIXEL_E | PIXEL_H);

            if (r > 0 && previous_row_ptr[c + 1])           condition |= PIXEL_C;
            if (row_ptr[c + 1])                             condition |= PIXEL_E;
            if (r + 1 < img_.rows && next_row_ptr[c + 1])    condition |= PIXEL_H;

            state = table[condition];
            ProcessPixelNaive(r, c, state, rccode, chains, it);
        }

        // Last column
        condition >>= 1;
        condition &= ~(PIXEL_C | PIXEL_E | PIXEL_H);

        state = table[condition];
        ProcessPixelNaive(r, img_.cols - 1, state, rccode, chains, it);

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];
    }

    chain_code_ = ChainCode(rccode);
}

void Cederberg_DRAG::PerformChainCode() {

    RCCode rccode;
    //rccode.data.reserve(500);

    list<unsigned> chains;

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

#define ACTION_1    ProcessPixel(r, c, 0	, 0, rccode, chains, it);
#define ACTION_2    ProcessPixel(r, c, 1	, 0, rccode, chains, it);
#define ACTION_3    ProcessPixel(r, c, 2	, 0, rccode, chains, it);
#define ACTION_4    ProcessPixel(r, c, 3	, 0, rccode, chains, it);
#define ACTION_5    ProcessPixel(r, c, 10	, 0, rccode, chains, it);
#define ACTION_6    ProcessPixel(r, c, 16	, 1, rccode, chains, it);
#define ACTION_7    ProcessPixel(r, c, 17	, 1, rccode, chains, it);
#define ACTION_8    ProcessPixel(r, c, 32	, 1, rccode, chains, it);
#define ACTION_9    ProcessPixel(r, c, 33	, 1, rccode, chains, it);
#define ACTION_10   ProcessPixel(r, c, 48	, 2, rccode, chains, it);
#define ACTION_11   ProcessPixel(r, c, 49	, 2, rccode, chains, it);
#define ACTION_12   ProcessPixel(r, c, 56	, 2, rccode, chains, it);
#define ACTION_13   ProcessPixel(r, c, 64  	, 1, rccode, chains, it);
#define ACTION_14   ProcessPixel(r, c, 65	, 1, rccode, chains, it);
#define ACTION_15   ProcessPixel(r, c, 128  , 1, rccode, chains, it);
#define ACTION_16   ProcessPixel(r, c, 129  , 1, rccode, chains, it);
#define ACTION_17   ProcessPixel(r, c, 144  , 2, rccode, chains, it);
#define ACTION_18   ProcessPixel(r, c, 145  , 2, rccode, chains, it);
#define ACTION_19   ProcessPixel(r, c, 152  , 2, rccode, chains, it);
#define ACTION_20   ProcessPixel(r, c, 192  , 2, rccode, chains, it);
#define ACTION_21   ProcessPixel(r, c, 193  , 2, rccode, chains, it);
#define ACTION_22   ProcessPixel(r, c, 200  , 2, rccode, chains, it);
#define ACTION_23   ProcessPixel(r, c, 256  , 1, rccode, chains, it);
#define ACTION_24   ProcessPixel(r, c, 257  , 1, rccode, chains, it);
#define ACTION_25   ProcessPixel(r, c, 292  , 2, rccode, chains, it);
#define ACTION_26   ProcessPixel(r, c, 293  , 2, rccode, chains, it);
#define ACTION_27   ProcessPixel(r, c, 308  , 3, rccode, chains, it);
#define ACTION_28   ProcessPixel(r, c, 309  , 3, rccode, chains, it);
#define ACTION_29   ProcessPixel(r, c, 512  , 1, rccode, chains, it);
#define ACTION_30   ProcessPixel(r, c, 513  , 1, rccode, chains, it);
#define ACTION_31   ProcessPixel(r, c, 528  , 2, rccode, chains, it);
#define ACTION_32   ProcessPixel(r, c, 529  , 2, rccode, chains, it);
#define ACTION_33   ProcessPixel(r, c, 536  , 2, rccode, chains, it);
#define ACTION_34   ProcessPixel(r, c, 576  , 2, rccode, chains, it);
#define ACTION_35   ProcessPixel(r, c, 577  , 2, rccode, chains, it);
#define ACTION_36   ProcessPixel(r, c, 584  , 2, rccode, chains, it);
#define ACTION_37   ProcessPixel(r, c, 768  , 2, rccode, chains, it);
#define ACTION_38   ProcessPixel(r, c, 769  , 2, rccode, chains, it);
#define ACTION_39   ProcessPixel(r, c, 776  , 2, rccode, chains, it);
#define ACTION_40   ProcessPixel(r, c, 804  , 3, rccode, chains, it);
#define ACTION_41   ProcessPixel(r, c, 805  , 3, rccode, chains, it);
#define ACTION_42   ProcessPixel(r, c, 820  , 4, rccode, chains, it);
#define ACTION_43   ProcessPixel(r, c, 821  , 4, rccode, chains, it);
#define ACTION_44   ProcessPixel(r, c, 828  , 4, rccode, chains, it);
#define ACTION_45   ProcessPixel(r, c, 1024 , 1, rccode, chains, it);
#define ACTION_46   ProcessPixel(r, c, 1025 , 1, rccode, chains, it);
#define ACTION_47   ProcessPixel(r, c, 1060 , 2, rccode, chains, it);
#define ACTION_48   ProcessPixel(r, c, 1061 , 2, rccode, chains, it);
#define ACTION_49   ProcessPixel(r, c, 1076 , 3, rccode, chains, it);
#define ACTION_50   ProcessPixel(r, c, 1077 , 3, rccode, chains, it);
#define ACTION_51   ProcessPixel(r, c, 2048 , 1, rccode, chains, it);
#define ACTION_52   ProcessPixel(r, c, 2064 , 2, rccode, chains, it);
#define ACTION_53   ProcessPixel(r, c, 2072 , 2, rccode, chains, it);
#define ACTION_54   ProcessPixel(r, c, 2112 , 2, rccode, chains, it);
#define ACTION_55   ProcessPixel(r, c, 2120 , 2, rccode, chains, it);
#define ACTION_56   ProcessPixel(r, c, 2304 , 2, rccode, chains, it);
#define ACTION_57   ProcessPixel(r, c, 2312 , 2, rccode, chains, it);
#define ACTION_58   ProcessPixel(r, c, 2340 , 3, rccode, chains, it);
#define ACTION_59   ProcessPixel(r, c, 2356 , 4, rccode, chains, it);
#define ACTION_60   ProcessPixel(r, c, 2364 , 4, rccode, chains, it);
#define ACTION_61   ProcessPixel(r, c, 3072 , 2, rccode, chains, it);
#define ACTION_62   ProcessPixel(r, c, 3080 , 2, rccode, chains, it);
#define ACTION_63   ProcessPixel(r, c, 3108 , 3, rccode, chains, it);
#define ACTION_64   ProcessPixel(r, c, 3124 , 4, rccode, chains, it);
#define ACTION_65   ProcessPixel(r, c, 3132 , 4, rccode, chains, it);

    int r = 0;
    int c = -1;

    list<unsigned>::iterator& it = chains.begin();

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
/*
// Slower than DRAG
void Cederberg_DRAG_2::PerformChainCode() {

    int w = img_.cols;
    int h = img_.rows;

    vector<int> chains1(w * 4, -1);
    vector<int> chains2(w * 4, -1);

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

#define ACTION_1    ProcessPixel_2(r, c, 0	, rccode, chains1, chains2, it);
#define ACTION_2    ProcessPixel_2(r, c, 1	, rccode, chains1, chains2, it);
#define ACTION_3    ProcessPixel_2(r, c, 2	, rccode, chains1, chains2, it);
#define ACTION_4    ProcessPixel_2(r, c, 3	, rccode, chains1, chains2, it);
#define ACTION_5    ProcessPixel_2(r, c, 10	, rccode, chains1, chains2, it);
#define ACTION_6    ProcessPixel_2(r, c, 16	, rccode, chains1, chains2, it);
#define ACTION_7    ProcessPixel_2(r, c, 17	, rccode, chains1, chains2, it);
#define ACTION_8    ProcessPixel_2(r, c, 32	, rccode, chains1, chains2, it);
#define ACTION_9    ProcessPixel_2(r, c, 33	, rccode, chains1, chains2, it);
#define ACTION_10   ProcessPixel_2(r, c, 48	, rccode, chains1, chains2, it);
#define ACTION_11   ProcessPixel_2(r, c, 49	, rccode, chains1, chains2, it);
#define ACTION_12   ProcessPixel_2(r, c, 56	, rccode, chains1, chains2, it);
#define ACTION_13   ProcessPixel_2(r, c, 64  	, rccode, chains1, chains2, it);
#define ACTION_14   ProcessPixel_2(r, c, 65	,  rccode, chains1, chains2, it);
#define ACTION_15   ProcessPixel_2(r, c, 128  , rccode, chains1, chains2, it);
#define ACTION_16   ProcessPixel_2(r, c, 129  , rccode, chains1, chains2, it);
#define ACTION_17   ProcessPixel_2(r, c, 144  , rccode, chains1, chains2, it);
#define ACTION_18   ProcessPixel_2(r, c, 145  , rccode, chains1, chains2, it);
#define ACTION_19   ProcessPixel_2(r, c, 152  , rccode, chains1, chains2, it);
#define ACTION_20   ProcessPixel_2(r, c, 192  , rccode, chains1, chains2, it);
#define ACTION_21   ProcessPixel_2(r, c, 193  , rccode, chains1, chains2, it);
#define ACTION_22   ProcessPixel_2(r, c, 200  , rccode, chains1, chains2, it);
#define ACTION_23   ProcessPixel_2(r, c, 256  , rccode, chains1, chains2, it);
#define ACTION_24   ProcessPixel_2(r, c, 257  , rccode, chains1, chains2, it);
#define ACTION_25   ProcessPixel_2(r, c, 292  , rccode, chains1, chains2, it);
#define ACTION_26   ProcessPixel_2(r, c, 293  , rccode, chains1, chains2, it);
#define ACTION_27   ProcessPixel_2(r, c, 308  , rccode, chains1, chains2, it);
#define ACTION_28   ProcessPixel_2(r, c, 309  , rccode, chains1, chains2, it);
#define ACTION_29   ProcessPixel_2(r, c, 512  , rccode, chains1, chains2, it);
#define ACTION_30   ProcessPixel_2(r, c, 513  , rccode, chains1, chains2, it);
#define ACTION_31   ProcessPixel_2(r, c, 528  , rccode, chains1, chains2, it);
#define ACTION_32   ProcessPixel_2(r, c, 529  , rccode, chains1, chains2, it);
#define ACTION_33   ProcessPixel_2(r, c, 536  , rccode, chains1, chains2, it);
#define ACTION_34   ProcessPixel_2(r, c, 576  , rccode, chains1, chains2, it);
#define ACTION_35   ProcessPixel_2(r, c, 577  , rccode, chains1, chains2, it);
#define ACTION_36   ProcessPixel_2(r, c, 584  , rccode, chains1, chains2, it);
#define ACTION_37   ProcessPixel_2(r, c, 768  , rccode, chains1, chains2, it);
#define ACTION_38   ProcessPixel_2(r, c, 769  , rccode, chains1, chains2, it);
#define ACTION_39   ProcessPixel_2(r, c, 776  , rccode, chains1, chains2, it);
#define ACTION_40   ProcessPixel_2(r, c, 804  , rccode, chains1, chains2, it);
#define ACTION_41   ProcessPixel_2(r, c, 805  , rccode, chains1, chains2, it);
#define ACTION_42   ProcessPixel_2(r, c, 820  , rccode, chains1, chains2, it);
#define ACTION_43   ProcessPixel_2(r, c, 821  , rccode, chains1, chains2, it);
#define ACTION_44   ProcessPixel_2(r, c, 828  , rccode, chains1, chains2, it);
#define ACTION_45   ProcessPixel_2(r, c, 1024 , rccode, chains1, chains2, it);
#define ACTION_46   ProcessPixel_2(r, c, 1025 , rccode, chains1, chains2, it);
#define ACTION_47   ProcessPixel_2(r, c, 1060 , rccode, chains1, chains2, it);
#define ACTION_48   ProcessPixel_2(r, c, 1061 , rccode, chains1, chains2, it);
#define ACTION_49   ProcessPixel_2(r, c, 1076 , rccode, chains1, chains2, it);
#define ACTION_50   ProcessPixel_2(r, c, 1077 , rccode, chains1, chains2, it);
#define ACTION_51   ProcessPixel_2(r, c, 2048 , rccode, chains1, chains2, it);
#define ACTION_52   ProcessPixel_2(r, c, 2064 , rccode, chains1, chains2, it);
#define ACTION_53   ProcessPixel_2(r, c, 2072 , rccode, chains1, chains2, it);
#define ACTION_54   ProcessPixel_2(r, c, 2112 , rccode, chains1, chains2, it);
#define ACTION_55   ProcessPixel_2(r, c, 2120 , rccode, chains1, chains2, it);
#define ACTION_56   ProcessPixel_2(r, c, 2304 , rccode, chains1, chains2, it);
#define ACTION_57   ProcessPixel_2(r, c, 2312 , rccode, chains1, chains2, it);
#define ACTION_58   ProcessPixel_2(r, c, 2340 , rccode, chains1, chains2, it);
#define ACTION_59   ProcessPixel_2(r, c, 2356 , rccode, chains1, chains2, it);
#define ACTION_60   ProcessPixel_2(r, c, 2364 , rccode, chains1, chains2, it);
#define ACTION_61   ProcessPixel_2(r, c, 3072 , rccode, chains1, chains2, it);
#define ACTION_62   ProcessPixel_2(r, c, 3080 , rccode, chains1, chains2, it);
#define ACTION_63   ProcessPixel_2(r, c, 3108 , rccode, chains1, chains2, it);
#define ACTION_64   ProcessPixel_2(r, c, 3124 , rccode, chains1, chains2, it);
#define ACTION_65   ProcessPixel_2(r, c, 3132 , rccode, chains1, chains2, it);

    int r = 0;
    int c = -1;

    int it = w * 4;

    goto tree_0;

#include "cederberg_forest_first_line.inc"    

    // Build Raster Scan Chain Code
    for (r = 1; r < h; r++) {
        swap(chains1, chains2);

        previous_row_ptr = row_ptr;
        row_ptr = next_row_ptr;
        next_row_ptr += img_.step[0];

        it = -1;
        //it = NextChain(chains1, it);

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
*/

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
//REGISTER_CHAINCODEALG(Cederberg_DRAG_2)
