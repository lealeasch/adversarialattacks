// hmm/post-sequence..h

#ifndef KALDI_HMM_POST_SEQUENCE_H_
#define KALDI_HMM_POST_SEQUENCE_H_

#include "base/kaldi-common.h"
#include "tree/context-dep.h"
#include "util/const-integer-set.h"
#include "fst/fst-decl.h" // forward declarations.


namespace kaldi {


class PostSequence {

 public:

  PostSequence(ContextDependencyInterface &ctx_dep, std::string json_rxfilename, std::string wxfilename);

 private:

  /// This is actually one plus the highest-numbered pdf we ever got back from the
  /// tree (but the tree numbers pdfs contiguously from zero so this is the number
  /// of pdfs).
  int32 num_pdfs_;

  // decision tree
  ContextDependencyInterface &ctx_dep_;

  bool ReadSequence(std::string rxfilename, std::string wxfilename);

  void save_all_silentstates(std::string dir);

  std::string ComputeFromString(std::string line);
};



} // end namespace kaldi


#endif
