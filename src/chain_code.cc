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


#include "chain_code.h"

using namespace std;


ChainCode::ChainCode(const std::vector<std::vector<cv::Point>>& contours, bool contrary) {
    for (const std::vector<cv::Point>& contour : contours) {

        // Find top-left value
        int top = 0;
        cv::Point max = contour.front();
        for (unsigned int i = 1; i < contour.size(); i++) {
            if (contour[i].y < max.y || (contour[i].y == max.y && contour[i].x < max.x)) {
                max = contour[i];
                top = i;
            }
        }

        Chain chain(max.y, max.x);
        cv::Point prev = max;

        if (contour.size() > 1) {

            if (contrary) {
                for (int i = top - 1 + static_cast<int>(contour.size()); i >= top; i--) { // OpenCV order is reversed

                    cv::Point cur = contour[i % contour.size()];
                    cv::Point diff = cur - prev;

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
                for (unsigned int i = top + 1; i <= top + contour.size(); i++) {

                    cv::Point cur = contour[i % contour.size()];
                    cv::Point diff = cur - prev;

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

        chains.push_back(chain);
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

void ChainCode::Chain::AddRightChain(const RCCode::Chain& chain) {

    for (unsigned i = 0; i < chain.value_count; i++) {
        uint8_t val = chain.get_value(i);
        uint8_t new_val;
        if (val == 0) {
            new_val = 0;
        }
        else {
            new_val = 8 - val;
        }
        push_back(new_val);
    }

}

void ChainCode::Chain::AddLeftChain(const RCCode::Chain& chain) {

    for (int i = static_cast<int>(chain.value_count) - 1; i >= 0; i--) {
        uint8_t val = 4 - chain.get_value(i);
        push_back(val);
    }
}

void ChainCode::AddChain(const RCCode& rccode, vector<int>& used_elems, unsigned pos) {

    int chain_pos = static_cast<int>(chains.size());

    Chain new_chain(rccode[pos].row, rccode[pos].col);

    new_chain.AddRightChain(rccode[pos].right);

    used_elems[pos] = chain_pos;

    while (true) {

        pos = rccode[pos].next;
        new_chain.AddLeftChain(rccode[pos].left);

        if (used_elems[pos] != -1) {
            break;
        }

        new_chain.AddRightChain(rccode[pos].right);

        used_elems[pos] = chain_pos;
    }

    chains.push_back(new_chain);
}


void UpdateHierarchyRec(vector<cv::Vec4i>& hierarchy,
    const RCCode& rccode, const vector<unique_ptr<RCNode>>& node_vec,
    const vector<int>& used_elems) {

    for (auto& it = node_vec.cbegin(); it != node_vec.cend(); ++it) {
        auto node = it->get();

        RCNode* parent = node->parent;

        int chain_pos = used_elems[node->elem_index];

        // next sibling
        hierarchy[chain_pos][0] = (it + 1 == node_vec.cend()) ? -1 : used_elems[(it + 1)->get()->elem_index];

        // prev sibling
        hierarchy[chain_pos][1] = (it == node_vec.cbegin()) ? -1 : used_elems[(it - 1)->get()->elem_index];

        // first child
        hierarchy[chain_pos][2] = node->children.size() > 0 ? used_elems[node->children[0]->elem_index] : -1;

        // parent
        hierarchy[chain_pos][3] = (parent->elem_index == -1) ? -1 : used_elems[parent->elem_index];

        if (node->children.size() > 0) {
            UpdateHierarchyRec(hierarchy, rccode, node->children, used_elems);
        }
    }

}


void RCCodeToChainCodeInternal(const RCCode& rccode, ChainCode& chcode, vector<int>& used_elems) {

    const RCNode* root = rccode.root.get();

    for (unsigned i = 0; i < rccode.Size(); i++) {
        if (used_elems[i] == -1) {
            chcode.AddChain(rccode, used_elems, i);
        }
    }
}


void RCCodeToChainCode(const RCCode& rccode, ChainCode& chcode) {
    vector<int> used_elems(rccode.Size(), -1);
    //used_elems.resize(rccode.Size(), false);
    RCCodeToChainCodeInternal(rccode, chcode, used_elems);
}

void RCCodeToChainCode(const RCCode& rccode, ChainCode& chcode, vector<cv::Vec4i>& hierarchy) {
    vector<int> used_elems(rccode.Size(), -1);
    //used_elems.resize(rccode.Size(), false);
    RCCodeToChainCodeInternal(rccode, chcode, used_elems);

    hierarchy = vector<cv::Vec4i>(chcode.chains.size());
    UpdateHierarchyRec(hierarchy, rccode, rccode.root->children, used_elems);
}



bool CheckHierarchy(const std::vector<cv::Vec4i>& hierarchy) {

    int n = static_cast<int>(hierarchy.size());

    // Check that a parent can reach only his children
    // Check that next and prev siblings are coherent
    // Count children for each parent
    vector<int> child_count(hierarchy.size(), 0);

    for (int i = 0; i < n; ++i) {

        int first_child = hierarchy[i][2];

        if (first_child != -1) {

            ++child_count[i];

            if (hierarchy[first_child][1] != -1)
                return false;

            if (hierarchy[first_child][3] != i)
                return false;

            int current_child = first_child;
            int next_child = hierarchy[current_child][0];
            while (next_child != -1) {

                ++child_count[i];

                if (hierarchy[next_child][1] != current_child)
                    return false;

                if (hierarchy[next_child][3] != i)
                    return false;

                current_child = next_child;
                next_child = hierarchy[next_child][0];

            }
        }
    }

    // Check that all children were counted
    for (int i = 0; i < hierarchy.size(); ++i) {
        int parent = hierarchy[i][3];
        if (parent != -1) {
            --child_count[parent];
        }
    }    
    auto predicate = [](int v) -> bool { return v == 0; };
    return all_of(child_count.cbegin(), child_count.cend(), predicate);

    // return true;
}

void SortChains(ChainCode& chcode) {

    auto comp = [](const ChainCode::Chain& a, const ChainCode::Chain& b) {
        if (a.row != b.row)
            return a.row < b.row;
        else
            return a.col < b.col;
    };
    sort(chcode.chains.begin(), chcode.chains.end(), comp);

}

void SortChains(ChainCode& chcode, vector<cv::Vec4i>& hierarchy) {

    int n = static_cast<int>(chcode.chains.size());

    vector<int> mapping(n);
    vector<int> inv_mapping(n);
    iota(inv_mapping.begin(), inv_mapping.end(), 0);

    // Che cos'è chains = chains? Copiato da stack overflow
    auto comp = [chains = chcode.chains](const int a, const int b) {
        if (chains[a].row != chains[b].row)
            return chains[a].row < chains[b].row;
        else if (chains[a].col != chains[b].col)
            return chains[a].col < chains[b].col;
        else if (chains[a].value_count != chains[b].value_count)
            return chains[a].value_count < chains[b].value_count;
        else {
            for (size_t i = 0; i < chains[a].internal_values.size(); ++i) {
                if (chains[a].internal_values[i] != chains[b].internal_values[i]) {
                    return chains[a].internal_values[i] < chains[b].internal_values[i];
                }
            }
        }
        throw std::runtime_error("Unreachable code!");
    };

    sort(inv_mapping.begin(), inv_mapping.end(), comp);

    // Do we need this?
    for (int i = 0; i < n; ++i) {
        mapping[inv_mapping[i]] = i;
    }

    // Reorder chains
    vector<ChainCode::Chain> sorted_chains(n);
    for (int i = 0; i < n; ++i) {
        sorted_chains[i] = chcode.chains[inv_mapping[i]];
    }

    // Reorder hierarchy
    vector<cv::Vec4i> sorted_hierarchy(n, {-1, -1, -1, -1});

    // Parents first
    for (int i = 0; i < n; ++i) {
        int old_parent = hierarchy[inv_mapping[i]][3];
        if (old_parent != -1) {
            sorted_hierarchy[i][3] = mapping[old_parent];
        }
    }

    // Now that parents are correct, the rest of the hierarchy data can
    // be reconstructed from the parents. It is not useful for the
    // comparison, but it is calculated for completeness
    vector<int> last_children(n, -1);
    for (int i = 0; i < n; ++i) {
        int parent = sorted_hierarchy[i][3];
        if (parent != -1) {
            int last_sibling = last_children[parent];
            if (last_sibling == -1) {
                sorted_hierarchy[parent][2] = i;
            }
            else {
                sorted_hierarchy[last_sibling][0] = i;
                sorted_hierarchy[i][1] = last_sibling;                
            }
            last_children[parent] = i;
        }
    }

    chcode.chains = sorted_chains;
    hierarchy = sorted_hierarchy;
}