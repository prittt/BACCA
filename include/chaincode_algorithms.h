// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef BACCA_CHAIN_CODE_ALGORITHMS_H_
#define BACCA_CHAIN_CODE_ALGORITHMS_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "performance_evaluator.h"
#include "chain_code.h"

class ChainCodeAlg {
public:
    static cv::Mat1b img_;

    bool with_hierarchy_ = false;
    ChainCode chain_code_;
    std::vector<cv::Vec4i> hierarchy_;

    PerformanceEvaluator perf_;

    ChainCodeAlg() {}
    virtual ~ChainCodeAlg() = default;

    virtual void PerformChainCode() { throw std::runtime_error("'PerformChainCode()' not implemented"); }
    virtual void PerformChainCodeWithSteps() { throw std::runtime_error("'PerformChainCodeWithSteps()' not implemented"); }
    virtual void PerformChainCodeMem(std::vector<uint64_t>& accesses) { throw std::runtime_error("'PerformChainCodeMem(...)' not implemented"); }

    virtual void FreeChainCodeData() { chain_code_.Clean(); hierarchy_.clear(); hierarchy_.shrink_to_fit(); }

};

class ChainCodeAlgMapSingleton {
public:
    std::map<std::string, ChainCodeAlg*> data_;

    static ChainCodeAlgMapSingleton& GetInstance();
    static ChainCodeAlg* GetChainCodeAlg(const std::string& s);
    static bool Exists(const std::string& s);
    ChainCodeAlgMapSingleton(ChainCodeAlgMapSingleton const&) = delete;
    void operator=(ChainCodeAlgMapSingleton const&) = delete;

private:
    ChainCodeAlgMapSingleton() {}
    ~ChainCodeAlgMapSingleton()
    {
        for (std::map<std::string, ChainCodeAlg*>::iterator it = data_.begin(); it != data_.end(); ++it)
            delete it->second;
    }
};

enum StepType {
    ALGORITHM = 0,
    CONVERSION = 1,
    ST_SIZE = 2,
};

std::string Step(StepType n_step);

#endif // BACCA_CHAIN_CODE_ALGORITHMS_