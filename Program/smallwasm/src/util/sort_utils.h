#pragma once

namespace shine::util {

// Small, STL-free quicksort with insertion sort for tiny partitions.
template<typename T, typename Less>
inline void sort_inplace(T* data, unsigned int count, Less less) {
    if (!data || count < 2u) return;

    const int kSmall = 12;

    struct Range { int l; int r; };
    Range stack[64];
    int top = 0;
    stack[top++] = {0, (int)count - 1};

    while (top > 0) {
        Range range = stack[--top];
        int left = range.l;
        int right = range.r;
        if (right - left <= kSmall) {
            for (int i = left + 1; i <= right; ++i) {
                T key = data[i];
                int j = i - 1;
                while (j >= left && less(key, data[j])) {
                    data[j + 1] = data[j];
                    --j;
                }
                data[j + 1] = key;
            }
            continue;
        }

        int mid = left + ((right - left) >> 1);
        T pivot = data[mid];
        int i = left;
        int j = right;
        while (i <= j) {
            while (less(data[i], pivot)) ++i;
            while (less(pivot, data[j])) --j;
            if (i <= j) {
                T tmp = data[i];
                data[i] = data[j];
                data[j] = tmp;
                ++i;
                --j;
            }
        }

        if (left < j) stack[top++] = {left, j};
        if (i < right) stack[top++] = {i, right};
    }
}

} // namespace shine::util
