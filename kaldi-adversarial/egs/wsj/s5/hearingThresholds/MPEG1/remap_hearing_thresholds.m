function [remapped_heating_thr, remapped_heating_thr_dB] = remap_hearing_thresholds(LTmin_all, num_frames, ft, win_len, LTq)

% LTmin_all:    raw hearing thresholds
% num_frames:   number of frames
% ft:           orignal sampling frequency
% win_len:      length of window


fs = 44100;

%% ----- non-linear frequency mapping (bark2frequency) ------ %%
[~, Map, ~, freq_mapping] = Table_absolute_threshold(1, fs, 128); % Threshold in quiet

resampled_LTmin = zeros(size(LTmin_all,1),fs/2);

start_idx = 1;
end_idx = floor(freq_mapping(Map(8),1));
resampled_LTmin(:,start_idx:end_idx) = repmat(LTmin_all(:,1), 1 , end_idx-start_idx+1);

j = 2;
% each critical band contains 8 frequencies (may be the same frequencies)
for i = 8:8:length(Map)-8
    start_idx = floor(freq_mapping(Map(i),1))+1;
    end_idx = floor(freq_mapping(Map(i+8),1));
    resampled_LTmin(:,start_idx:end_idx) = repmat(LTmin_all(:,j), 1 , end_idx-start_idx+1);
    j = j +1 ;
end

start_idx = floor(freq_mapping(Map(end),1))+1;
end_idx = 22050;
resampled_LTmin(:,start_idx:end_idx) = repmat(LTmin_all(:,end), 1 , end_idx-start_idx+1);


%% ----- resample frequency axis ------ %%
% fixed mapping indeces
idx_frames = linspace(1,ft/2, win_len/2);

resampled_LTmin = resampled_LTmin(:,round(idx_frames));


%% ----- resample time axis ------ %% 
% fixed mapping indeces
idx = linspace(1,size(resampled_LTmin,1), num_frames);

remapped_heating_thr_dB = resampled_LTmin(round(idx),:);

%% ----- normalize values ------ %%
normalized = remapped_heating_thr_dB - min(LTq);
remapped_heating_thr = normalized./ (max(max(LTmin_all)) - min(LTq));
     
