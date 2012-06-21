#!/bin/bash

tr ' ' '\n' < $1 | tr -d '"' > lowercase_words

cat lowercase_words | tail -n +2 > lowercase_next_words
cat lowercase_words | tail -n +3 > lowercase_next_next_words
cat lowercase_words | tail -n +4 > lowercase_next_next_next_words

paste lowercase_words lowercase_next_words | sort | uniq -c | sort -nr | head -n 500 | tr '\t' ' ' > lowercase_bigrams_freq_top_500.txt
paste lowercase_words lowercase_next_words lowercase_next_next_words | sort | uniq -c | sort -nr | head -n 500 | tr '\t' ' ' > lowercase_trigrams_freq_top_500.txt
paste lowercase_words lowercase_next_words lowercase_next_next_words lowercase_next_next_next_words | sort | uniq -c | sort -nr | head -n 500 | tr '\t' ' ' > lowercase_quadgrams_freq_top_500.txt

cat lowercase_words | sort | uniq -c | sort -nr | head -n 500 | tr '\t' ' ' > lowercase_word_freq_top_500.txt


# tr ' ' '\n' < $1 > camelcase_words
# cat camelcase_words | tail -n +2 > camelcase_next_words
# 
# cat camelcase_words | sort | uniq -c | sort -nr | head -n 500 > camelcase_word_freq_top_500
# 
# 
# paste camelcase_words camelcase_next_words | sed -E '/^[A-Z]+[a-z]+.*[ \t]+[A-Z]+[a-z]+.*$/!d' | sort | uniq -c | sort -nr | head -n 500 > camelcase_proper_noun_bigrams_top_500
# 
