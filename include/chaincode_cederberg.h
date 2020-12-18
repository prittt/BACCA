// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef BACCA_CHAINCODE_CEDERBERG_H_
#define BACCA_CHAINCODE_CEDERBERG_H_

#include "chaincode_algorithms.h"

class Cederberg : public ChainCodeAlg {

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
    }
};

class Cederberg_LUT : public ChainCodeAlg {

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
    }
};

class Cederberg_LUT_PRED : public ChainCodeAlg {

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
    }
};

class Cederberg_DRAG : public ChainCodeAlg {
private:

    RCCode PerformRCCode();
    void ConvertToChainCode(const RCCode& rccode);

public:
    virtual void PerformChainCode() override;
    
    virtual void PerformChainCodeWithSteps() override;

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
    }
};


class Cederberg_Tree : public ChainCodeAlg {
public:
	virtual void PerformChainCode() override;
	
	virtual void FreeChainCodeData() {
		ChainCodeAlg::FreeChainCodeData();
	}
};



class Cederberg_Spaghetti : public ChainCodeAlg {
public:
	virtual void PerformChainCode() override;

	virtual void FreeChainCodeData() {
		ChainCodeAlg::FreeChainCodeData();
	}
};


class Cederberg_Spaghetti_FREQ_All : public ChainCodeAlg {
public:
	virtual void PerformChainCode() override;

	virtual void FreeChainCodeData() {
		ChainCodeAlg::FreeChainCodeData();
	}
};

class Cederberg_Spaghetti_FREQ_AllNoClassical : public ChainCodeAlg {
public:
	virtual void PerformChainCode() override;

	virtual void FreeChainCodeData() {
		ChainCodeAlg::FreeChainCodeData();
	}
};


class Cederberg_Spaghetti_FREQ_Hamlet : public ChainCodeAlg {
public:
	virtual void PerformChainCode() override;

	virtual void FreeChainCodeData() {
		ChainCodeAlg::FreeChainCodeData();
	}
};


#endif // BACCA_CHAINCODE_CEDERBERG_H_