import os.path
import csv
import numpy as np
import sys


class Silence:
    sil = 0
    nil = 1
    sil_phones = []

    def __init__(self, dir):
        with open(dir) as f:
            for line in f:
                line = line.split(",")
                self.sil_phones = []
                for elem in line:
                    if elem.isdigit():
                        self.sil_phones.append(int(elem))



class Utterance:
    transcription = ""
    sil_seq = []
    nil_seq = []
    num_phones_btw_sil = []
    sil_cl = [] #Silence("")

    def __init__(self, transcription, sil_seq, nil_seq, dir):
        self.transcription = transcription
        self.sil_seq = sil_seq
        self.nil_seq = nil_seq
        self.sil_cl = Silence(dir)
        self.find_num_phones()

    def find_num_phones(self):
        self.num_phones_btw_sil = []
        cnt = 0
        for elem in self.sil_seq[1:]:
            if elem[4] not in self.sil_cl.sil_phones and elem[3] == 0:
                cnt += 1
            elif elem[4] in self.sil_cl.sil_phones and elem[3] == 0:
                self.num_phones_btw_sil.append(cnt)
                cnt = 0
        self.num_phones_btw_sil = self.num_phones_btw_sil[:-1]


class SilAnalysis:
    num_sil = 0
    sil_nil_list = []

    def __init__(self, num_sil, sil_nil_list):
        self.num_sil = num_sil
        self.sil_nil_list = sil_nil_list


def string_to_int(line):
    line = line.split(",")
    tmp = []
    for elem in line:
        tmp.append(int(elem))

    return tmp


# reads prepared target sequences
def read_utterance(dir, sil_dir):

    with open(dir) as f:
        line = f.readline().strip()
        utt = []
        while 1:
            if not line: break  # stop the loop if no line was read
            sil_seq = []
            nosil_seq = []
            if line != "transcription": break
            transcription = f.readline().strip().split(",")
            print(transcription)
            f.readline()
            while 1:
                line = f.readline().strip()
                if line == "no silence": break
                if not line:
                    print("ERROR: 'no silent' sequence is missing")
                    break
                sil_seq.append(string_to_int(line))
            while 1:
                line = f.readline().strip()
                if line == "transcription": break
                if not line: break
                nosil_seq.append(string_to_int(line))

            utt.append(Utterance(transcription, sil_seq, nosil_seq, sil_dir))
        return utt


def read_orignal_post(post_dir, filenames):
    max_original_all = []
    post_original_all = []
    for u in filenames:
        original_dir = post_dir + u + "/original.csv"
        print(u)
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

            max_original_all.append(max_original)
            post_original_all.append(post_original)

    return max_original_all, post_original_all



# reads original posteriograms: OLD VERSION
def read_orignal_from_post_old(dir):
    max_original_all = []
    filenames = []
    for u in os.listdir(dir):
        original_dir = dir + u + "/original.csv"
        if os.path.isfile(original_dir):
            print(u)
            filenames.append(u)
            with open(original_dir, 'r') as f:
                reader = csv.reader(f)
                original = np.asarray(list(reader))

                max_original = []
                for row in original:
                    tmp = []
                    for element in row:
                        if len(element) > 0:
                            tmp.append(float(element))

                    max_original.append(tmp.index(max(tmp)))
            max_original_all.append(max_original)
    return max_original_all, filenames


# reads original posteriograms
def read_orignal_from_post(dir):
    path_all = []
    filenames = []
    for u in os.listdir(dir):
        if u.endswith('.csv'):
            print(u.split(".")[0])
            filenames.append(u.split(".")[0])
            
            with open(dir + u, 'r') as f:
                original = f.readline().strip().split(",")

                path = []
                for elem in original:
                    if elem.isdigit():
                        path.append(int(elem))

                path_all.append(path)

    return path_all, filenames


# 1D  maximum filter of 'array' with filter kernel size 'size'
def max_filter(array, size):

    if size % 2 == 0:
        print("ERROR: Please choose an odd filter kernel size")
        exit()

    context = (size-1)/2

    filter_array = []
    for i,elem in enumerate(array):
        filter_array.append(max(array[max([i-context, 0]):min([i+context+1, len(array)])]))

    return filter_array


# find number of sil segments
def find_sil_segment(utterances, sil_cl):

    sil = sil_cl.sil
    nil = sil_cl.nil

    sil_des = []
    for utt in utterances:
        prev_frame = []
        prev_frame.append([sil, 0])
        #filter_utt = max_filter(utt, 5)
        for i,elem in enumerate(utt):
            if elem == sil and prev_frame[-1][0] == nil:
                prev_frame.append([sil, i])
            elif elem == nil and prev_frame[-1][0] == sil:
                prev_frame.append([nil,i])

        l = [row[0] for row in prev_frame]
        num_sil = l.count(sil)
        sil_des.append(SilAnalysis(num_sil, prev_frame))

    return sil_des


# original poseriograms to bin values (sil, non sil)
def val2bin(original, sil_cl):
    sil_phones = sil_cl.sil_phones
    sil = sil_cl.sil
    nil = sil_cl.nil

    bin = []
    for row in original:
        tmp = []
        for element in row:
            if element in sil_phones:
                tmp.append(sil)
            else:
                tmp.append(nil)

        bin.append(tmp)

    return bin


def get_num_words_per_nil_segment(utt, t, sil_cl):

    nil = sil_cl.nil
    length_cnt = []
    for seg_cnt, seg in enumerate(utt.sil_nil_list):
        # TODO: no silence at end
        if seg[0] == nil:  # and seg_cnt != 0:# and seg_cnt != len(utt.sil_nil_list) - 1:
            length_cnt.append(utt.sil_nil_list[seg_cnt + 1][1] - seg[1])

    sum_l = sum(length_cnt)
    length_cnt_norm = []
    for elem in length_cnt:
        length_cnt_norm.append(float(elem) / float(sum_l))

    sum_phones = sum(t.num_phones_btw_sil)

    rough_num = []
    for l_norm in length_cnt_norm:
        rough_num.append(sum_phones * l_norm)

    num_word_list = []
    num_phones_list = []
    num_phones = 0
    itr = 0
    num_words = 0
    for phones in t.num_phones_btw_sil:
        if itr >= len(rough_num):
            break
        num_phones += phones
        num_words += 1
        if num_phones > rough_num[itr]:
            if num_words < 2 or itr == len(rough_num) - 1:
                num_word_list.append(num_words)
                num_phones_list.append(num_phones)
                num_words = 0
                num_phones = 0
                itr += 1
            else:
                num_word_list.append(num_words - 1)
                num_phones_list.append(num_phones - phones)
                num_words = 1
                num_phones = phones
                itr += 1

    if sum(num_word_list) != len(t.num_phones_btw_sil):
        num_word_list[-1] = len(t.num_phones_btw_sil) - sum(num_word_list[:-1])

    if len(num_word_list) == len(rough_num) - 1:
        # in case for last nil segment no word is left
        # TODO: in case num_word_list[-1] is too short
        if num_words == 0:
            num_word_list[-1] -= 1
            num_words += 1
            if num_word_list[-1] < 1:
                print("TODO: in case num_word_list[-1] is too short")

        num_word_list.append(num_words)

    return num_word_list


# init target with sil segments
def init_target(t, sil_desc, sil_cl, sil_hmm_states, dataset):
    targets = []
    num_word_list = []
    for utt in sil_desc:
        s = utt.num_sil
        if s - 2 > len(t.transcription) - 1:
            # TODO: what to do if number of words is less than number of silence segment?
            # TODO: no sil at beginning or end
            print("Problem: {} {}".format(t.transcription, s - 2))
            targets.append([0])
        elif s == 1:
            # TODO: no sil at beginning or end
            print("TODO: no sil at beginning or end")
            targets.append([0])
        elif s == 0:
            # TODO: no sil
            print("TODO: no sil")
            targets.append([0])
        elif s == 2:
            init_utt = []
            for i,elem in enumerate(t.nil_seq):
                init_utt.append(t.nil_seq[i][4])
            targets.append(init_utt)
        else:
            num_word_list = get_num_words_per_nil_segment(utt, t, sil_cl)

            word_iter = iter(num_word_list)
            num_word = next(word_iter)
            track_idx = 0
            num_inserted_words = 0
            init_utt = []
            num_inserted_words_total = 0
            last_was_silent_flag = False
            for i, elem in enumerate(t.sil_seq):

                track_idx = i - num_inserted_words_total * sil_hmm_states

                if elem[3] == 0 and elem[4] in sil_cl.sil_phones and i != 0:
                    num_inserted_words += 1
                    num_inserted_words_total += 1
                    if num_inserted_words == num_word:
                        # replace last phone with the one from sil_seq
                        init_utt[-4] = t.sil_seq[i - 4][4]
                        init_utt[-3] = t.sil_seq[i - 3][4]
                        init_utt[-2] = t.sil_seq[i - 2][4]
                        init_utt[-1] = t.sil_seq[i - 1][4]
                        # add sil
                        for j in range(0, sil_hmm_states):
                            init_utt.append(t.sil_seq[i + j][4])
                        last_was_silent_flag = True
                        # next non-silent segment
                        try:
                            num_word = next(word_iter)
                        except StopIteration:
                            break
                        num_inserted_words = 0

                if last_was_silent_flag and elem[4] not in sil_cl.sil_phones and dataset != "yesno": # on case an silent was inserted
                    init_utt.append(t.sil_seq[i][4])
                    if t.sil_seq[i][3] == 3:
                        last_was_silent_flag = False
                elif elem[4] not in sil_cl.sil_phones or i < sil_hmm_states:
                    init_utt.append(t.nil_seq[track_idx][4])

            targets.append(init_utt)

    return targets



def init_target_max_seg(utterances, target, sil_desc, filenames):

    targets = []
    for i,utt in enumerate(sil_desc):
        t = utterances[target[filenames[i]]]
        print("{} {}".format(target[filenames[i]], filenames[i]))
        init_utt = []
        for elem in t.nil_seq:
            init_utt.append(elem[4])

        targets.append(init_utt)

    return targets



# init target with sil segments
def init_target_old(t, num_sil, sil_hmm_states = 5):

    targets = []
    for utt in num_sil:
        s = utt.num_sil
        if s-2 > len(t.transcription) - 1:
             # TODO: what to do if number of words is less than number of silence segment?
             # TODO: no sil at beginning or end
            print("Problem: {} {}".format(t.transcription,s-2))
            targets.append([0])
        elif s == 1:
            init_utt = []
            for i in range(0, len(t.nil_seq)- 5):
                init_utt.append(t.nil_seq[i][4])
            targets.append(init_utt)
        elif s == 2:
            init_utt = []
            for i,elem in enumerate(t.nil_seq):
                init_utt.append(t.nil_seq[i][4])
            targets.append(init_utt)
        else:
            track_idx = 0
            num_inserted_sil = 0
            init_utt = []
            for i,elem in enumerate(t.sil_seq):
                init_utt.append(elem[4])
                if elem[3] == sil_hmm_states - 1:
                    num_inserted_sil += 1
                    track_idx = i - num_inserted_sil*sil_hmm_states
                if num_inserted_sil == s-1: # insert last phone after last silence
                    init_utt.append(t.sil_seq[i + 1][4])
                    init_utt.append(t.sil_seq[i + 2][4])
                    init_utt.append(t.sil_seq[i + 3][4])
                    init_utt.append(t.sil_seq[i + 4][4])
                    break

            for i in range(track_idx+2, len(t.nil_seq)):
                init_utt.append(t.nil_seq[i][4])

            targets.append(init_utt)

    return targets


def mapping(long_seq, short_seq):

    long_segments = []
    start_idx = long_seq[0]
    for i,elem in enumerate(long_seq):
        if elem != long_seq[i-1]:
            long_segments.append([start_idx, i-1, long_seq[i-1]])
            start_idx = i

    long_segments.append([start_idx, len(long_seq) - 1, long_seq[-1]])

    short_segments = []
    start_idx = short_seq[0]
    for i, elem in enumerate(short_seq):
        if elem != short_seq[i-1]:
            short_segments.append([start_idx, i-1, short_seq[i-1]])
            start_idx = i

    short_segments.append([start_idx, len(short_seq) - 1, short_seq[-1]])

    if len(long_segments) != len(short_segments):
        print("ERROR: Wrong init of target sequence. inconsistent number")
        return [0, 0]

    mapped = []
    for i, elem in enumerate(long_segments):
        lng = long_segments[i]
        sht = short_segments[i]
        if lng[2] != sht[2]:
            print("ERROR: Wrong init of target sequence. Wrong segment")
            return [0, 0]

        y = []
        for x in range(lng[0], lng[1] + 1):
            y.append([x, ((x - lng[0])*(sht[1] - sht[0] + 1))/(lng[1] - lng[0] + 1) + sht[0]])

        mapped = sum([mapped, y], [])

    return mapped


def read_max_seg(tar_dir, filenames):

    max_seg_list = []
    for file in filenames:
        with open(tar_dir + "thresholds/" + file + "_max-seg.csv") as f:
            reader = csv.reader(f)
            max_seg_string = list(reader)
            max_seg = []
            for elem in max_seg_string[0]:
                max_seg.append(int(elem))

            max_seg_list.append(max_seg)

    return max_seg_list




def main():

    if len(sys.argv) < 4:
        print("Wrong or not enough arguments")
        sys.exit()

    data_name = sys.argv[1]
    dir_name = sys.argv[2]
    len_post = int(sys.argv[3]) # 3447

    # num utterance
    print("defined for experiments: " + data_name)
    print("defined for model: " + dir_name)


    root_dir = "./"
    # file with triphone sequence
    seq_dir = root_dir + "targets/target-post-sequence.txt"
    # adversarial utterance
    utt_dir = root_dir + "exp/"+ dir_name + "/adversarial_" + data_name +  "/utterances/"
    # target dir
    tar_dir = root_dir + "exp/"+ dir_name + "/adversarial_" + data_name + "/utterances/"


    # data dir
    data_dir = root_dir + "data/"
    # adversarial text
    adversarial_text_dir = data_dir + data_name + "/target"

    # sil phones
    sil_dir = root_dir + "targets/sil-post.txt"

    # contains silence phones
    # TODO: remove, is in 'utterances'
    sil_cl = Silence(sil_dir)

    print("Parse target utterances...")
    utterances = read_utterance(seq_dir, sil_dir)
    print("Finished parsing\n\n")

    print("Read in adversarial utterances orignal posteriograms...")
    original, filenames = read_orignal_from_post(utt_dir)
    print("Finished read")

    original_bin = val2bin(original, sil_cl)

    # returns number of silence segments in original utterance
    sil_desc = find_sil_segment(original_bin, sil_cl)

    n_target = {}
    #n_target = []
    with open(adversarial_text_dir) as read_file:
        for line in read_file:
            line = line.split()
            print("{} {}".format(line[0], int(line[1])))
            n_target[line[0]] = int(line[1])
            #n_target.append(int(line[1]))

    # init targets with silence segments
    targets = init_target_max_seg(utterances, n_target, sil_desc, filenames)
    # TODO: first entry is enough
    #targets = targets[0][1:]

    maxseg = [0] * len(filenames)

    for u,file in enumerate(filenames):
        print(u,file)

        target_sequence = []
        for i, elem in enumerate(original[u]):
            target_sequence.append(targets[u][(i*len(targets[u]))/len(original[u])])

        target_posteriogram = []
        for i,post in enumerate(target_sequence):
            tmp = [0]*len_post
            tmp[post] = 1
            target_posteriogram.append(tmp)


        if not os.path.exists(tar_dir + file):
            os.makedirs(tar_dir + file)
        np.savetxt(tar_dir + file + '/target.csv', target_posteriogram, delimiter=',')


if __name__ == "__main__":
    main()