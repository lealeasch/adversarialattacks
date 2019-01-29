// nnet2/nnet-update.cc

// Copyright 2012   Johns Hopkins University (author: Daniel Povey)
//           2014   Xiaohui Zhang

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

#include "nnet2/nnet-update-spoof.h"

namespace kaldi {
namespace nnet2 {


SpoofUpdater::SpoofUpdater(const Nnet &nnet):
    nnet_(nnet) {
}

void SpoofUpdater::ComputeForExample(const CuMatrixBase<BaseFloat> &data, CuMatrix<BaseFloat> &fft_feat, bool pad) {

  int32 dim = data.NumCols();

  forward_data_.resize(nnet_.NumComponents() + 1);


   int32 left_context = (pad ? nnet_.LeftContext() : 0),
       right_context = (pad ? nnet_.RightContext() : 0);

  int32 num_rows = left_context + data.NumRows() + right_context;
  nnet_.ComputeChunkInfo(num_rows, 1, &chunk_info_out_);

  CuMatrix<BaseFloat> &input(forward_data_[0]);
  input.Resize(num_rows, dim);
  input.Range(left_context, data.NumRows(),
              0, dim).CopyFromMat(data);
  for (int32 i = 0; i < left_context; i++)
    input.Row(i).CopyFromVec(data.Row(0));
  int32 last_row = data.NumRows() - 1;
  for (int32 i = 0; i < right_context; i++)
    input.Row(num_rows - i - 1).CopyFromVec(data.Row(last_row));

  Propagate();

  int32 num_components = nnet_.NumComponents();
  for (int32 c = 0; c < num_components; c++) {
    const Component &component = nnet_.GetComponent(c);

    if(component.Type().compare("FftComponent") == 0) {
      fft_feat.Resize(forward_data_[c+1].NumRows(), forward_data_[c+1].NumCols());
      CalculateMagnitute(forward_data_[c+1], &fft_feat);
      return;
    }
  }

  // in case something goes wrong
  fft_feat.Resize(0,0);
  return;
}



double SpoofUpdater::ComputeForExample(const CuMatrixBase<BaseFloat> &data, CuMatrix<BaseFloat> &original_magn, CuMatrix<BaseFloat> &threshhold, CuMatrix<BaseFloat> &thresholds_norm, const CuMatrix<BaseFloat> &prob_target, CuMatrix<BaseFloat> &deriv, double *tot_diff, bool pad, double max_val_dB, double thresh) {

  int32 dim = data.NumCols();

  forward_data_.resize(nnet_.NumComponents() + 1);


   int32 left_context = (pad ? nnet_.LeftContext() : 0),
       right_context = (pad ? nnet_.RightContext() : 0);

  int32 num_rows = left_context + data.NumRows() + right_context;
  nnet_.ComputeChunkInfo(num_rows, 1, &chunk_info_out_);

  CuMatrix<BaseFloat> &input(forward_data_[0]);
  input.Resize(num_rows, dim);
  input.Range(left_context, data.NumRows(),
              0, dim).CopyFromMat(data);
  for (int32 i = 0; i < left_context; i++)
    input.Row(i).CopyFromVec(data.Row(0));
  int32 last_row = data.NumRows() - 1;
  for (int32 i = 0; i < right_context; i++)
    input.Row(num_rows - i - 1).CopyFromVec(data.Row(last_row));

  Propagate();

  double ans = ComputeObjfAndDeriv(data, prob_target, &deriv, tot_diff);

  max_val_dB = BackpropInput(&deriv, original_magn, threshhold, thresholds_norm, max_val_dB, thresh); // this is summed (after weighting), not averaged.

  return max_val_dB;
}

double SpoofUpdater::ComputeObjfAndDeriv(
    const CuMatrixBase<BaseFloat> &data,
    const CuMatrix<BaseFloat> &prob_target,
    CuMatrix<BaseFloat> *deriv,
    double *tot_diff = 0) const {
  BaseFloat tot_objf = 0.0, tot_weight = 0.0;
  int32 num_components = nnet_.NumComponents();
  int32 num_chunks = data.NumRows();
  deriv->Resize(num_chunks, nnet_.OutputDim()); // sets to zero.
  const CuMatrix<BaseFloat> &output(forward_data_[num_components]);
  //KALDI_ASSERT(SameDim(output, *deriv));

  std::vector<MatrixElement<BaseFloat> > sv_labels;
  sv_labels.reserve(num_chunks); // We must have at least this many labels.


  int32 end_idx;
  if(num_chunks < prob_target.NumRows()) 
    end_idx = num_chunks;
  else
    end_idx = prob_target.NumRows();
  for (int32 m = 0; m < end_idx; m++) {

      for (int32 n = 0; n < prob_target.NumCols(); n++) {
        MatrixElement<BaseFloat> elem = {m, n, prob_target(m,n)};
        sv_labels.push_back(elem);
    }
  }

  deriv->CompObjfAndDeriv(sv_labels, output, &tot_objf, &tot_weight);

  return 0;
}


void SpoofUpdater::GetOutput(CuMatrix<BaseFloat> *output) {
  int32 num_components = nnet_.NumComponents(); 
  KALDI_ASSERT(forward_data_.size() == nnet_.NumComponents() + 1); 
  *output = forward_data_[num_components];
}

void SpoofUpdater::Propagate() {
  static int32 num_times_printed = 0;
        
  int32 num_components = nnet_.NumComponents();
  for (int32 c = 0; c < num_components; c++) {
    const Component &component = nnet_.GetComponent(c);
    const CuMatrix<BaseFloat> &input = forward_data_[c];
    CuMatrix<BaseFloat> &output = forward_data_[c+1];
    // Note: the Propagate function will automatically resize the
    // output.
    component.Propagate(chunk_info_out_[c], chunk_info_out_[c+1], input, &output);

    // If we won't need the output of the previous layer for
    // backprop, delete it to save memory.
    bool need_last_output =
        (c>0 && nnet_.GetComponent(c-1).BackpropNeedsOutput()) ||
        component.BackpropNeedsInput() || component.Type().compare("FftComponent");
    if (g_kaldi_verbose_level >= 3 && num_times_printed < 100) {
      KALDI_VLOG(3) << "Stddev of data for component " << c
                    << " for this minibatch is "
                    << (TraceMatMat(forward_data_[c], forward_data_[c], kTrans) /
                        (forward_data_[c].NumRows() * forward_data_[c].NumCols()));
      num_times_printed++;
    }
    if (!need_last_output)
      forward_data_[c].Resize(0, 0); // We won't need this data.
  }
}

double SpoofUpdater::ComputeTotAccuracy(
    const std::vector<NnetExample> &data) const {
  BaseFloat tot_accuracy = 0.0;
  int32 num_components = nnet_.NumComponents();
  const CuMatrix<BaseFloat> &output(forward_data_[num_components]);
  KALDI_ASSERT(output.NumRows() == static_cast<int32>(data.size()));
  CuArray<int32> best_pdf(output.NumRows());
  std::vector<int32> best_pdf_cpu;
  
  output.FindRowMaxId(&best_pdf);
  best_pdf.CopyToVec(&best_pdf_cpu);

  for (int32 i = 0; i < output.NumRows(); i++) {
    KALDI_ASSERT(data[i].labels.size() == 1 &&
                 "Training code currently does not support multi-frame egs");
    const std::vector<std::pair<int32,BaseFloat> > &labels = data[i].labels[0];
    for (size_t j = 0; j < labels.size(); j++) {
      int32 ref_pdf_id = labels[j].first,
          hyp_pdf_id = best_pdf_cpu[i];
      BaseFloat weight = labels[j].second;
      tot_accuracy += weight * (hyp_pdf_id == ref_pdf_id ? 1.0 : 0.0);
    }
  }
  return tot_accuracy;
}


double SpoofUpdater::BackpropInput(CuMatrix<BaseFloat> *deriv, CuMatrix<BaseFloat> &original_magn, CuMatrix<BaseFloat> &threshhold, CuMatrix<BaseFloat> &thresholds_norm, double max_val_dB, double thresh) {

  // We assume ComputeObjfAndDeriv has already been called.
  for (int32 c = nnet_.NumComponents()-1;c >= 0; c--) {
    const Component &component = nnet_.GetComponent(c);

    Component *component_to_update = NULL; // for training of NN only
    const CuMatrix<BaseFloat> &input = forward_data_[c],
        &output = forward_data_[c+1];
    CuMatrix<BaseFloat> input_deriv(input.NumRows(), input.NumCols());

    if(component.Type().compare("FftComponent") == 0 && thresh != -1)
    {
      int rr = 216+4-1; //330
      int cc = 241-1;//218
      int offset = output.NumCols()/2;

      CuMatrix<BaseFloat> modified(output);
      CuMatrix<BaseFloat> modified_dB(modified.NumRows(), modified.NumCols());
      CuMatrix<BaseFloat> scale_factors(modified.NumRows(), modified.NumCols());
      CuMatrix<BaseFloat> scale_factors2(thresholds_norm);


      if(original_magn.NumRows() != 0) {

        modified.AddMat(-1, original_magn);
        CalculateMagnitute(modified, &modified_dB);
        modified_dB.Add(-max_val_dB);


        CuMatrix<BaseFloat> temp(threshhold);
        temp.Add(-95.0+thresh);

        modified_dB.AddMat(-1, temp);

        scale_factors = modified_dB;
        scale_factors.Scale(-1.0/(66.2266+95.0));

        scale_factors.ApplyFloor(1.0e-20);

        deriv->MulElements(scale_factors);


      } else {
        CalculateMagnitute(output, &modified_dB);
        max_val_dB = std::numeric_limits<BaseFloat>::epsilon();
        for(int c = 0; c < modified_dB.NumCols(); c++) {
          for(int r = 0; r <  modified_dB.NumRows(); r++) {

            if(modified_dB(r,c) > max_val_dB)
              max_val_dB = modified_dB(r,c);
            }
        }
        KALDI_LOG << "Max in dB: " << max_val_dB;
      }
      //scale_factors2.Scale(0.1);
      deriv->MulElements(scale_factors2);
      original_magn = output;
    }



    const CuMatrix<BaseFloat> &output_deriv(*deriv);
    component.Backprop(chunk_info_out_[c], chunk_info_out_[c+1], input, output,                       
                       output_deriv, component_to_update,
                       &input_deriv);



    input_deriv.Swap(deriv);
  }

  return max_val_dB;
}


void SpoofUpdater::CalculateMagnitute(const CuMatrix<BaseFloat> &complex, CuMatrix<BaseFloat> *magnitude) {

  if(complex.NumCols() == 0 || complex.NumRows() == 0) {
    KALDI_LOG << "Wrong Dimension";
    return;
  }

  CuMatrix<BaseFloat> temp(complex);
  double min = std::numeric_limits<BaseFloat>::epsilon(); //0.0001;//std::numeric_limits<BaseFloat>::epsilon()*std::numeric_limits<BaseFloat>::epsilon();
  for(int c = 0; c < temp.NumCols()/2; c++) {
    for(int r = 0; r <  temp.NumRows(); r++) {

      double real = temp(r,c)*temp(r,c);
      double imag = temp(r,c+temp.NumCols()/2)*temp(r,c+temp.NumCols()/2);

      if(real < min)
        real = min;

      if(imag < min)
        imag = min;

      double mag = 20*log10(sqrt(real + imag));
      (*magnitude)(r,c) = mag;
      (*magnitude)(r,c+temp.NumCols()/2) = mag;
    }
  }
}


bool PrintMatrix(CuMatrix<BaseFloat> A, string name) {

    std::cout << name << ":" << std::endl;

  for(int j = 0; j < A.NumRows(); j++) {
    for(int i = 0; i < A.NumCols(); i++) {
      std::cout << A(i,j) << "\t";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;


  return true;
}


double DoSpoof(const Nnet &nnet,
               const CuMatrixBase<BaseFloat> &feat,
               CuMatrix<BaseFloat> &fft_diff,
               CuMatrix<BaseFloat> &threshhold,
               CuMatrix<BaseFloat> &thresholds_norm,
               CuMatrix<BaseFloat> &deriv,
               const CuMatrix<BaseFloat> &prob_target,
               double max_val_dB,
               double thresh,
               double *tot_diff, 
               bool pad) {
  try {
    SpoofUpdater updater(nnet);
    // CuMatrix<BaseFloat> deriv;
    double x = updater.ComputeForExample(feat, fft_diff, threshhold, thresholds_norm, prob_target, deriv, tot_diff, pad, max_val_dB, thresh);

    return x;
  } catch (...) {
    KALDI_LOG << "Error doing spoofing, nnet info is: " << nnet.Info();
    throw;
  }
}

void DoPropagteToFFT(const Nnet &nnet,
               const CuMatrixBase<BaseFloat> &feat,
              CuMatrix<BaseFloat> &fft_feat,
              bool pad) {
  try {
    SpoofUpdater updater(nnet);
    updater.ComputeForExample(feat, fft_feat, pad);

  } catch (...) {
    KALDI_LOG << "Error doing propagate, nnet info is: " << nnet.Info();
    throw;
  }
}



  
  
} // namespace nnet2
} // namespace kaldi
