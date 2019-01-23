// nnet2/nnet-update.h

// Copyright 2012  Johns Hopkins University (author: Daniel Povey)
//           2014  Xiaohui Zhang

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

#ifndef KALDI_NNET2_NNET_UPDATE_SPOOF_H_
#define KALDI_NNET2_NNET_UPDATE_SPOOF_H_

#include "nnet2/nnet-nnet.h"
#include "nnet2/nnet-example.h"
#include "util/table-types.h"


namespace kaldi {
namespace nnet2 {




// This class NnetUpdater contains functions for updating the neural net or
// computing its gradient, given a set of NnetExamples. We
// define it in the header file becaused it's needed by the ensemble training.
// But in normal cases its functionality should be used by calling DoBackprop(),
// and by ComputeNnetObjf()
class SpoofUpdater {
 public:
  // Note: in the case of training with SGD, "nnet" and "nnet_to_update" will
  // be identical.  They'll be different if we're accumulating the gradient
  // for a held-out set and don't want to update the model.  Note: nnet_to_update
  // may be NULL if you don't want do do backprop.
  SpoofUpdater(const Nnet &nnet);
  
  /// Does the entire forward and backward computation for this minbatch.
  /// Returns total objective function over this minibatch.  If tot_accuracy != NULL,
  /// outputs to that pointer the total accuracy.
  double ComputeForExample(const CuMatrixBase<BaseFloat> &data, CuMatrix<BaseFloat> &original_magn, CuMatrix<BaseFloat> &threshhold, CuMatrix<BaseFloat> &thresholds_norm, const CuMatrix<BaseFloat> &prob_target, CuMatrix<BaseFloat> &deriv, double *tot_diff, bool pad, double max_val_dB, double thresh);
                           //double *tot_accuracy);

  void ComputeForExample(const CuMatrixBase<BaseFloat> &data, CuMatrix<BaseFloat> &fft_feat, bool pad);

  void GetOutput(CuMatrix<BaseFloat> *output);
 protected:

  void Propagate();


  double ComputeObjfAndDeriv(
    const CuMatrixBase<BaseFloat> &data,
    const CuMatrix<BaseFloat> &prob_target,
    CuMatrix<BaseFloat> *deriv,
    double *tot_diff) const;


  /// Backprop must be called after ComputeObjfAndDeriv.  Does the
  /// backpropagation; "nnet_to_update_" is updated.  Note: "deriv" will
  /// contain, at input, the derivative w.r.t. the output layer (as computed by
  /// ComputeObjfAndDeriv), but will be used as a temporary variable by this
  /// function.
  double BackpropInput(CuMatrix<BaseFloat> *deriv, CuMatrix<BaseFloat> &fft_diff, CuMatrix<BaseFloat> &threshhold, CuMatrix<BaseFloat> &thresholds_norm, double max_val_dB, double thresh);

  void CalculateMagnitute(const CuMatrix<BaseFloat> &complex, CuMatrix<BaseFloat> *magnitude);

  friend class NnetEnsembleTrainer;
 private:
  // Must be called after Propagate().
  double ComputeTotAccuracy(const std::vector<NnetExample> &data) const;

  const Nnet &nnet_;
  Nnet *nnet_to_update_;
  int32 num_chunks_; // same as the minibatch size.
  std::vector<ChunkInfo> chunk_info_out_; 
  // std::vector <ChunkInfo> chunk_info_; is it different to chunk_info_out_? If so it has to be changed in ComputeForExample

  
  std::vector<CuMatrix<BaseFloat> > forward_data_; // The forward data
  // for the outputs of each of the components.

};

bool PrintMatrix(CuMatrix<BaseFloat> A, string name);


double DoSpoof(const Nnet &nnet,
               const CuMatrixBase<BaseFloat> &feat,
               CuMatrix<BaseFloat> &fft_diff,
               CuMatrix<BaseFloat> &threshhold,
               CuMatrix<BaseFloat> &thresholds_norm,
               CuMatrix<BaseFloat> &output_feats,
               const CuMatrix<BaseFloat> &prob_target,
               double max_val_dB,
               double thresh,
               double *tot_accuracy = NULL,
               bool pad = false);

void DoPropagteToFFT(const Nnet &nnet,
               const CuMatrixBase<BaseFloat> &feat,
               CuMatrix<BaseFloat> &original_magn,
               bool pad);

} // namespace nnet2
} // namespace kaldi

#endif // KALDI_NNET2_NNET_SPOOF_H_
