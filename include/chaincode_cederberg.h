#ifndef BACCA_CHAINCODE_CEDERBERG_H_
#define BACCA_CHAINCODE_CEDERBERG_H_

#include "chaincode_algorithms.h"

class Cederberg : public ChainCodeAlg {

    RCCode rccode;

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
        rccode.Clean();
    }
};

class Cederberg_LUT : public ChainCodeAlg {

    RCCode rccode;

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
        rccode.Clean();
    }
};

class Cederberg_LUT_PRED : public ChainCodeAlg {

    RCCode rccode;

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
        rccode.Clean();
    }
};

class Cederberg_DRAG : public ChainCodeAlg {
public:
    RCCode rccode;

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
        rccode.Clean();
    }
};

class Cederberg_DRAG_2 : public ChainCodeAlg {
public:
    RCCode rccode;

public:
    virtual void PerformChainCode();

    virtual void FreeChainCodeData() {
        ChainCodeAlg::FreeChainCodeData();
        rccode.Clean();
    }
};


#endif // BACCA_CHAINCODE_CEDERBERG_H_