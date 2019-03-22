function [LTmin_all, LTq] = synthesize(dataset, adversarial_dir, datadir, fs, win_len)

overlap = 0;

dest_dir = strcat('../../../', dataset);

cf.nfft = win_len;
cf.overlap = overlap;

utt = {};
audiofile = {};
fid = fopen(datadir);
tline = fgetl(fid);
while ischar(tline)
    C = strsplit(tline,' ');
    utt{end+1} = C{1};
    audiofile{end+1} = C{2};
    tline = fgetl(fid);
end
fclose(fid);


mkdir(fullfile(dest_dir))
for uu = 1:length(utt)
    
    [x,fs_o] = audioread(strcat('../../../', audiofile{uu}));
    
    % resample?
    if fs_o ~= fs
        x = x(1:round(fs/fs_o):end);
    end
    
    try
        utterance = load(strcat(adversarial_dir, utt{uu}, '/adversarial.csv'));
    catch
        continue
    end
    
    % fix scaling due to value convertions
    y = iwin(utterance',cf)*3.051757812500000e-05;
    y = [y; x(length(y)+1:end)];
    
    %mkdir(fullfile(dest_dir, utt{uu}))
    audiowrite(fullfile(dest_dir, strcat(utt{uu},'.wav')),y, fs);
end

exit
