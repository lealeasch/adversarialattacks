// feat/feature-time.cc

// Copyright 2009-2011  Karel Vesely;  Petr Motlicek
//                2016  Johns Hopkins University (author: Daniel Povey)

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


#include "feat/feature-time.h"


namespace kaldi {


void TimeComputer::Compute(BaseFloat signal_log_energy,
                           BaseFloat vtln_warp,
                           VectorBase<BaseFloat> *signal_frame,
                           VectorBase<BaseFloat> *feature) {
  KALDI_ASSERT(signal_frame->Dim() == opts_.frame_opts.PaddedWindowSize() &&
               feature->Dim() == this->Dim());

  SubVector<BaseFloat> power_spectrum(*signal_frame,
                                      0, signal_frame->Dim());


  feature->CopyFromVec(power_spectrum);
}

TimeComputer::TimeComputer(const TimeOptions &opts):
    opts_(opts), srfft_(NULL),
    mel_energies_(opts.mel_opts.num_bins) {
  int32 num_bins = opts.mel_opts.num_bins;
  Matrix<BaseFloat> dct_matrix(num_bins, num_bins);
  ComputeDctMatrix(&dct_matrix);
  // Note that we include zeroth dct in either case.  If using the
  // energy we replace this with the energy.  This means a different
  // ordering of features than HTK.
  SubMatrix<BaseFloat> dct_rows(dct_matrix, 0, opts.num_ceps, 0, num_bins);
  dct_matrix_.Resize(opts.num_ceps, num_bins);
  dct_matrix_.CopyFromMat(dct_rows);  // subset of rows.
  if (opts.cepstral_lifter != 0.0) {
    lifter_coeffs_.Resize(opts.num_ceps);
    ComputeLifterCoeffs(opts.cepstral_lifter, &lifter_coeffs_);
  }
  if (opts.energy_floor > 0.0)
    log_energy_floor_ = Log(opts.energy_floor);

  int32 padded_window_size = opts.frame_opts.PaddedWindowSize();
  if ((padded_window_size & (padded_window_size-1)) == 0)  // Is a power of two...
    srfft_ = new SplitRadixRealFft<BaseFloat>(padded_window_size);

  // We'll definitely need the filterbanks info for VTLN warping factor 1.0.
  // [note: this call caches it.]
  GetMelBanks(1.0);
}

TimeComputer::TimeComputer(const TimeComputer &other):
    opts_(other.opts_), lifter_coeffs_(other.lifter_coeffs_),
    dct_matrix_(other.dct_matrix_),
    log_energy_floor_(other.log_energy_floor_),
    mel_banks_(other.mel_banks_),
    srfft_(NULL),
    mel_energies_(other.mel_energies_.Dim(), kUndefined) {
  for (std::map<BaseFloat, MelBanks*>::iterator iter = mel_banks_.begin();
       iter != mel_banks_.end(); ++iter)
    iter->second = new MelBanks(*(iter->second));
  if (other.srfft_ != NULL)
    srfft_ = new SplitRadixRealFft<BaseFloat>(*(other.srfft_));
}



TimeComputer::~TimeComputer() {
  for (std::map<BaseFloat, MelBanks*>::iterator iter = mel_banks_.begin();
      iter != mel_banks_.end();
      ++iter)
    delete iter->second;
  delete srfft_;
}

const MelBanks *TimeComputer::GetMelBanks(BaseFloat vtln_warp) {
  MelBanks *this_mel_banks = NULL;
  std::map<BaseFloat, MelBanks*>::iterator iter = mel_banks_.find(vtln_warp);
  if (iter == mel_banks_.end()) {
    this_mel_banks = new MelBanks(opts_.mel_opts,
                                  opts_.frame_opts,
                                  vtln_warp);
    mel_banks_[vtln_warp] = this_mel_banks;
  } else {
    this_mel_banks = iter->second;
  }
  return this_mel_banks;
}



}  // namespace kaldi
