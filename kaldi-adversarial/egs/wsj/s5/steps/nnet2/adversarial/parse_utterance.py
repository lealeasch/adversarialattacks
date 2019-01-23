import numpy as np
import json


def search_word(line, word):
    if word.lower().strip() in line.lower():
        line = str.split(line)
        # search case insensitive
        if line[0].lower().strip() == word.lower().strip():
            return line


def phone2hmm(word, num_states):
    sequence = []
    for i in range(0,num_states):
        sequence.append([word, i])
    return sequence

# convert to json format
def toJson(utterance, utt_silc, utt_cosilc, data):

    data['utterance'].append({
        'transcription': utterance,
        'sequence-sil': utt_silc.tolist(),
        'sequence-no-sil': utt_cosilc.tolist()
    })

    return data

def append_sequence(phone_list, sequence):
    for elem in sequence:
        phone_list.append(elem)

    return phone_list


#root_dir = "/media/lea/Daten/Scibo/Projects/asr_hidden_voice_commands.git/kaldi/egs/wsj/s5/"
root_dir = "./"
# dir with all phones (and corresponding int value)
phone_dir = root_dir + "data/lang/phones.txt"
# dir with all dictionary (word to phones)
word_dir = root_dir + "data/lang/phones/align_lexicon.txt"
# mapping phones to int (word to phones as int value)
word_phone_int_dir = root_dir + "data/lang/phones/align_lexicon.int"
# all silence phones (different number of HMM states)
sil_phones_dir = root_dir + "data/lang/phones/silence.txt"
# list of target utterances
utterance_dir = root_dir + "targets/target-utterances.txt"
# dir for json files
json_dir = root_dir + "targets/"
# index of sil phones
sil_dir = root_dir + "targets/sil.txt"

utterances = []
with open(utterance_dir) as f:
    for utterance in f:
        utterance = utterance.lower()
       # utterance = utterance.replace('.', '')
        utterance = utterance.replace(',', '')
        utterance = utterance.replace('#', '')
        utterances.append(str.split(utterance))
f.close()

f = open(word_phone_int_dir)
dictionary = f.readlines()
f.close()

# parse silence phones to get int
num_phones = 0
sil_phones = []
f = open(sil_phones_dir)
for phone in f:
    with open(phone_dir) as p:
        for line in p:
            line = str.split(line)
            if phone.strip().lower() == line[0].strip().lower():
                sil_phones.append(int(line[1]))
            if int(line[1]) > num_phones:
                num_phones = int(line[1])
f.close()

with open(sil_dir, 'w') as f:
    f.write(str(num_phones) + "\n")
    f.write(str(sil_phones))




dict = {}
f = open(word_dir)
# in case several pronunciations of one word exist, any kind is good (here it will be the last in list)
for line_cnt,line in enumerate(f):
    # iterate through all words in all utterances
    for utterance in utterances:
        for word in utterance:
            line_with_word = search_word(line, word)
            if line_with_word:
                dict[word] = str.split(dictionary[line_cnt])[2::]
                print('{}: {}'.format(word,dict[word]))
f.close()



sil_phone = 1
data = {}
data['utterance'] = []
# iterate through all words in all utterances
with open(json_dir + "target-sequence.txt", 'w') as f:
    for u,utterance in enumerate(utterances):
        # with silence
        utt_silc  = []
        sequence = phone2hmm([int(sil_phone), int(sil_phone), int(dict[utterance[0]][0])], 5)
        append_sequence(utt_silc, sequence)

        # no silence
        utt_cosilc = []
        sequence = phone2hmm([int(sil_phone), int(sil_phone), int(dict[utterance[0]][0])], 5)
        append_sequence(utt_cosilc, sequence)

        for w,word in enumerate(utterance):
            for p,phone in enumerate(dict[word]):
                prev_phone_silc = dict[word][p-1] if p > 0 else sil_phone
                next_phone_silc = dict[word][p+1] if p < len(dict[word])-1 else sil_phone

                if p > 0:
                    prev_phone_cosilc = dict[word][p - 1]
                elif w > 0:
                    prev_phone_cosilc = dict[utterance[w - 1]][-1]
                else:
                    prev_phone_cosilc = sil_phone

                if p < len(dict[word])-1:
                    next_phone_cosilc = dict[word][p + 1]
                elif w < len(utterance)-1:
                    next_phone_cosilc = dict[utterance[w + 1]][0]
                else:
                    next_phone_cosilc = sil_phone

                # silence phones have different number of hmm states
                if phone not in sil_phones:
                    sequence = phone2hmm([int(prev_phone_silc), int(phone), int(next_phone_silc)], 4)
                    append_sequence(utt_silc, sequence)

                    sequence = phone2hmm([int(prev_phone_cosilc), int(phone), int(next_phone_cosilc)], 4)
                    append_sequence(utt_cosilc, sequence)
                else:
                    sequence = phone2hmm([int(prev_phone_silc), int(phone), int(next_phone_silc)], 5)
                    append_sequence(utt_silc, sequence)

                    sequence = phone2hmm([int(next_phone_cosilc), int(phone), int(next_phone_cosilc)], 5)
                    append_sequence(utt_cosilc, sequence)

            next_phone = dict[utterance[w + 1]][0] if w < len(utterance) - 1 else sil_phone

            sequence = phone2hmm([int(dict[word][p]), int(sil_phone), int(next_phone)], 5)
            append_sequence(utt_silc, sequence)


        sequence = phone2hmm([int(sil_phone), int(sil_phone), int(sil_phone)], 5)
        append_sequence(utt_silc, sequence)

        sequence = phone2hmm([int(dict[utterance[-1]][-1]), int(sil_phone), int(sil_phone)], 5)
        append_sequence(utt_cosilc, sequence)

        f.write("transcription\n{}\nsilence\n".format(utterance))

        for elem in utt_silc:
            f.write("{}\n".format(elem))

        f.write("no silence\n")

        for elem in utt_cosilc:
            f.write("{}\n".format(elem))
