Kaldi Speech Recognition Toolkit including Adversarial Examples
================================

- Dependencies: CUDA (only tested with CUDA 10) and Matlab (for the hearing thresholds), all other dependencies can be found in the kaldi install instructions and moslty install automatically.

- To build kaldi: see `./kaldi-adversarial/INSTALL`.  

- The receipt can be found in `./kaldi-adversarial/egs/wsj/s5/run_mt.sh`. 

- wsj data set is required


Description of stages:
================================
- Stage 0 to 7 are the normal nnet2 receipt for wsj

- Stage 8: feature extraction for time features

- Stage 9: DNN training for time features, the real feature extraction is included into the DNN

- Stage 10: decode data set which is used to create adversarial examples ($dataname), this infomration is used to create first targets

- Stage 11: calculates hearing thresholds. Unformtunatly, this steps depend on Matlab Code

- Stage 12: calculates adversarial examples in $itr steps. Also includes the forced alignment step. Each steps conducts 100 backprob steps. The algorithms stops, if the adversarial was successful and will only continue with the audio files, which have not been successfull, yet.

- Stage 13: stores .wav file and test the files.


data description:
================================
The neccessary files are stored in `./kaldi-adversarial/egs/wsj/s5/data/` and almost follows the standard kaldi data description

- wav.scp: utterance_id wav file
- spk2utt: skeaker_id utterance_id
	- use this file to define one utterance per speaker, which will help to parallize the algorithm, unpedendent from the real speaker id, if exists (split is defined per speaker)
- spk2gender: skeaker_id gender
	- not really necessary, might be removed in a later release, can be set arbritary
- text: utterance_id text
	- any text, or the real transcription (not the target text)
- target: utterance_id index_to_target
	- refers to ./kaldi-adversarial/egs/wsj/s5/targets/target-utterances.txt (line - 1)
	- if you want a new target, add the new text, one per line, or modify this file


parameter description in run_mt.sh:
================================

- stage: start stage
- dataname: same name as the prepared folder in data/
- thresh: lambda value, -1 def. ines that no hearing trehsholds are used
- itr: number of max iterations. $itr * 100 defines backpropabgation steps
- numjobs: max the number of speakers in the $dataname data
- split: max the number of speakers in the $dataname data