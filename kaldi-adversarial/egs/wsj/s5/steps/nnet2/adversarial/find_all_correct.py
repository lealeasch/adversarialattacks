import sys
import os

def main():

    if len(sys.argv) < 4:
        print("Wrong or not enough arguments")
        sys.exit()

    data_name = sys.argv[1]
    itr = int(sys.argv[2])
    dir_name = sys.argv[3]

    # num utterance
    print("defined for experiments: " + data_name)
    print("defined for model: " + dir_name)

    root_dir = "./"
    # result dir
    result_dir = root_dir + "exp/" + dir_name +"/adversarial_" + data_name + "/scoring_kaldi/wer_details/per_utt"
    itr_dir = root_dir + "exp/" + dir_name + "/adversarial_" + data_name + "/scoring_kaldi/wer_details/utt_itr"

    utt = []
    if os.path.isfile(itr_dir):
        with open(itr_dir) as f:
            for line in f:
                line = line.split()
                utt.append(line[0])


    with open(itr_dir, "a") as write_f:

        with open(result_dir) as f:
            for line in f:
                line = line.split()

                if line[1] == "#csid":
                    if int(line[3]) == 0 and int(line[4]) == 0 and int(line[5]) == 0:
                        if not line[0] in utt:
                            write_f.write("{} {}\n".format(line[0], str(itr)))


if __name__ == "__main__":
    main()