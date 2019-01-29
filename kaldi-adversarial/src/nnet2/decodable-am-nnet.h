// nnet2/decodable-am-nnet.h

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

#ifndef KALDI_NNET2_DECODABLE_AM_NNET_H_
#define KALDI_NNET2_DECODABLE_AM_NNET_H_

#include <vector>
#include <complex>
#include <math.h> 
#include "base/kaldi-common.h"
#include "gmm/am-diag-gmm.h"
#include "hmm/transition-model.h"
#include "itf/decodable-itf.h"
#include "nnet2/am-nnet.h"
#include "nnet2/nnet-print-csv.h"
#include "nnet2/nnet-compute.h"
#include "nnet2/nnet-update-spoof.h"
#include "util/common-utils.h"
#include "matrix/kaldi-matrix.h"
#include "util/common-utils.h"
#include "transform/cmvn.h"

namespace kaldi {
namespace nnet2 {

/// DecodableAmNnet is a decodable object that decodes
/// with a neural net acoustic model of type AmNnet.

class DecodableAmNnet: public DecodableInterface {
 public:
  DecodableAmNnet(const TransitionModel &trans_model,
                  const AmNnet &am_nnet,
                  const CuMatrixBase<BaseFloat> &feats,
                  bool pad_input = true, // if !pad_input, the NumIndices()
                                         // will be < feats.NumRows().
                  BaseFloat prob_scale = 1.0):
      trans_model_(trans_model) {
    // Note: we could make this more memory-efficient by doing the
    // computation in smaller chunks than the whole utterance, and not
    // storing the whole thing.  We'll leave this for later.
    int32 num_rows = feats.NumRows() -
        (pad_input ? 0 : am_nnet.GetNnet().LeftContext() +
                         am_nnet.GetNnet().RightContext());
    if (num_rows <= 0) {
      KALDI_WARN << "Input with " << feats.NumRows()  << " rows will produce "
                 << "empty output.";
      return;
    }
    CuMatrix<BaseFloat> log_probs(num_rows, trans_model.NumPdfs());
    // the following function is declared in nnet-compute.h
    NnetComputation(am_nnet.GetNnet(), feats, pad_input, &log_probs);


    KALDI_LOG << "FEATS(10,100): " << feats(10,100) << "    LOG PROB: " << log_probs(10,100);
    KALDI_LOG << "FEATS(10,200): " << feats(10,200) << "    LOG PROB: " << log_probs(10,200);
    KALDI_LOG << "FEATS(20,100): " << feats(20,100) << "    LOG PROB: " << log_probs(20,100);
    KALDI_LOG << "FEATS(50,100): " << feats(50,100) << "    LOG PROB: " << log_probs(50,100);
    KALDI_LOG << "FEATS(0,0): " << feats(0,0) << "    LOG PROB: " << log_probs(0,0);


    log_probs.ApplyFloor(1.0e-20); // Avoid log of zero which leads to NaN.
    log_probs.ApplyLog();
    CuVector<BaseFloat> priors(am_nnet.Priors());
    KALDI_ASSERT(priors.Dim() == trans_model.NumPdfs() &&
                 "Priors in neural network not set up.");
    priors.ApplyLog();
    // subtract log-prior (divide by prior)
    log_probs.AddVecToRows(-1.0, priors);
    // apply probability scale.
    log_probs.Scale(prob_scale);
    // Transfer the log-probs to the CPU for faster access by the
    // decoding process.
    log_probs_.Swap(&log_probs);
  }

  // Note, frames are numbered from zero.  But transition_id is numbered
  // from one (this routine is called by FSTs).
  virtual BaseFloat LogLikelihood(int32 frame, int32 transition_id) {
    return log_probs_(frame,
                      trans_model_.TransitionIdToPdf(transition_id));
  }

  virtual int32 NumFramesReady() const { return log_probs_.NumRows(); }
  
  // Indices are one-based!  This is for compatibility with OpenFst.
  virtual int32 NumIndices() const { return trans_model_.NumTransitionIds(); }
  
  virtual bool IsLastFrame(int32 frame) const {
    KALDI_ASSERT(frame < NumFramesReady());
    return (frame == NumFramesReady() - 1);
  }

 protected:
  const TransitionModel &trans_model_;
  Matrix<BaseFloat> log_probs_; // actually not really probabilities, since we divide
  // by the prior -> they won't sum to one.

  KALDI_DISALLOW_COPY_AND_ASSIGN(DecodableAmNnet);
};

/// This version of DecodableAmNnet is intended for a version of the decoder
/// that processes different utterances with multiple threads.  It needs to do
/// the computation in a different place than the initializer, since the
/// initializer gets called in the main thread of the program.

class DecodableAmNnetParallel: public DecodableInterface {
 public:
  DecodableAmNnetParallel(
      const TransitionModel &trans_model,
      const AmNnet &am_nnet,
      const CuMatrix<BaseFloat> *feats,
      bool pad_input = true,
      BaseFloat prob_scale = 1.0):
      trans_model_(trans_model), am_nnet_(am_nnet), feats_(feats),
      pad_input_(pad_input), prob_scale_(prob_scale) {
    KALDI_ASSERT(feats_ != NULL);
  }

  void Compute() {
    log_probs_.Resize(feats_->NumRows(), trans_model_.NumPdfs());
    // the following function is declared in nnet-compute.h
    NnetComputation(am_nnet_.GetNnet(), *feats_,
                    pad_input_, &log_probs_);
    log_probs_.ApplyFloor(1.0e-20); // Avoid log of zero which leads to NaN.
    log_probs_.ApplyLog();
    CuVector<BaseFloat> priors(am_nnet_.Priors());
    KALDI_ASSERT(priors.Dim() == trans_model_.NumPdfs() &&
                 "Priors in neural network not set up.");
    priors.ApplyLog();
    // subtract log-prior (divide by prior)
    log_probs_.AddVecToRows(-1.0, priors);
    // apply probability scale.
    log_probs_.Scale(prob_scale_);
    delete feats_;
    feats_ = NULL;
  }

  // Note, frames are numbered from zero.  But state_index is numbered
  // from one (this routine is called by FSTs).
  virtual BaseFloat LogLikelihood(int32 frame, int32 transition_id) {
    if (feats_) Compute(); // this function sets feats_ to NULL.
    return log_probs_(frame,
                      trans_model_.TransitionIdToPdf(transition_id));
  }

  int32 NumFramesReady() const {
    if (feats_) {
      if (pad_input_) return feats_->NumRows();
      else {
        int32 ans = feats_->NumRows() - am_nnet_.GetNnet().LeftContext() -
            am_nnet_.GetNnet().RightContext();
        if (ans < 0) ans = 0;
        return ans;
      }
    } else {
      return log_probs_.NumRows();
    }
  }
  
  // Indices are one-based!  This is for compatibility with OpenFst.
  virtual int32 NumIndices() const { return trans_model_.NumTransitionIds(); }
  
  virtual bool IsLastFrame(int32 frame) const {
    KALDI_ASSERT(frame < NumFramesReady());
    return (frame == NumFramesReady() - 1);
  }
  ~DecodableAmNnetParallel() {
    delete feats_;
  }
 protected:
  const TransitionModel &trans_model_;
  const AmNnet &am_nnet_;
  CuMatrix<BaseFloat> log_probs_; // actually not really probabilities, since we divide
  // by the prior -> they won't sum to one.
  const CuMatrix<BaseFloat> *feats_;
  bool pad_input_;
  BaseFloat prob_scale_;
  KALDI_DISALLOW_COPY_AND_ASSIGN(DecodableAmNnetParallel);
};


class DecodableAmNnetSpoof: public DecodableInterface {
 public:
  DecodableAmNnetSpoof(const TransitionModel &trans_model,
                  const AmNnet &am_nnet,
                  const CuMatrixBase<BaseFloat> &feats,
                  const CuMatrixBase<BaseFloat> &feats_target,
                  std::string utt,
                  bool pad_input = true, // if !pad_input, the NumIndices()
                                         // will be < feats.NumRows().
                  BaseFloat prob_scale = 1.0,
                  int num_utterance = 1,
                  int num_iter = 500,
                  double thresh = 20.0,
                  std::string path="",
                  std::string dir_arg=""):
      trans_model_(trans_model) {
    // Note: we could make this more memory-efficient by doing the
    // computation in smaller chunks than the whole utterance, and not
    // storing the whole thing.  We'll leave this for later.

    //CuMatrix<BaseFloat> feats(feats_old);
    //feats.SetZero();

    KALDI_LOG << utt;
    int32 num_rows = feats.NumRows() -
        (pad_input ? 0 : am_nnet.GetNnet().LeftContext() +
                         am_nnet.GetNnet().RightContext());
    if (num_rows <= 0) {
      KALDI_WARN << "Input with " << feats.NumRows()  << " rows will produce "
                 << "empty output.";
      return;
    }

    CuMatrix<BaseFloat> log_probs(num_rows, trans_model.NumPdfs());
    // the following function is declared in nnet-compute.h
    NnetComputation(am_nnet.GetNnet(), feats, pad_input, &log_probs);

    int32 num_rows_target = feats_target.NumRows() -
        (pad_input ? 0 : am_nnet.GetNnet().LeftContext() +
                         am_nnet.GetNnet().RightContext());

    CuMatrix<BaseFloat> target_log_probs(num_rows_target, trans_model.NumPdfs());
    CuMatrix<BaseFloat> target_pre_log_probs(num_rows_target, trans_model.NumPdfs());
    // the following function is declared in nnet-compute.h
    NnetComputation(am_nnet.GetNnet(), feats_target, pad_input, &target_log_probs, &target_pre_log_probs);

    double max_val = std::numeric_limits<BaseFloat>::epsilon();
    for(int c = 0; c < feats.NumCols(); c++) {
        for(int r = 0; r <  feats.NumRows(); r++) {
            if(std::abs(feats(r,c)) > max_val)
                max_val = std::abs(feats(r,c));
        }
    }

    //feats.Scale(1/max_val);

    double tot_diff = 0;
    CuMatrix<BaseFloat> output_grad;
    CuMatrix<BaseFloat> updated_feats(feats);
    CuMatrix<BaseFloat> log_probs_updated(log_probs);

    saveMatrix(updated_feats, utt, path, std::to_string(-1));


    Matrix<BaseFloat> results;

    // Read in arg from csv file
    //std::string dir_utt = path + "/new_target.csv";
    std::string dir_utt = path + "/" + utt + "/target.csv";
    ReadCuMatrixBaseFloat(dir_utt, &target_log_probs, false);

    CuMatrix<BaseFloat> thresholds(num_rows_target, trans_model.NumPdfs());
    dir_utt = path + "/thresholds/" + utt + "_dB.csv";
    ReadCuMatrixBaseFloat(dir_utt, &thresholds, false);

    CuMatrix<BaseFloat> thresholds_norm(num_rows_target, trans_model.NumPdfs());
    dir_utt = path + "/thresholds/" + utt + ".csv";
    ReadCuMatrixBaseFloat(dir_utt, &thresholds_norm, false);

    saveMatrix(log_probs, utt, path, "original");
    saveMatrix(target_log_probs, utt, path, "real-target");

    int rr = 105;
    int cc = 2589;


    double factor = 0.05; //0.05

    KALDI_LOG << "MAX: " << max_val;

    KALDI_LOG << "ORIGINAL FEATS: " << feats(rr,cc) << "    " << log_probs(rr,cc);
    KALDI_LOG << "TARGET FEATS: " << feats_target(rr,cc) << "    " << target_log_probs(rr,cc);

    CuMatrix<BaseFloat> fft_diff(num_rows_target, 128);

    KALDI_LOG << "updated_feats.NumRows: " << updated_feats.NumRows();
    KALDI_LOG << "updated_feats.NumCols: " << updated_feats.NumCols();

    KALDI_LOG << "thresholds.NumRows: " << thresholds.NumRows();
    KALDI_LOG << "thresholds.NumCols: " << thresholds.NumCols();

    KALDI_LOG << "log_probs.NumRows: " << log_probs.NumRows();
    KALDI_LOG << "log_probs.NumCols: " << log_probs.NumCols();

    KALDI_LOG << "target_log_probs.NumRows: " << target_log_probs.NumRows();
    KALDI_LOG << "target_log_probs.NumCols: " << target_log_probs.NumCols();

    // Do Spoofing
    CuMatrix<BaseFloat> original_mag(0,0);
    double max_val_dB = 0;
    max_val_dB = DoSpoof(am_nnet.GetNnet(), updated_feats, original_mag, thresholds, thresholds_norm, output_grad, target_log_probs, 0, thresh, &tot_diff, pad_input);
    CuMatrix<BaseFloat> keep_original_mag(original_mag);


    KALDI_LOG << "Num Iterations: " << num_iter;
    KALDI_LOG << "Threshold : " << thresh;

    for (int i = 0; i < num_iter; i++) { //70 --> 1500 0 + 1000 * num_utterance/2

      // Do Spoofing
      DoSpoof(am_nnet.GetNnet(), updated_feats, keep_original_mag, thresholds, thresholds_norm, output_grad, target_log_probs, max_val_dB, thresh, &tot_diff, pad_input);
      keep_original_mag = original_mag;

      CuSubMatrix<BaseFloat> grad_part(output_grad, am_nnet.GetNnet().LeftContext(),
                                          updated_feats.NumRows(), 0, output_grad.NumCols());

      //KALDI_LOG << "grad: " << grad_part.Row(329);
      updated_feats.AddMat(factor*max_val, grad_part); //*100 

      // the following function is declared in nnet-compute.h
      NnetComputation(am_nnet.GetNnet(), updated_feats, pad_input, &log_probs_updated);
      log_probs_updated.ApplyFloor(1.0e-20);

      if(i%50 == 0)
        KALDI_LOG << "FEATS: "  << updated_feats(rr,cc) << "    " << log_probs_updated(rr,cc) << "    " << grad_part(rr,cc);
    }

    saveMatrix(log_probs_updated, utt, path, "updated");

    KALDI_LOG << "FEATS: " << updated_feats(rr,cc) << "    " << log_probs_updated(rr,cc);

    KALDI_LOG << "FEATS(10,100): " << updated_feats(10,100) << "    LOG PROB: " << log_probs_updated(10,100);
    KALDI_LOG << "FEATS(10,200): " << updated_feats(10,200) << "    LOG PROB: " << log_probs_updated(10,200);
    KALDI_LOG << "FEATS(20,100): " << updated_feats(20,100) << "    LOG PROB: " << log_probs_updated(20,100);
    KALDI_LOG << "FEATS(50,100): " << updated_feats(50,100) << "    LOG PROB: " << log_probs_updated(50,100);
    KALDI_LOG << "FEATS(0,0): " << updated_feats(0,0) << "    LOG PROB: " << log_probs_updated(0,0);

    //updated_feats_normed = reNormalize(updated_feats, utt);
    //updated_feats.Scale(max_val);

    saveMatrix(updated_feats, utt, path, std::to_string(99));


    log_probs_updated.ApplyFloor(1.0e-20); // Avoid log of zero which leads to NaN.
    log_probs_updated.ApplyLog();
    CuVector<BaseFloat> priors(am_nnet.Priors());
    KALDI_ASSERT(priors.Dim() == trans_model.NumPdfs() &&
                 "Priors in neural network not set up.");
    priors.ApplyLog();
    // subtract log-prior (divide by prior)
    log_probs_updated.AddVecToRows(-1.0, priors);
    // apply probability scale.
    log_probs_updated.Scale(prob_scale);
    // Transfer the log-probs to the CPU for faster access by the
    // decoding process.
    log_probs_.Swap(&log_probs_updated);
  }

  // Note, frames are numbered from zero.  But transition_id is numbered
  // from one (this routine is called by FSTs).
  virtual BaseFloat LogLikelihood(int32 frame, int32 transition_id) {
    return log_probs_(frame,
                      trans_model_.TransitionIdToPdf(transition_id));
  }

  virtual int32 NumFramesReady() const { return log_probs_.NumRows(); }
  
  // Indices are one-based!  This is for compatibility with OpenFst.
  virtual int32 NumIndices() const { return trans_model_.NumTransitionIds(); }
  
  virtual bool IsLastFrame(int32 frame) const {
    KALDI_ASSERT(frame < NumFramesReady());
    return (frame == NumFramesReady() - 1);
  }

  void saveMatrix(CuMatrix<BaseFloat> &input, std::string num_utterance, std::string dir, std::string iter){

    std::string dir_utt = dir + "/" + num_utterance;
    std::string mk_string = "mkdir -p " + dir_utt;
    const int dir_err = system(mk_string.c_str());

    if (-1 == dir_err)
      KALDI_ERR << "Error creating directory!\n";

    std::string file = dir_utt + "/" + iter  + ".csv";

    WriteCuMatrixBaseFloat(file,input);
    //ReadCuMatrixBaseFloat(file, &input);
  };

  CuMatrix<BaseFloat> reNormalize(CuMatrix<BaseFloat> feats, string utt) {

    std::string utt2spk_rspecifier = "ark:/media/lea/Daten/Scibo/Projects/asr_hidden_voice_commands.git/kaldi/egs/yesno/s5/data/test_yesno/split1/1/utt2spk";
    std::string cmvn_rspecifier = "scp:/media/lea/Daten/Scibo/Projects/asr_hidden_voice_commands.git/kaldi/egs/yesno/s5/data/test_yesno/split1/1/cmvn.scp";
    RandomAccessDoubleMatrixReaderMapped cmvn_reader(cmvn_rspecifier,
                                                       utt2spk_rspecifier);


    if (!cmvn_reader.HasKey(utt)) {
      KALDI_WARN << "No normalization statistics available for key "
                 << utt << ", producing no output for this utterance";

    }
    Matrix<double> cmvn_stats = cmvn_reader.Value(utt);
    Matrix<BaseFloat> updated_feats_norm(feats);

    bool norm_vars = true;

    ApplyCmvnReverse(cmvn_stats, norm_vars, &updated_feats_norm);

    CuMatrix<BaseFloat> updated_feats_normed(updated_feats_norm);

    return updated_feats_normed;
  }


 void synthesize(std::string dir_utt, std::string dest_dir, CuMatrix<BaseFloat> abs, Matrix<BaseFloat> *results){

    std::cout.precision(10);

    // Read in arg from csv file
    Matrix<BaseFloat> arg;
    ReadMatrixBaseFloat(dir_utt, &arg, false);

    int nfft = arg.NumCols()-2;
    int idx1 = arg.NumCols()-2;
    int idx2 = arg.NumCols()-1;
 

    SplitRadixRealFft<BaseFloat> srfft(nfft*2);

    results->Resize(2*nfft, arg.NumRows());

    Vector<BaseFloat> abs_row(nfft);
    Vector<BaseFloat> arg_row(nfft);
    Vector<BaseFloat> res;
    for(int r = 0; r < arg.NumRows(); r++) {

      for(int i = 0; i < nfft; i++) {
        abs_row(i) = std::sqrt(std::exp(abs(r,i)));
        arg_row(i) = arg(r,i);
      }

      ComputeComplex(&abs_row, &arg_row, &res);

      res(0) = arg(r,idx1);
      res(1) = arg(r,idx2);

      srfft.Compute(res.Data(), false);

      results->CopyColFromVec(res,r); 
    }

    WriteMatrixBaseFloat(dest_dir, *results);
  };

  void ComputeComplex(VectorBase<BaseFloat> *abs, VectorBase<BaseFloat> *arg, Vector<BaseFloat> *res) {

    int32 dim = arg->Dim();

    res->Resize(2*dim);
    for (int32 i = 0; i < dim; i++) {

      BaseFloat abs_i = (*abs)(i), arg_i = (*arg)(i);

      //KALDI_LOG << "ABS: " << abs_i << " ARG: " << arg_i;
      double real_part = abs_i*std::cos(arg_i);
      double imag_part = abs_i*std::sin(arg_i);

      //KALDI_LOG << "REAL: " << real_part << " IMAG: " << imag_part;
      std::complex<double> c(real_part,imag_part);

      (*res)(2*i) = std::real(c);
      (*res)(2*i+1) = std::imag(c);
    }
  }


 protected:
  const TransitionModel &trans_model_;
  Matrix<BaseFloat> log_probs_; // actually not really probabilities, since we divide
  // by the prior -> they won't sum to one.

  KALDI_DISALLOW_COPY_AND_ASSIGN(DecodableAmNnetSpoof);
};

class DecodableAmNnetSpoofIter: public DecodableInterface {
 public:
  DecodableAmNnetSpoofIter(const TransitionModel &trans_model,
                  const AmNnet &am_nnet,
                  const CuMatrixBase<BaseFloat> &feats,
                  const CuMatrixBase<BaseFloat> &feats_target,
                  std::string utt,
                  bool pad_input = true, // if !pad_input, the NumIndices()
                                         // will be < feats.NumRows().
                  BaseFloat prob_scale = 1.0,
                  int num_utterance = 1,
                  int num_iter = 500,
                  double thresh = 20.0,
                  std::string path="",
                  std::string dir_arg=""):
      trans_model_(trans_model) {
    // Note: we could make this more memory-efficient by doing the
    // computation in smaller chunks than the whole utterance, and not
    // storing the whole thing.  We'll leave this for later.

    //CuMatrix<BaseFloat> feats(feats_old);
    //feats.SetZero();

    KALDI_LOG << utt;
    int32 num_rows = feats.NumRows() -
        (pad_input ? 0 : am_nnet.GetNnet().LeftContext() +
                         am_nnet.GetNnet().RightContext());
    if (num_rows <= 0) {
      KALDI_WARN << "Input with " << feats.NumRows()  << " rows will produce "
                 << "empty output.";
      return;
    }

    CuMatrix<BaseFloat> log_probs(num_rows, trans_model.NumPdfs());
    // the following function is declared in nnet-compute.h
    NnetComputation(am_nnet.GetNnet(), feats, pad_input, &log_probs);

    int32 num_rows_target = feats_target.NumRows() -
        (pad_input ? 0 : am_nnet.GetNnet().LeftContext() +
                         am_nnet.GetNnet().RightContext());

    CuMatrix<BaseFloat> target_log_probs(num_rows_target, trans_model.NumPdfs());
    CuMatrix<BaseFloat> target_pre_log_probs(num_rows_target, trans_model.NumPdfs());
    // the following function is declared in nnet-compute.h
    NnetComputation(am_nnet.GetNnet(), feats_target, pad_input, &target_log_probs, &target_pre_log_probs);

    double max_val = std::numeric_limits<BaseFloat>::epsilon();
    for(int c = 0; c < feats.NumCols(); c++) {
        for(int r = 0; r <  feats.NumRows(); r++) {
            if(std::abs(feats(r,c)) > max_val)
                max_val = std::abs(feats(r,c));
        }
    }

    //feats.Scale(1/max_val);

    double tot_diff = 0;
    CuMatrix<BaseFloat> output_grad;
    CuMatrix<BaseFloat> updated_feats(feats);
    CuMatrix<BaseFloat> log_probs_updated(log_probs);

    std::string dir_utt_updated = path + "/utterances/" + utt + "/adversarial.csv";
    ReadCuMatrixBaseFloat(dir_utt_updated, &updated_feats, false);

    Matrix<BaseFloat> results;

    // Read in arg from csv file
    std::string dir_utt = path + "/utterances/" + utt + "/target.csv";
    ReadCuMatrixBaseFloat(dir_utt, &target_log_probs, false);

    CuMatrix<BaseFloat> thresholds(num_rows_target, trans_model.NumPdfs());
    dir_utt = path + "/thresholds/" + utt + "_dB.csv";
    ReadCuMatrixBaseFloat(dir_utt, &thresholds, false);

    CuMatrix<BaseFloat> thresholds_norm(num_rows_target, trans_model.NumPdfs());
    dir_utt = path + "/thresholds/" + utt + ".csv";
    ReadCuMatrixBaseFloat(dir_utt, &thresholds_norm, false);

    int rr = 105;
    int cc = 2589;

    double factor = 0.05; //0.05

    KALDI_LOG << "FEATS(10,100): " << updated_feats(10,100) << "    LOG PROB: " << log_probs_updated(10,100);
    KALDI_LOG << "FEATS(10,200): " << updated_feats(10,200) << "    LOG PROB: " << log_probs_updated(10,200);
    KALDI_LOG << "FEATS(20,100): " << updated_feats(20,100) << "    LOG PROB: " << log_probs_updated(20,100);
    KALDI_LOG << "FEATS(50,100): " << updated_feats(50,100) << "    LOG PROB: " << log_probs_updated(50,100);
    KALDI_LOG << "FEATS(0,0): " << updated_feats(0,0) << "    LOG PROB: " << log_probs_updated(0,0);

    
    KALDI_LOG << "MAX: " << max_val;

    KALDI_LOG << "ORIGINAL FEATS: " << feats(rr,cc) << "    " << log_probs(rr,cc);
    KALDI_LOG << "TARGET FEATS: " << feats_target(rr,cc) << "    " << target_log_probs(rr,cc);

    CuMatrix<BaseFloat> fft_diff(num_rows_target, 128);

    KALDI_LOG << "updated_feats.NumRows: " << updated_feats.NumRows();
    KALDI_LOG << "updated_feats.NumCols: " << updated_feats.NumCols();

    KALDI_LOG << "thresholds.NumRows: " << thresholds.NumRows();
    KALDI_LOG << "thresholds.NumCols: " << thresholds.NumCols();

    KALDI_LOG << "log_probs.NumRows: " << log_probs.NumRows();
    KALDI_LOG << "log_probs.NumCols: " << log_probs.NumCols();

    KALDI_LOG << "target_log_probs.NumRows: " << target_log_probs.NumRows();
    KALDI_LOG << "target_log_probs.NumCols: " << target_log_probs.NumCols();

    // Do Spoofing
    CuMatrix<BaseFloat> original_mag(0,0);
    double max_val_dB = 0;
    max_val_dB = DoSpoof(am_nnet.GetNnet(), updated_feats, original_mag, thresholds, thresholds_norm, output_grad, target_log_probs, 0, thresh, &tot_diff, pad_input);
    CuMatrix<BaseFloat> keep_original_mag(original_mag);


    KALDI_LOG << "Num Iterations: " << num_iter;
    KALDI_LOG << "Threshold : " << thresh;

    for (int i = 0; i < num_iter; i++) { 

      // Do Spoofing
      DoSpoof(am_nnet.GetNnet(), updated_feats, keep_original_mag, thresholds, thresholds_norm, output_grad, target_log_probs, max_val_dB, thresh, &tot_diff, pad_input);
      keep_original_mag = original_mag;

      CuSubMatrix<BaseFloat> grad_part(output_grad, am_nnet.GetNnet().LeftContext(),
                                          updated_feats.NumRows(), 0, output_grad.NumCols());

      updated_feats.AddMat(factor*max_val, grad_part); //*100 

      // the following function is declared in nnet-compute.h
      NnetComputation(am_nnet.GetNnet(), updated_feats, pad_input, &log_probs_updated);
      log_probs_updated.ApplyFloor(1.0e-20);

      if(i%50 == 0)
        KALDI_LOG << "FEATS: "  << updated_feats(rr,cc) << "    " << log_probs_updated(rr,cc) << "    " << grad_part(rr,cc);
    }

    saveMatrix(log_probs_updated, utt, path, "utterances");

    KALDI_LOG << "FEATS: " << updated_feats(rr,cc) << "    " << log_probs_updated(rr,cc);

    KALDI_LOG << "FEATS(10,100): " << updated_feats(10,100) << "    LOG PROB: " << log_probs_updated(10,100);
    KALDI_LOG << "FEATS(10,200): " << updated_feats(10,200) << "    LOG PROB: " << log_probs_updated(10,200);
    KALDI_LOG << "FEATS(20,100): " << updated_feats(20,100) << "    LOG PROB: " << log_probs_updated(20,100);
    KALDI_LOG << "FEATS(50,100): " << updated_feats(50,100) << "    LOG PROB: " << log_probs_updated(50,100);
    KALDI_LOG << "FEATS(0,0): " << updated_feats(0,0) << "    LOG PROB: " << log_probs_updated(0,0);

    //updated_feats_normed = reNormalize(updated_feats, utt);
    //updated_feats.Scale(max_val);

    saveMatrix(updated_feats, utt, path, "adversarial");


    log_probs_updated.ApplyFloor(1.0e-20); // Avoid log of zero which leads to NaN.
    log_probs_updated.ApplyLog();
    CuVector<BaseFloat> priors(am_nnet.Priors());
    KALDI_ASSERT(priors.Dim() == trans_model.NumPdfs() &&
                 "Priors in neural network not set up.");
    priors.ApplyLog();
    // subtract log-prior (divide by prior)
    log_probs_updated.AddVecToRows(-1.0, priors);
    // apply probability scale.
    log_probs_updated.Scale(prob_scale);
    // Transfer the log-probs to the CPU for faster access by the
    // decoding process.
    log_probs_.Swap(&log_probs_updated);
  }

  // Note, frames are numbered from zero.  But transition_id is numbered
  // from one (this routine is called by FSTs).
  virtual BaseFloat LogLikelihood(int32 frame, int32 transition_id) {
    return log_probs_(frame,
                      trans_model_.TransitionIdToPdf(transition_id));
  }

  virtual int32 NumFramesReady() const { return log_probs_.NumRows(); }
  
  // Indices are one-based!  This is for compatibility with OpenFst.
  virtual int32 NumIndices() const { return trans_model_.NumTransitionIds(); }
  
  virtual bool IsLastFrame(int32 frame) const {
    KALDI_ASSERT(frame < NumFramesReady());
    return (frame == NumFramesReady() - 1);
  }

  void saveMatrix(CuMatrix<BaseFloat> &input, std::string num_utterance, std::string dir, std::string iter){

    std::string dir_utt = dir + "/" + num_utterance;
    std::string mk_string = "mkdir -p " + dir_utt;
    const int dir_err = system(mk_string.c_str());

    if (-1 == dir_err)
      KALDI_ERR << "Error creating directory!\n";

    std::string file = dir_utt + "/" + iter  + ".csv";

    WriteCuMatrixBaseFloat(file,input);
    //ReadCuMatrixBaseFloat(file, &input);
  };

  CuMatrix<BaseFloat> reNormalize(CuMatrix<BaseFloat> feats, string utt) {

    std::string utt2spk_rspecifier = "ark:/media/lea/Daten/Scibo/Projects/asr_hidden_voice_commands.git/kaldi/egs/yesno/s5/data/test_yesno/split1/1/utt2spk";
    std::string cmvn_rspecifier = "scp:/media/lea/Daten/Scibo/Projects/asr_hidden_voice_commands.git/kaldi/egs/yesno/s5/data/test_yesno/split1/1/cmvn.scp";
    RandomAccessDoubleMatrixReaderMapped cmvn_reader(cmvn_rspecifier,
                                                       utt2spk_rspecifier);


    if (!cmvn_reader.HasKey(utt)) {
      KALDI_WARN << "No normalization statistics available for key "
                 << utt << ", producing no output for this utterance";

    }
    Matrix<double> cmvn_stats = cmvn_reader.Value(utt);
    Matrix<BaseFloat> updated_feats_norm(feats);

    bool norm_vars = true;

    ApplyCmvnReverse(cmvn_stats, norm_vars, &updated_feats_norm);

    CuMatrix<BaseFloat> updated_feats_normed(updated_feats_norm);

    return updated_feats_normed;
  }


 void synthesize(std::string dir_utt, std::string dest_dir, CuMatrix<BaseFloat> abs, Matrix<BaseFloat> *results){

    std::cout.precision(10);

    // Read in arg from csv file
    Matrix<BaseFloat> arg;
    ReadMatrixBaseFloat(dir_utt, &arg, false);

    int nfft = arg.NumCols()-2;
    int idx1 = arg.NumCols()-2;
    int idx2 = arg.NumCols()-1;
 

    SplitRadixRealFft<BaseFloat> srfft(nfft*2);

    results->Resize(2*nfft, arg.NumRows());

    Vector<BaseFloat> abs_row(nfft);
    Vector<BaseFloat> arg_row(nfft);
    Vector<BaseFloat> res;
    for(int r = 0; r < arg.NumRows(); r++) {

      for(int i = 0; i < nfft; i++) {
        abs_row(i) = std::sqrt(std::exp(abs(r,i)));
        arg_row(i) = arg(r,i);
      }

      ComputeComplex(&abs_row, &arg_row, &res);

      res(0) = arg(r,idx1);
      res(1) = arg(r,idx2);

      srfft.Compute(res.Data(), false);

      results->CopyColFromVec(res,r); 
    }

    WriteMatrixBaseFloat(dest_dir, *results);
  };

  void ComputeComplex(VectorBase<BaseFloat> *abs, VectorBase<BaseFloat> *arg, Vector<BaseFloat> *res) {

    int32 dim = arg->Dim();

    res->Resize(2*dim);
    for (int32 i = 0; i < dim; i++) {

      BaseFloat abs_i = (*abs)(i), arg_i = (*arg)(i);

      //KALDI_LOG << "ABS: " << abs_i << " ARG: " << arg_i;
      double real_part = abs_i*std::cos(arg_i);
      double imag_part = abs_i*std::sin(arg_i);

      //KALDI_LOG << "REAL: " << real_part << " IMAG: " << imag_part;
      std::complex<double> c(real_part,imag_part);

      (*res)(2*i) = std::real(c);
      (*res)(2*i+1) = std::imag(c);
    }
  }


 protected:
  const TransitionModel &trans_model_;
  Matrix<BaseFloat> log_probs_; // actually not really probabilities, since we divide
  // by the prior -> they won't sum to one.

  KALDI_DISALLOW_COPY_AND_ASSIGN(DecodableAmNnetSpoofIter);
};

} // namespace nnet2
} // namespace kaldi

#endif  // KALDI_NNET2_DECODABLE_AM_NNET_H_
