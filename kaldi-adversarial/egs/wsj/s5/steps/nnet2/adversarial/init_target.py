import os.path
import numpy as np
import csv
import sys

class Align:
    utt = 0
    align = []

    def __init__(self, utt, align):
        self.utt = utt
        self.align = align


def read_forced_align(align_dir, num_jobs):

    align_list = []
    for job in range(1, num_jobs+1):

        aligns = align_dir + "ali." + str(job) + ".txt"

        with open(aligns) as f:
            for line in f:
                align = line.split()
                utt = align[0]
                align = align[1:]
                align_int = []
                for elem in align:
                    align_int.append(int(elem))

                align_list.append(Align(utt,align_int))

    return align_list


def read_original_post(post_dir, file):

    original_dir = post_dir + file + "/original.csv"
    with open(original_dir, 'r') as f:
        reader = csv.reader(f)
        original = np.asarray(list(reader))

        max_original = []
        post_original = []
        for row in original:
            tmp = []
            for elem in row:
                if elem != '':
                    tmp.append(float(elem))

            if len(tmp) > 0:
                max_original.append(tmp.index(max(tmp)))
                post_original.append(tmp)

    return max_original, post_original


# reads original posteriograms
def read_orignal_from_post(dir):
    path_all = {}
    filenames = []
    for u in os.listdir(dir):
        print(u.split(".")[0])
        filenames.append(u.split(".")[0])
        with open(dir + u, 'r') as f:
            original = f.readline().strip().split(",")

            path = []
            for elem in original:
                if elem.isdigit():
                    path.append(int(elem))

            path_all[filenames[-1]] =  path

    return path_all

def main():

    if len(sys.argv) < 5:
        print("Wrong or not enough arguments")
        sys.exit()

    data_name = sys.argv[1]
    num_jobs = int(sys.argv[2])
    dir_name = sys.argv[3]
    len_post = int(sys.argv[4])

    print("defined for experiments: " + data_name)
    print("defined for jobs: " + str(num_jobs))
    print("defined for model: " + dir_name)

    root_dir = "./"
    # init align dir
    # TODO: define as parameter
    align_dir = root_dir + "exp/"+ dir_name + "/adversarial_" + data_name + "/"
    # target dir
    tar_dir = align_dir + "utterances/"
    # adversarial utterances
    utt_dir = root_dir + "targets/utterances/"

    align_list = read_forced_align(align_dir, num_jobs)

    for al_num,align in enumerate(align_list):

        print("{}: {}".format(align.utt, al_num))
        target_posteriogram = []
        for elem in align.align:
            tmp = [0] * len_post
            if elem >= len_post:
                print("Wrong init: {}".format(elem))
                exit(0)
            tmp[elem] = 1
            target_posteriogram.append(tmp)

        file = align.utt
        if not os.path.exists(tar_dir + file):
            os.makedirs(tar_dir + file)
        np.savetxt(tar_dir + file + '/target.csv', target_posteriogram, delimiter=',')


if __name__ == "__main__":
    main()