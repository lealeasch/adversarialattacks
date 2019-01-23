// bin/phone-to-post.cc

// author:  Lea Schoenherr

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "hmm/post-sequence.h"
#include "tree/context-dep.h"

int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;

    const char *usage =
        "Calculates posteriors for sentence from tree\n"
        "\n"
        "Usage:  phone-to-post [options] <tree> <dir>\n"
        "e.g.:\n"
        " phone-to-post tree dir\n";
    
    bool binary_write = true;
    
    ParseOptions po(usage);
    po.Register("binary", &binary_write, "Write output in binary mode");
    
    po.Read(argc, argv);
    
    if (po.NumArgs() != 2) {
      po.PrintUsage();
      exit(1);
    }

    std::string tree_rxfilename = po.GetArg(1),
        target = po.GetArg(2);
    
    ContextDependency ctx_dep;
    ReadKaldiObject(tree_rxfilename, &ctx_dep);

    PostSequence *post_sequence = NULL;
    post_sequence = new PostSequence(ctx_dep, target, target);

    return 0;
  } catch(const std::exception &e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
}
