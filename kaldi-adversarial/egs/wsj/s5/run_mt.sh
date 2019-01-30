#!/bin/bash
stage=0

dataname=test_dev93_mt_9

thresh=0
itr=100
max_itr=50

numjobs=20
split=20

train=true   # set to false to disable the training-related scripts
             # note: you probably only want to set --train false if you
             # are using at least --stage 1.
decode=true  # set to false to disable the decoding-related scripts.

. ./cmd.sh ## You'll want to change cmd.sh to something that will work on your system.
           ## This relates to the queue.
. ./path.sh
. utils/parse_options.sh  # e.g. this parses the --stage option if supplied.

#data=/media/lea/boennbbc/kaldi_data/wsj/data
data=data
#exp=exp/media/lea/boennbbc/kaldi_data/wsj/exp
exp=exp


# This is a shell script, but it's recommended that you run the commands one by
# one by copying and pasting into the shell.

wsj0=/media/database/WSJ0/csr_senn_d?
wsj1=/media/database/WSJ1/csr_senn_d?


if [ $stage -le 0 ]; then
  # data preparation.
  local/wsj_data_prep.sh $wsj0/??-{?,??}.? $wsj1/??-{?,??}.?  || exit 1;

  # Sometimes, we have seen WSJ distributions that do not have subdirectories
  # like '11-13.1', but instead have 'doc', 'si_et_05', etc. directly under the
  # wsj0 or wsj1 directories. In such cases, try the following:
  #
  # corpus=/exports/work/inf_hcrc_cstr_general/corpora/wsj
  # local/cstr_wsj_data_prep.sh $corpus
  # rm data/local/dict/lexiconp.txt
  # $corpus must contain a 'wsj0' and a 'wsj1' subdirectory for this to work.
  #
  # "nosp" refers to the dictionary before silence probabilities and pronunciation
  # probabilities are added.
  local/wsj_prepare_dict.sh --dict-suffix "_nosp" || exit 1;


  utils/prepare_lang.sh data/local/dict_nosp \
                        "<SPOKEN_NOISE>" data/local/lang_tmp_nosp data/lang_nosp || exit 1;


  local/wsj_format_data.sh --lang-suffix "_nosp" || exit 1;


  # We suggest to run the next three commands in the background,
  # as they are not a precondition for the system building and
  # most of the tests: these commands build a dictionary
  # containing many of the OOVs in the WSJ LM training data,
  # and an LM trained directly on that data (i.e. not just
  # copying the arpa files from the disks from LDC).
  # Caution: the commands below will only work if $decode_cmd
  # is setup to use qsub.  Else, just remove the --cmd option.
  # NOTE: If you have a setup corresponding to the older cstr_wsj_data_prep.sh style,
  # use local/cstr_wsj_extend_dict.sh --dict-suffix "_nosp" $corpus/wsj1/doc/ instead.
  (
    local/wsj_extend_dict.sh --dict-suffix "_nosp" $wsj1/13-32.1  && \
      utils/prepare_lang.sh data/local/dict_nosp_larger \
                            "<SPOKEN_NOISE>" data/local/lang_tmp_nosp_larger data/lang_nosp_bd && \
      local/wsj_train_lms.sh --dict-suffix "_nosp" &&
      local/wsj_format_local_lms.sh --lang-suffix "_nosp" # &&
  ) &


  # Now make MFCC features.
  # mfccdir should be some place with a largish disk where you
  # want to store MFCC features.

  for x in test_eval92 test_eval93 test_dev93 train_si284; do
    steps/make_mfcc.sh --cmd "$train_cmd" --nj 20 data/$x #|| exit 1;
    steps/compute_cmvn_stats.sh data/$x #|| exit 1;
  done

  utils/subset_data_dir.sh --first data/train_si284 7138 data/train_si84 || exit 1

  # Now make subset with the shortest 2k utterances from si-84.
  utils/subset_data_dir.sh --shortest data/train_si84 2000 data/train_si84_2kshort || exit 1;

  # Now make subset with half of the data from si-84.
  utils/subset_data_dir.sh data/train_si84 3500 data/train_si84_half || exit 1;
fi


if [ $stage -le 1 ]; then
  # monophone


  # Note: the --boost-silence option should probably be omitted by default
  # for normal setups.  It doesn't always help. [it's to discourage non-silence
  # models from modeling silence.]
  if $train; then
    steps/train_mono.sh --boost-silence 1.25 --nj 10 --cmd "$train_cmd" \
      data/train_si84_2kshort data/lang_nosp $exp/mono0a || exit 1;
  fi

  if $decode; then
    utils/mkgraph.sh data/lang_nosp_test_tgpr $exp/mono0a $exp/mono0a/graph_nosp_tgpr && \
      steps/decode.sh --nj 10 --cmd "$decode_cmd" $exp/mono0a/graph_nosp_tgpr \
        data/test_dev93 $exp/mono0a/decode_nosp_tgpr_dev93 && \
      steps/decode.sh --nj 8 --cmd "$decode_cmd" $exp/mono0a/graph_nosp_tgpr \
        data/test_eval92 $exp/mono0a/decode_nosp_tgpr_eval92
  fi
fi

if [ $stage -le 2 ]; then
  # tri1
  if $train; then
    steps/align_si.sh --boost-silence 1.25 --nj 10 --cmd "$train_cmd" \
      data/train_si84_half data/lang_nosp $exp/mono0a $exp/mono0a_ali || exit 1;

    steps/train_deltas.sh --boost-silence 1.25 --cmd "$train_cmd" 2000 10000 \
      data/train_si84_half data/lang_nosp $exp/mono0a_ali $exp/tri1 || exit 1;
  fi

  if $decode; then
    utils/mkgraph.sh data/lang_nosp_test_tgpr \
      $exp/tri1 $exp/tri1/graph_nosp_tgpr || exit 1;

    for x in dev93 eval92; do
      nspk=$(wc -l <data/test_${x}/spk2utt)
      steps/decode.sh --nj $nspk --cmd "$decode_cmd" $exp/tri1/graph_nosp_tgpr \
        data/test_${x} $exp/tri1/decode_nosp_tgpr_${x} || exit 1;

      # test various modes of LM rescoring (4 is the default one).
      # This is just confirming they're equivalent.
      for mode in 1 2 3 4 5; do
        steps/lmrescore.sh --mode $mode --cmd "$decode_cmd" \
          data/lang_nosp_test_{tgpr,tg} data/test_${x} \
          $exp/tri1/decode_nosp_tgpr_${x} \
          $exp/tri1/decode_nosp_tgpr_${x}_tg$mode  || exit 1;
      done

    done
  fi
fi

if [ $stage -le 3 ]; then
  # tri2b.  there is no special meaning in the "b"-- it's historical.
  if $train; then
    steps/align_si.sh --nj 10 --cmd "$train_cmd" \
      data/train_si84 data/lang_nosp $exp/tri1 $exp/tri1_ali_si84 || exit 1;

    steps/train_lda_mllt.sh --cmd "$train_cmd" \
      --splice-opts "--left-context=3 --right-context=3" 2500 15000 \
      data/train_si84 data/lang_nosp $exp/tri1_ali_si84 $exp/tri2b || exit 1;
  fi

  if $decode; then
    utils/mkgraph.sh data/lang_nosp_test_tgpr \
      $exp/tri2b $exp/tri2b/graph_nosp_tgpr || exit 1;
    for x in dev93 eval92; do
      nspk=$(wc -l <data/test_${x}/spk2utt)
      steps/decode.sh --nj ${nspk} --cmd "$decode_cmd" $exp/tri2b/graph_nosp_tgpr \
        data/test_${x} $exp/tri2b/decode_nosp_tgpr_${x} || exit 1;

       # compare lattice rescoring with biglm decoding, going from tgpr to tg.
      steps/decode_biglm.sh --nj ${nspk} --cmd "$decode_cmd" \
        $exp/tri2b/graph_nosp_tgpr data/lang_nosp_test_{tgpr,tg}/G.fst \
        data/test_${x} $exp/tri2b/decode_nosp_tgpr_${x}_tg_biglm

       # baseline via LM rescoring of lattices.
      steps/lmrescore.sh --cmd "$decode_cmd" \
        data/lang_nosp_test_tgpr/ data/lang_nosp_test_tg/ \
        data/test_${x} $exp/tri2b/decode_nosp_tgpr_${x} \
        $exp/tri2b/decode_nosp_tgpr_${x}_tg || exit 1;

      # Demonstrating Minimum Bayes Risk decoding (like Confusion Network decoding):
      mkdir $exp/tri2b/decode_nosp_tgpr_${x}_tg_mbr
      cp $exp/tri2b/decode_nosp_tgpr_${x}_tg/lat.*.gz \
         $exp/tri2b/decode_nosp_tgpr_${x}_tg_mbr;
      local/score_mbr.sh --cmd "$decode_cmd"  \
         data/test_${x}/ data/lang_nosp_test_tgpr/ \
         $exp/tri2b/decode_nosp_tgpr_${x}_tg_mbr
    done
  fi
fi


# local/run_delas.sh trains a delta+delta-delta system.  It's not really recommended or
# necessary, but it does contain a demonstration of the decode_fromlats.sh
# script which isn't used elsewhere.
# local/run_deltas.sh

if [ $stage -le 4 ]; then
  # From 2b system, train 3b which is LDA + MLLT + SAT.

  # Align tri2b system with all the si284 data.
  if $train; then
    steps/align_si.sh  --nj 10 --cmd "$train_cmd" \
      data/train_si284 data/lang_nosp $exp/tri2b $exp/tri2b_ali_si284  || exit 1;

    steps/train_sat.sh --cmd "$train_cmd" 4200 40000 \
      data/train_si284 data/lang_nosp $exp/tri2b_ali_si284 $exp/tri3b || exit 1;
  fi

  if $decode; then
    utils/mkgraph.sh data/lang_nosp_test_tgpr \
      $exp/tri3b $exp/tri3b/graph_nosp_tgpr || exit 1;

    # the larger dictionary ("big-dict"/bd) + locally produced LM.
    utils/mkgraph.sh data/lang_nosp_test_bd_tgpr \
      $exp/tri3b $exp/tri3b/graph_nosp_bd_tgpr || exit 1;

    # At this point you could run the command below; this gets
    # results that demonstrate the basis-fMLLR adaptation (adaptation
    # on small amounts of adaptation data).
    # local/run_basis_fmllr.sh --lang-suffix "_nosp"

    for x in dev93 eval92; do
      nspk=$(wc -l <data/test_${x}/spk2utt)
      steps/decode_fmllr.sh --nj ${nspk} --cmd "$decode_cmd" \
        $exp/tri3b/graph_nosp_tgpr data/test_${x} \
        $exp/tri3b/decode_nosp_tgpr_${x} || exit 1;
      steps/lmrescore.sh --cmd "$decode_cmd" \
        data/lang_nosp_test_tgpr data/lang_nosp_test_tg \
        data/test_${x} $exp/tri3b/decode_nosp_{tgpr,tg}_${x} || exit 1

      # decode with big dictionary.
      steps/decode_fmllr.sh --cmd "$decode_cmd" --nj 8 \
        $exp/tri3b/graph_nosp_bd_tgpr data/test_${x} \
        $exp/tri3b/decode_nosp_bd_tgpr_${x} || exit 1;

      # Example of rescoring with ConstArpaLm.
      steps/lmrescore_const_arpa.sh \
        --cmd "$decode_cmd" data/lang_nosp_test_bd_{tgpr,fgconst} \
        data/test_${x} $exp/tri3b/decode_nosp_bd_tgpr_${x}{,_fg} || exit 1;
    done
  fi

fi

if [ $stage -le 5 ]; then
  # Estimate pronunciation and silence probabilities.

  # Silprob for normal lexicon.
  steps/get_prons.sh --cmd "$train_cmd" \
    data/train_si284 data/lang_nosp $exp/tri3b || exit 1;
  utils/dict_dir_add_pronprobs.sh --max-normalize true \
    data/local/dict_nosp \
    $exp/tri3b/pron_counts_nowb.txt $exp/tri3b/sil_counts_nowb.txt \
    $exp/tri3b/pron_bigram_counts_nowb.txt data/local/dict || exit 1

  utils/prepare_lang.sh data/local/dict \
    "<SPOKEN_NOISE>" data/local/lang_tmp data/lang || exit 1;

  for lm_suffix in bg bg_5k tg tg_5k tgpr tgpr_5k; do
    mkdir -p data/lang_test_${lm_suffix}
    cp -r data/lang/* data/lang_test_${lm_suffix}/ || exit 1;
    rm -rf data/lang_test_${lm_suffix}/tmp
    cp data/lang_nosp_test_${lm_suffix}/G.* data/lang_test_${lm_suffix}/
  done

  # Silprob for larger ("bd") lexicon.
  utils/dict_dir_add_pronprobs.sh --max-normalize true \
    data/local/dict_nosp_larger \
    $exp/tri3b/pron_counts_nowb.txt $exp/tri3b/sil_counts_nowb.txt \
    $exp/tri3b/pron_bigram_counts_nowb.txt data/local/dict_larger || exit 1

  utils/prepare_lang.sh data/local/dict_larger \
    "<SPOKEN_NOISE>" data/local/lang_tmp_larger data/lang_bd || exit 1;

  for lm_suffix in tgpr tgconst tg fgpr fgconst fg; do
    mkdir -p data/lang_test_bd_${lm_suffix}
    cp -r data/lang_bd/* data/lang_test_bd_${lm_suffix}/ || exit 1;
    rm -rf data/lang_test_bd_${lm_suffix}/tmp
    cp data/lang_nosp_test_bd_${lm_suffix}/G.* data/lang_test_bd_${lm_suffix}/
  done
fi


if [ $stage -le 6 ]; then
  # From 3b system, now using data/lang as the lang directory (we have now added
  # pronunciation and silence probabilities), train another SAT system (tri4b).

  if $train; then
    steps/train_sat.sh  --cmd "$train_cmd" 4200 40000 \
      data/train_si284 data/lang $exp/tri3b $exp/tri4b || exit 1;
  fi

  if $decode; then
    utils/mkgraph.sh data/lang_test_tgpr \
      $exp/tri4b $exp/tri4b/graph_tgpr || exit 1;
    utils/mkgraph.sh data/lang_test_bd_tgpr \
      $exp/tri4b $exp/tri4b/graph_bd_tgpr || exit 1;

    for x in dev93 eval92; do
      nspk=$(wc -l <data/test_${x}/spk2utt)
      steps/decode_fmllr.sh --nj ${nspk} --cmd "$decode_cmd" \
        $exp/tri4b/graph_tgpr data/test_${x} \
        $exp/tri4b/decode_tgpr_${x} || exit 1;
      steps/lmrescore.sh --cmd "$decode_cmd" \
        data/lang_test_tgpr data/lang_test_tg \
        data/test_${x} $exp/tri4b/decode_{tgpr,tg}_${x} || exit 1

      steps/decode_fmllr.sh --nj ${nspk} --cmd "$decode_cmd" \
        $exp/tri4b/graph_bd_tgpr data/test_${x} \
        $exp/tri4b/decode_bd_tgpr_${x} || exit 1;
      steps/lmrescore_const_arpa.sh \
        --cmd "$decode_cmd" data/lang_test_bd_{tgpr,fgconst} \
        data/test_${x} $exp/tri4b/decode_bd_tgpr_${x}{,_fg} || exit 1;
    done
  fi
fi

if [ $stage -le 7 ]; then


  train_stage=-10
  use_gpu=true
  parallel_opts="--gpu 1"
  num_threads=1
  minibatch_size=512
  dir=exp/nnet5d_gpu

if [ ! -f $dir/final.mdl ]; then
   steps/nnet2/train_pnorm_fast.sh --stage $train_stage \
   --samples-per-iter 400000 \
   --parallel-opts "$parallel_opts" \
   --num-threads "$num_threads" \
   --minibatch-size "$minibatch_size" \
   --num-jobs-nnet 8  --mix-up 8000 \
   --initial-learning-rate 0.02 --final-learning-rate 0.004 \
   --num-hidden-layers 4 \
   --pnorm-input-dim 2000 --pnorm-output-dim 400 \
   --cmd "$decode_cmd" \
    data/train_si284 data/lang exp/tri4b $dir || exit 1
fi



  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 10 \
   --transform-dir exp/tri4b/decode_tgpr_dev93 \
    exp/tri4b/graph_tgpr data/test_dev93 $dir/decode_tgpr_dev93 # &

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 8 \
  --transform-dir exp/tri4b/decode_tgpr_eval92 \
    exp/tri4b/graph_tgpr data/test_eval92 $dir/decode_tgpr_eval92 # &

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 10 \
   --transform-dir exp/tri4b/decode_bd_tgpr_dev93 \
    exp/tri4b/graph_bd_tgpr data/test_dev93 $dir/decode_bd_tgpr_dev93 # &

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 8 \
  --transform-dir exp/tri4b/decode_bd_tgpr_eval92 \
    exp/tri4b/graph_bd_tgpr data/test_eval92 $dir/decode_bd_tgpr_eval92
fi



if [ $stage -le 8 ]; then

for x in test_eval92 test_eval93 test_dev93 train_si284; do
  steps/make_time.sh --cmd "$train_cmd" --nj 20 data/$x || exit 1;
  steps/compute_cmvn_stats.sh data/$x || exit 1;
done

  utils/subset_data_dir.sh --first data/train_si284 7138 data/train_si84 || exit 1

fi




train_stage=-10
use_gpu=true
parallel_opts="--gpu 1"
num_threads=1
minibatch_size=512
model_name=nnet5d_gpu_time
dir=exp/$model_name

if [ $stage -le 9 ]; then


# Attention!! In get_egs.sh change parameter 'num_utts_subset' to define number of validation utterances
if [ ! -f $dir/final.mdl ]; then
steps/nnet2/train_pnorm_fast_time.sh --stage $train_stage \
   --samples-per-iter 400000 \
   --parallel-opts "$parallel_opts" \
   --num-threads "$num_threads" \
   --minibatch-size "$minibatch_size" \
   --num-jobs-nnet 6  --mix-up 8000 \
   --initial-learning-rate 0.02 --final-learning-rate 0.004 \
   --num-hidden-layers 4 \
   --pnorm-input-dim 2000 --pnorm-output-dim 400 \
   --cmd "$decode_cmd" \
    data/train_si284 data/lang exp/tri4b $dir
fi


  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 10 \
    exp/tri4b/graph_tgpr data/test_dev93 $dir/decode_tgpr_dev93 # &

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 8 \
    exp/tri4b/graph_tgpr data/test_eval92 $dir/decode_tgpr_eval92 # &

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 10 \
    exp/tri4b/graph_bd_tgpr data/test_dev93 $dir/decode_bd_tgpr_dev93 # &

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj 8 \
    exp/tri4b/graph_bd_tgpr data/test_eval92 $dir/decode_bd_tgpr_eval92
fi

if [ $stage -le 10 ]; then

  mkdir -p ${dir}/decode_${dataname}/utterances

  utils/spk2utt_to_utt2spk.pl data/${dataname}/spk2utt > data/${dataname}/utt2spk

  for x in $dataname; do
    steps/make_time.sh --cmd "$train_cmd" --nj $split data/$x || exit 1;
    steps/compute_cmvn_stats.sh data/$x || exit 1;
  done

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj $numjobs \
     exp/tri4b/graph_bd_tgpr data/$dataname $dir/decode_$dataname

  # move to adversarial folder
  mkdir -p ${dir}/adversarial_${dataname}/utterances
  mv ${dir}/decode_${dataname}/utterances ${dir}/adversarial_${dataname}

fi

if [ $stage -le 11 ]; then

  mkdir -p $dir/adversarial_${dataname}/thresholds

  cd hearingThresholds/
  matlab -nodesktop -nodisplay -nosplash -r  "calc_threshold('../data/${dataname}/wav.scp', 512, 256, '../${dir}/adversarial_${dataname}/thresholds/')"
  cd ..
fi

if [ $stage -le 12 ]; then
  steps/nnet2/adversarial/adversarial_mt.sh --cmd "$decode_cmd" --nj $numjobs --thresh $thresh --numiter $itr --maxitr $max_itr --experiment $dataname \
    exp/tri4b/graph_bd_tgpr data/$dataname $dir/adversarial_$dataname exp/tri4b data/lang $dir/tree data/adversarial_$dataname $model_name $dir
fi

if [ $stage -le 13 ]; then

  cd steps/nnet2/adversarial/
  matlab -nodesktop -nodisplay -nosplash -r  "synthesize('adversarial-wav/${dataname}_${thresh}dB', '../../../${dir}/adversarial_${dataname}/utterances/', '../../../data/${dataname}/wav.scp', 16000, 256)"
  cd ../../..

  cp $dir/adversarial_${dataname}/scoring_kaldi/wer_details/utt_itr adversarial-wav/${dataname}_${thresh}dB/
  cat > adversarial-wav/${dataname}_${thresh}dB/steps_per_itr << EOL
  Backprob steps per Iteration: $itr
  Maximum number of Iterations: $max_itr
EOL

  for x in adversarial_$dataname; do
    steps/make_time.sh --cmd "$train_cmd" --nj $split data/$x || exit 1;
    steps/compute_cmvn_stats.sh data/$x || exit 1;
  done

  steps/nnet2/decode.sh --cmd "$decode_cmd" --nj $numjobs \
    exp/tri4b/graph_bd_tgpr data/adversarial_$dataname "$dir/decode_adversarial_${dataname}_${thresh}dB"

  for x in "$dir/decode_adversarial_${dataname}_${thresh}dB"; do [ -d $x ] && grep WER $x/wer_* | utils/best_wer.sh; done

fi

