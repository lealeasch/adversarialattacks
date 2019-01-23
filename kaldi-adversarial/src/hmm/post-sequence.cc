// hmm/post-sequence.cc

#include <vector>
#include <list>
#include <algorithm>
#include "hmm/post-sequence.h"
#include "tree/context-dep.h"

namespace kaldi {

PostSequence::PostSequence(ContextDependencyInterface &ctx_dep, std::string rxfilename, std::string wxfilename): 
  ctx_dep_(ctx_dep){

  if(!ReadSequence(rxfilename, wxfilename))
    KALDI_ERR << "failed reading file " << rxfilename;

  save_all_silentstates(rxfilename);
}

void PostSequence::save_all_silentstates(std::string dir) {

  std::string data(dir + "/sil.txt");
  std::ofstream out(dir + "/sil-post.txt");

  std::ifstream fileread(data.c_str());
  if (!fileread.is_open()){
      std::cerr << "Error opening file" + dir + "/sil.txt";
      fileread.close();
      return;
  }

  std::list<int32> sil_post;

  const std::vector<int32> phones = {0,0,0};
  int32 phone = 0;
  int32 pdf_id = -2;

  int32 num_phones;
  std::string line;
  getline(fileread,line);
  std::istringstream s_t(line);
  s_t >> num_phones;

  getline(fileread,line);
  std::istringstream ss(line.substr(1,line.length()-2));
  std::string item;
  while(getline(ss, item, ',')) {
    std::istringstream s_it(item);
    s_it >> phone;
    for(int32 i = 1; i <= num_phones; i++) {
      for(int32 j = 1; j <= num_phones; j++) {
        for(int32 k = 0; k < 5; k++) {
          const std::vector<int32> phones = {i,phone,j};
          ctx_dep_.Compute(phones, k, &pdf_id);
          if(std::find(std::begin(sil_post), std::end(sil_post), pdf_id) == std::end(sil_post)) {
            sil_post.push_back(pdf_id);
            out << pdf_id <<",";
          }
        }
      }
    }
  }

  out.close();
}


bool PostSequence::ReadSequence(std::string rxfilename, std::string wxfilename) {

  std::ofstream out(wxfilename + "/target-post-sequence.txt");
  std::string data(rxfilename + "/target-sequence.txt");

  std::ifstream fileread(data.c_str());
  if (!fileread.is_open()){
      std::cerr << "Error opening file";
      fileread.close();
      return false;
    }

  std::string line;
  getline(fileread,line);
  if(line.compare("transcription") != 0)
    return false;
  while(getline(fileread,line)) {
    out << "transcription\n";
    out << line + "\n";
    getline(fileread,line);
    if(line.compare("silence") != 0)
      return false;
    out << "silence\n";
    getline(fileread,line);
    while(line.compare("no silence") != 0) {
      std::string res = ComputeFromString(line);
      out << res;
      getline(fileread,line);
    }
    out << "no silence\n";
    getline(fileread,line);
    while(line.compare("transcription") != 0 && !fileread.eof() ) {
      std::string res = ComputeFromString(line);
      out << res;
      getline(fileread,line);
    }
  }

  fileread.close(); 
  out.close();

  return true; 
}

std::string PostSequence::ComputeFromString(std::string line) {

  int32 first, second, third , hmm_state;
  std::istringstream ss(line.substr(2,line.length()));
  std::string item;
  getline(ss, item, ',');
  std::istringstream s_first(item);
  s_first >> first;
  getline(ss, item, ',');
  std::istringstream s_second(item);
  s_second >> second;
  getline(ss, item, ',');
  std::istringstream s_third(item);
  s_third >> third;
  getline(ss, item, ']');
  std::istringstream s_hmm(item);
  s_hmm >> hmm_state;

  const std::vector<int32> phones = {first,second,third};
  int32 pdf_id = -2;
  ctx_dep_.Compute(phones, hmm_state, &pdf_id);

  return std::to_string(phones[0]) + ',' + 
          std::to_string(phones[1]) + ',' + 
          std::to_string(phones[2]) + ',' + 
          std::to_string(hmm_state) + ',' + 
          std::to_string(pdf_id) + '\n';
}


} // End namespace kaldi
