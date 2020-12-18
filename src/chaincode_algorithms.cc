// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include "chaincode_algorithms.h"

cv::Mat1b ChainCodeAlg::img_;

ChainCodeAlgMapSingleton& ChainCodeAlgMapSingleton::GetInstance()
{
    static ChainCodeAlgMapSingleton instance;	// Guaranteed to be destroyed.
                                            // Instantiated on first use.
    return instance;
}

ChainCodeAlg* ChainCodeAlgMapSingleton::GetChainCodeAlg(const std::string& s)
{
    return ChainCodeAlgMapSingleton::GetInstance().data_.at(s);
}

bool ChainCodeAlgMapSingleton::Exists(const std::string& s)
{
    return ChainCodeAlgMapSingleton::GetInstance().data_.end() != ChainCodeAlgMapSingleton::GetInstance().data_.find(s);
}

std::string Step(StepType n_step)
{
    switch (n_step) {
    case ALGORITHM:
        return "Algorithm";
        break;
    case CONVERSION:
        return "Conversion";
        break;
    case ST_SIZE: // To avoid warning on AppleClang
        break;
    }

    return "";
}