// nnet2bin/nnet-latgen-faster.cc

// Copyright 2009-2012   Microsoft Corporation
//                       Johns Hopkins University (author: Daniel Povey)
//                2014   Guoguo Chen

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


#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "fstext/kaldi-fst-io.h"
#include "decoder/decoder-wrappers.h"
#include "nnet2/decodable-am-nnet.h"
#include "base/timer.h"


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    using namespace kaldi::nnet2;
    typedef kaldi::int32 int32;
    using fst::SymbolTable;
    using fst::Fst;
    using fst::StdArc;

    const char *usage =
        "Generate lattices using neural net model.\n"
        "Usage: nnet-latgen-faster [options] <nnet-in> <fst-in|fsts-rspecifier> <features-rspecifier>"
        " <lattice-wspecifier> [ <words-wspecifier> [<alignments-wspecifier>] ]\n";
    ParseOptions po(usage);
    Timer timer;
    bool allow_partial = false;
    BaseFloat acoustic_scale = 0.1;
    LatticeFasterDecoderConfig config;
    std::string use_gpu = "yes";

    std::string word_syms_filename;
    config.Register(&po);
    po.Register("acoustic-scale", &acoustic_scale, "Scaling factor for acoustic likelihoods");
    po.Register("word-symbol-table", &word_syms_filename, "Symbol table for words [for debug output]");
    po.Register("allow-partial", &allow_partial, "If true, produce output even if end state was not reached.");

    po.Read(argc, argv);

    if (po.NumArgs() < 8 || po.NumArgs() > 10) {
      po.PrintUsage();
      exit(1);
    }

/*#if HAVE_CUDA==1
    CuDevice::Instantiate().SelectGpuId(use_gpu);
#endif*/

    std::string model_in_filename = po.GetArg(1),
        fst_in_str = po.GetArg(2),
        feature_rspecifier = po.GetArg(3),
        lattice_wspecifier = po.GetArg(4),
        path = po.GetArg(5),
        itr = po.GetOptArg(6),
        thr = po.GetOptArg(7),
        rxfilename = po.GetOptArg(8),
        words_wspecifier = po.GetOptArg(9),
        alignment_wspecifier = po.GetOptArg(10);

    int num_iter = std::stoi(itr);
    double thresh = std::stod(thr);

  //std::string rxfilename = "exp/nnet5d_gpu_time/spoof_test_dev93_mt_1/scoring_kaldi/wer_details/utt_itr";
  std::string data(rxfilename);

    TransitionModel trans_model;
    AmNnet am_nnet;
    {
      bool binary;
      Input ki(model_in_filename, &binary);
      trans_model.Read(ki.Stream(), binary);
      am_nnet.Read(ki.Stream(), binary);
    }

    bool determinize = config.determinize_lattice;
    CompactLatticeWriter compact_lattice_writer;
    LatticeWriter lattice_writer;
    if (! (determinize ? compact_lattice_writer.Open(lattice_wspecifier)
           : lattice_writer.Open(lattice_wspecifier)))
      KALDI_ERR << "Could not open table for writing lattices: "
                 << lattice_wspecifier;

    Int32VectorWriter words_writer(words_wspecifier);

    Int32VectorWriter alignment_writer(alignment_wspecifier);

    fst::SymbolTable *word_syms = NULL;
    if (word_syms_filename != "")
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "
                   << word_syms_filename;


    double tot_like = 0.0;
    kaldi::int64 frame_count = 0;
    int num_success = 0, num_fail = 0;

    std::string arg_dir = "";
    SequentialBaseFloatCuMatrixReader target_feature_reader(feature_rspecifier);

    const CuMatrix<BaseFloat> &features_target(target_feature_reader.Value());

    if (ClassifyRspecifier(fst_in_str, NULL, NULL) == kNoRspecifier) {
      SequentialBaseFloatCuMatrixReader feature_reader(feature_rspecifier);

      // Input FST is just one FST, not a table of FSTs.
      Fst<StdArc> *decode_fst = fst::ReadFstKaldiGeneric(fst_in_str);
      timer.Reset();

      {
        LatticeFasterDecoder decoder(*decode_fst, config);
        int num_utt = 1;
        for (; !feature_reader.Done(); feature_reader.Next()) {
          std::string utt = feature_reader.Key();

          std::ifstream csvread(data.c_str());
          if (!csvread.is_open()){ 
              std::cerr << "Error opening " + rxfilename + "\n"; 
              csvread.close();
          }

          // check if next audio file is already changed succesfully
          std::string line;
          std::string item;
          while (getline(csvread, line)) {

            std::stringstream ss(line);

            std::getline(ss, item, ' ');

            if(item.compare(utt) == 0) {
              KALDI_LOG << "Break at utt: " << utt;
                break;
              }
          }
          csvread.close();

          if(item.compare(utt) == 0) {
            KALDI_LOG << "Continue at utt: " << utt;
            continue;
          }

          utt = feature_reader.Key();
          const CuMatrix<BaseFloat> &features(feature_reader.Value());

          if (features.NumRows() == 0) {
            KALDI_WARN << "Zero-length utterance: " << utt;
            num_fail++;
            continue;
          }

          bool pad_input = true;
          DecodableAmNnetSpoofIter nnet_decodable(trans_model,
                                         am_nnet,
                                         features,
                                         features_target,
                                         utt,
                                         pad_input,
                                         acoustic_scale,
                                         num_utt,
                                         num_iter,
                                         thresh,
                                         path,
                                         arg_dir);
          double like;
          if (DecodeUtteranceLatticeFaster(
                  decoder, nnet_decodable, trans_model, word_syms, utt,
                  acoustic_scale, determinize, allow_partial, &alignment_writer,
                  &words_writer, &compact_lattice_writer, &lattice_writer,
                  &like)) {
            tot_like += like;
            frame_count += features.NumRows();
            num_success++;
            num_utt++;
          } else num_fail++;
        }
      }
      delete decode_fst; // delete this only after decoder goes out of scope.
    } else { // We have different FSTs for different utterances.
      SequentialTableReader<fst::VectorFstHolder> fst_reader(fst_in_str);
      RandomAccessBaseFloatCuMatrixReader feature_reader(feature_rspecifier);

      int num_utt = 1;
      for (; !fst_reader.Done(); fst_reader.Next()) {
        std::string utt = fst_reader.Key();
        if (!feature_reader.HasKey(utt)) {
          KALDI_WARN << "Not decoding utterance " << utt
                     << " because no features available.";
          num_fail++;
          continue;
        }
        const CuMatrix<BaseFloat> &features = feature_reader.Value(utt);
        if (features.NumRows() == 0) {
          KALDI_WARN << "Zero-length utterance: " << utt;
          num_fail++;
          continue;
        }

        LatticeFasterDecoder decoder(fst_reader.Value(), config);

        bool pad_input = true;
          DecodableAmNnetSpoofIter nnet_decodable(trans_model,
                                         am_nnet,
                                         features,
                                         features_target,
                                         utt,
                                         pad_input,
                                         acoustic_scale,
                                         num_utt,
                                         num_iter,
                                         thresh,
                                         path,
                                         arg_dir);
        double like;
        if (DecodeUtteranceLatticeFaster(
                decoder, nnet_decodable, trans_model, word_syms, utt,
                acoustic_scale, determinize, allow_partial, &alignment_writer,
                &words_writer, &compact_lattice_writer, &lattice_writer,
                &like)) {
          tot_like += like;
          frame_count += features.NumRows();
          num_success++;
          num_utt++;
        } else num_fail++;
      }
    }

    double elapsed = timer.Elapsed();
    KALDI_LOG << "Time taken "<< elapsed
              << "s: real-time factor assuming 100 frames/sec is "
              << (elapsed*100.0/frame_count);
    KALDI_LOG << "Done " << num_success << " utterances, failed for "
              << num_fail;
    KALDI_LOG << "Overall log-likelihood per frame is " << (tot_like/frame_count) << " over "
              << frame_count<<" frames.";

    delete word_syms;
    if (num_success != 0) return 0;
    else return 1;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}
