/*
Copyright 2011 Erik Gorset. All rights reserved. (available at https://github.com/gorset/radix)

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

      THIS SOFTWARE IS PROVIDED BY Erik Gorset ``AS IS'' AND ANY EXPRESS OR IMPLIED
      WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
      FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Erik Gorset OR
      CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
      CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
      SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
      ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
      NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
      ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

      The views and conclusions contained in the software and documentation are those of the
      authors and should not be interpreted as representing official policies, either expressed
      or implied, of Erik Gorset.
*/

#ifndef RADIX_SORT_H_
#define RADIX_SORT_H_

#include <iostream>
#include <algorithm>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define ROUND_DOWN(x, s) ((x) & ~((s)-1))
#define MIN_BUCKET_SIZE 16

template<class T, class KeyExtractor>
inline void insertion_sort(T* array, const int offset, const int end, const KeyExtractor& key) {
    int x, y;
    T temp;
    for (x=offset; x<end; ++x) {
        for (y=x; y>offset && key(array[y-1])>key(array[y]); y--) {
            temp = array[y];
            array[y] = array[y-1];
            array[y-1] = temp;
        }
    }
}


template<class T, class KeyExtractor>
inline void radix_sort(T* array, const int offset, const int end, const int shift, const KeyExtractor& key) {
    int x, y, tmp;
    T value, temp;
    unsigned int last[256] = { 0 }, pointer[256];

    for (x=offset; x < end; ++x) {
        ++last[(key(array[x]) >> shift) & 0xFF];
    }

    last[0] += offset;
    pointer[0] = offset;
    for (x=1; x<256; ++x) {
        pointer[x] = last[x-1];
        last[x] += last[x-1];
    }

    for (x=0; x<256; ++x) {
        while (pointer[x] != last[x]) {
            T value = array[pointer[x]];
            y = (key(value) >> shift) & 0xFF;
            while (x != y) {
                temp = array[pointer[y]];
                array[pointer[y]++] = value;
                value = temp;
                y = (key(value) >> shift) & 0xFF;
            }
            array[pointer[x]++] = value;
        }
    }

    if (shift > 0) {
        for (x=0; x<256; ++x) {
            tmp = x > 0 ? pointer[x] - pointer[x-1] : pointer[0] - offset;
            if (tmp > MIN_BUCKET_SIZE) {
                radix_sort(array, pointer[x] - tmp, pointer[x], shift-8, key);
            } else if (tmp > 1) {
                insertion_sort(array, pointer[x] - tmp, pointer[x], key);
            }
        }
    }
}


template<class T, class KeyExtractor>
void radix_sort(T* array, const int size, KeyExtractor& key) {
    radix_sort(array, 0, size, 24, key);
}

template<class RandomAccessIterator, class KeyExtractor>
void radix_sort(RandomAccessIterator begin, const int size, KeyExtractor& key) {
    radix_sort(begin, 0, size, 24, key);
}

#endif