// nnet2/am-nnet.cc

// Copyright 2012  Johns Hopkins University (author:  Daniel Povey)

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

#include "nnet2/nnet-print-csv.h"

namespace kaldi {
namespace nnet2 {


bool WriteCuMatrixBaseFloat(std::string wxfilename,
                              CuMatrix<BaseFloat> &matrix) {

  std::ofstream outfile;

  outfile.open(wxfilename);

  for(int i = 0; i < matrix.NumRows(); i++) {
    for(int j = 0; j < matrix.NumCols(); j++) {
      BaseFloat this_prob =  matrix(i,j);
      outfile << this_prob << ",";
    }
    outfile << "\n";
  }
  outfile.close();

}

bool ReadCuMatrixBaseFloat(std::string rxfilename, CuMatrix<BaseFloat> *matrix, bool trans) {
    

  std::string data(rxfilename);
  //KALDI_LOG << rxfilename;

  std::ifstream csvread(data.c_str());
  if (!csvread.is_open()){ 
      std::cerr << "Error opening " + rxfilename + "\n"; 
      csvread.close();
      return false;
    }

  std::string line;
  //KALDI_LOG << line;

  int N = 0, M = 0;

  while (getline(csvread, line)) {
    M++;
    //KALDI_LOG << line;

    if(M == 2) {
      std::stringstream ss(line);
      std::string item;
      while (std::getline(ss, item, ',')) {
        //KALDI_LOG << item;
        N++;
      }
      }
  }

  csvread.close(); 

  if(trans)
    matrix->Resize(N, M);
  else
    matrix->Resize(M, N);

  std::ifstream csvread2(data.c_str());
  if (!csvread2.is_open()){ 
    std::cerr << "Error opening " + rxfilename + "\n"; 
    csvread2.close(); 
  }

  for (int m = 0; m < M; m++) {
    getline(csvread2,line);

    std::stringstream ss(line);
    std::string item;

    for (int n = 0; n < N; n++) {
      std::getline(ss, item, ',');
      if(trans)
        (*matrix)(n,m) = stof(item);
      else
        (*matrix)(m,n) = stof(item);
    }
  }

  csvread2.close(); 

  return 0; 
}



bool WriteMatrixBaseFloat(std::string wxfilename,
                              Matrix<BaseFloat> &matrix) {

  std::ofstream outfile;

  outfile.open(wxfilename);

  for(int i = 0; i < matrix.NumRows(); i++) {
    for(int j = 0; j < matrix.NumCols(); j++) {
      BaseFloat this_prob =  matrix(i,j);
      outfile << this_prob << " ";
    }
    outfile << "\n";
  }
  outfile.close();

}



bool ReadMatrixBaseFloat(std::string rxfilename, Matrix<BaseFloat> *matrix, bool trans) {
    

  std::string data(rxfilename);

  std::ifstream csvread(data.c_str());
  if (!csvread.is_open()){ 
      std::cerr << "Error opening " + rxfilename + "\n"; 
      csvread.close(); 
    }

  std::string line;

  int N = 0, M = 0;

  while (getline(csvread,line)) {
    M++;

    if(M == 2) {
      std::stringstream ss(line);
      std::string item;
      while (std::getline(ss, item, ' '))
        N++;
      }
  }

  csvread.close(); 

  if(trans)
    matrix->Resize(N, M);
  else
    matrix->Resize(M, N);

  std::ifstream csvread2(data.c_str());
  if (!csvread2.is_open()){ 
    std::cerr << "Error opening " + rxfilename + "\n"; 
    csvread2.close(); 
  }

  for (int m = 0; m < M; m++) {
    getline(csvread2,line);

    std::stringstream ss(line);
    std::string item;

    for (int n = 0; n < N; n++) {
      std::getline(ss, item, ' ');
      if(trans)
        (*matrix)(n,m) = stof(item);
      else
        (*matrix)(m,n) = stof(item);
    }
  }

  csvread2.close(); 

  return 0; 
}

} // namespace nnet2
} // namespace kaldi

