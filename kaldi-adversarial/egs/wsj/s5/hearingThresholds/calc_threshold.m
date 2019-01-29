function remapped_heating_thr = calc_threshold(wav_files, win_len, overlap, destination)

%add_pre = '/media/kaldi-monster';
add_pre = '';

% destination = strcat(add_pre, '/home/lea/github/adversarialattacks/kaldi-adversarial/egs/wsj/s5/exp/nnet5d_gpu_time/spoof_test_dev93_mt_9/thresholds/');
% wav_files = strcat(add_pre, '/home/lea/github/adversarialattacks/kaldi-adversarial/egs/wsj/s5/data/test_dev93_mt_9/wav.scp');
% win_len = 512;
% overlap = 256;

addpath('MPEG1')

utt = {};
id = {};
fid = fopen(wav_files);
tline = fgetl(fid);
while ischar(tline)
    C = strsplit(tline,' ');
    id{end+1} = C{1};
    utt{end+1} = C{2};
    tline = fgetl(fid);
end
fclose(fid);

for uu = 1:length(utt)
    
    wav_file = utt{uu};
    
    [audio, fs] = audioread(strcat(add_pre, wav_file));
    [remapped_heating_thr, remapped_heating_thr_dB] = calculate_hearing_threshold(audio, fs, win_len, overlap);
      
    % repeat first and last frame for padding
    remapped_heating_thr = [repmat(remapped_heating_thr(1,:),4,1); remapped_heating_thr; repmat(remapped_heating_thr(end,:),4,1)];
    remapped_heating_thr_dB = [repmat(remapped_heating_thr_dB(1,:),4,1); remapped_heating_thr_dB; repmat(remapped_heating_thr_dB(end,:),4,1)];

    % copy matrix, for real an imaginary part
    remapped_heating_thr = repmat(remapped_heating_thr,1,2);
    remapped_heating_thr_dB = repmat(remapped_heating_thr_dB,1,2);
    
    csvwrite(strcat(destination, id{uu}, '.csv'), remapped_heating_thr);
    csvwrite(strcat(destination, id{uu},'_dB.csv'), remapped_heating_thr_dB);
end

exit 
