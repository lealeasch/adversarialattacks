function [remapped_heating_thr, remapped_heating_thr_dB] = calculate_hearing_threshold(audio, fs, win_len, overlap)

% audio: raw audio
% fs:   original sampling frequency
% win_len: window length
% overlap: overlap between two windows

%addpath('MPEG1')
% resample (thresholds can only be calculated for 44100 Hz)
[P,Q] = rat(44100/fs);
xnew = resample(audio,P,Q);

%% step 1: calculate raw hearing thresholds
[LTmin_all, LTq] = raw_thresholds(xnew);

%% step 2: remap hearing thresholds
% calculate number of frames
num_frames = floor(length(audio)/(win_len-overlap));
% mapps hearing thresholds to original sampling frequency and frame
% length
[remapped_heating_thr, remapped_heating_thr_dB] = remap_hearing_thresholds(LTmin_all, num_frames, fs, win_len, LTq);
