// Copyright(c) 2019 Federico Bolelli, Costantino Grana
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
// *Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and / or other materials provided with the distribution.
//
// * Neither the name of THeBE nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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