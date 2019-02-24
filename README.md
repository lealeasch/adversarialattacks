Kaldi Speech Recognition Toolkit including Adversarial Examples
================================

- Dependencies: CUDA (tested with CUDA 10) and Matlab (for the hearing thresholds), all other dependencies can be found in the kaldi install instructions:

- To build kaldi: see `./kaldi-adversarial/INSTALL`.  

- The receipt can be found in `./kaldi-adversarial/egs/wsj/s5/run_mt.sh`. 

- wsj data set is required

Paper
================================

If you want to cite the paper, please use the following BibTeX entry:


@INPROCEEDINGS{Schoenherr2019,

  author = {Lea Sch\"{o}nherr and Katharina Kohls and Steffen Zeiler and Thorsten
  Holz and Dorothea Kolossa},
  
  title = {Adversarial Attacks Against Automatic Speech Recognition Systems
  via Psychoacoustic Hiding},
  
  booktitle = {accepted for Publication, NDSS},
  
  year = {2019}
  
}


Description of stages:
================================
- Stage 0 to 7 is the unchanged nnet2 receipt for wsj

- Stage 8: feature extraction for time features

- Stage 9: DNN training for time features, the real feature extraction is included in the DNN

- Stage 10: decode data set which is used to create adversarial examples ($dataname), this information is used to create first targets

- Stage 11: calculates hearing thresholds. 

- Stage 12: calculates adversarial examples in $itr steps. Also includes the forced alignment step. Each steps conducts 100 backpropagation steps. The algorithm stops if the adversarial was successful and will only continue with the audio files, which have not been successful, yet.

- Stage 13: stores .wav file and test the files.


data description:
================================
The neccessary files are stored in `./kaldi-adversarial/egs/wsj/s5/data/` and almost follows the standard kaldi data description

- wav.scp: utterance_id wav file
- spk2utt: skeaker_id utterance_id
	- use this file to define one utterance per speaker, which will help to parallelize the algorithm, inpedendent from the real speaker id, if exists (split is defined per speaker)
- spk2gender: skeaker_id gender
	- not really necessary, might be removed in a later release, can be set arbitrary
- text: utterance_id text
	- any text, or the real transcription (not the target text)
- target: utterance_id index_to_target
	- refers to ./kaldi-adversarial/egs/wsj/s5/targets/target-utterances.txt (line - 1)
	- if you want a new target, add the new text, one per line, or modify this file


parameter description in run_mt.sh:
================================

- stage: start stage
- dataname: same name as the prepared folder in data/
- thresh: lambda value, if equals -1 no hearing thresholds are used
- itr: number of max iterations. $itr * 100 defines backpropagation steps
- numjobs: max the number of speakers in the $dataname data
- split: max the number of speakers in the $dataname data
