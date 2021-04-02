// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef BACCA_CHAINCODE_SCHEFFLER_H_
#define BACCA_CHAINCODE_SCHEFFLER_H_

#include "chaincode_algorithms.h"

#include <memory>

class Scheffler : public ChainCodeAlg {

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
    }
};



class SchefflerTopology : public ChainCodeAlg {

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
    }
};


#endif // BACCA_CHAINCODE_CEDERBERG_H_