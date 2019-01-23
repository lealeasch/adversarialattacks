// nnet2/am-nnet.h

// Copyright 2012  Johns Hopkins University (author: Daniel Povey)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#ifndef KALDI_NNET2_AM_PRINT_H_
#define KALDI_NNET2_AM_PRINT_H_


#include "cudamatrix/cu-matrix-lib.h"
//include "nnet2/am-print.h"


namespace kaldi {
namespace nnet2 {


bool WriteCuMatrixBaseFloat(std::string wxfilename, CuMatrix<BaseFloat> &matrix);

bool ReadCuMatrixBaseFloat(std::string rxfilename, CuMatrix<BaseFloat> *matrix, bool trans);



bool WriteMatrixBaseFloat(std::string wxfilename, Matrix<BaseFloat> &matrix);

bool ReadMatrixBaseFloat(std::string rxfilename, Matrix<BaseFloat> *matrix, bool trans);


} // namespace nnet2
} // namespace kaldi

#endif // KALDI_NNET2_AM_PRINT_H_
