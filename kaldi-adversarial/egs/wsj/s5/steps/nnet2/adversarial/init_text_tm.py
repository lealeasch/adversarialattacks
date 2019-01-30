import os
import sys
from fnmatch import fnmatch

def main():

    if len(sys.argv) < 3:
        print("Wrong or not enough arguments")
        sys.exit()

    data_name = sys.argv[1]
    thresh=int(sys.argv[2])
    # num utterance
    print("defined for experiments: " + data_name)

    root_dir = "./"
    # data dir
    data_dir = root_dir + "data/"
    # spoofed text
    #spoofed_text_dir = data_dir + "spoof_eval92/"
    spoofed_text_dir = data_dir + "adversarial_" + data_name + "/"
    target_text_dir = data_dir + data_name + "/target"
    # original text
    #original_text_dir = data_dir + "test_eval92/"
    original_text_dir = data_dir + data_name + "/"
    # target dir
    target_utt = root_dir + "targets/target-utterances.txt"
    # adversarial wav
    adversarial_wav_dir = "adversarial-wav/" + data_name + "_" + str(thresh) + "dB"

    n_target = {}
    with open(target_text_dir) as read_file:
        for line in read_file:
            line = line.split()
            n_target[line[0]] = int(line[1])

    all_target = []
    with open(target_utt) as f:
        for line in f:

            utterance = line.upper()
            #utterance = utterance.replace('.', '')
            utterance = utterance.replace(',', '')
            utterance = utterance.replace('#', '')

            all_target.append(utterance)



    pattern = "text"
    for path,subdirs, files in os.walk(original_text_dir):
        for name in files:
            if fnmatch(name, pattern):
                text_dir = os.path.join(path, name)

                spoof_text_dir = text_dir.replace(original_text_dir, spoofed_text_dir)

                print("{} --> {}".format(text_dir, spoof_text_dir))

                if not os.path.exists(spoof_text_dir[:-5]):
                    os.makedirs(spoof_text_dir[:-5])

                with open(spoof_text_dir, "w") as write_file:
                    with open(text_dir) as read_file:
                        for line in read_file:
                            line = line.split()
                            write_file.write("{} {}".format(line[0], all_target[n_target[line[0]]]))

                with open(text_dir, 'w') as write_file:
                    with open(spoof_text_dir) as read_file:
                        for line in read_file:
                            write_file.write(line)




    pattern = "wav.scp"
    for path,subdirs, files in os.walk(original_text_dir):
        for name in files:
            if fnmatch(name, pattern):
                text_dir = os.path.join(path, name)

                spoof_text_dir = text_dir.replace(original_text_dir, spoofed_text_dir)

                print("{} --> {}".format(text_dir, spoof_text_dir))

                if not os.path.exists(spoof_text_dir[:-5]):
                    os.makedirs(spoof_text_dir[:-5])

                with open(spoof_text_dir, "w") as write_file:
                    with open(text_dir) as read_file:
                        for line in read_file:
                            line = line.split()
                            write_file.write("{} {}/{}.wav\n".format(line[0], adversarial_wav_dir, line[0]))



if __name__ == "__main__":
    main()