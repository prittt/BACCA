// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef BACCA_REGISTER_H_
#define BACCA_REGISTER_H_

#define REGISTER_CHAINCODEALG(algorithm)                                           \
class register_##algorithm {                                                   \
  public:                                                                      \
    register_##algorithm() {                                                   \
        ChainCodeAlgMapSingleton::GetInstance().data_[#algorithm] = new algorithm; \
    }                                                                          \
} reg_##algorithm;

//#define STRINGIFY(x) #x
//#define CONCAT(x,y) STRINGIFY(x ## _ ## y)
//
//#define REGISTER_SOLVER(algorithm, solver)                                                              \
//class register_ ## algorithm ## _ ## solver{                                                            \
//  public:                                                                                               \
//      register_ ## algorithm ## _ ## solver() {                                                         \
//          LabelingMapSingleton::GetInstance().data_[CONCAT(algorithm, solver)] = new algorithm<solver>; \
//  }                                                                                                     \
//}  register_ ## algorithm ## _ ## solver;

#endif // !BACCA_REGISTER_H_