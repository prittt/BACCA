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


//class Cederberg_Tree : public ChainCodeAlg {
//public:
//	virtual void PerformChainCode() override;
//	
//	virtual void FreeChainCodeData() {
//		ChainCodeAlg::FreeChainCodeData();
//	}
//};



class Cederberg_Spaghetti : public ChainCodeAlg {
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


//class Cederberg_Spaghetti_FREQ : public ChainCodeAlg {
//public:
//	virtual void PerformChainCode() override;
//
//	virtual void FreeChainCodeData() {
//		ChainCodeAlg::FreeChainCodeData();
//	}
//};
//
//class Cederberg_Spaghetti_FREQ_without_classical : public ChainCodeAlg {
//public:
//	virtual void PerformChainCode() override;
//
//	virtual void FreeChainCodeData() {
//		ChainCodeAlg::FreeChainCodeData();
//	}
//};
//
//
//class Cederberg_Spaghetti_FREQ_hamlet : public ChainCodeAlg {
//public:
//	virtual void PerformChainCode() override;
//
//	virtual void FreeChainCodeData() {
//		ChainCodeAlg::FreeChainCodeData();
//	}
//};
//
//
//class Cederberg_Spaghetti_FREQ_hamlet_new : public ChainCodeAlg {
//public:
//	virtual void PerformChainCode() override;
//
//	virtual void FreeChainCodeData() {
//		ChainCodeAlg::FreeChainCodeData();
//	}
//};

#endif // BACCA_CHAINCODE_CEDERBERG_H_