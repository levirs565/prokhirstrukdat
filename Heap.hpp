#pragma once

#include <algorithm>

/**
 * Struktur Data Heap
 * 
 */

template <typename T, typename K>
struct Heap
{
    size_t count = 0;
    K comparer;
    T *array = nullptr;
    size_t capacity = 0;

    ~Heap() {
        if (capacity != 0) delete array;
    }

    int left(int i)
    {
        return 2 * i + 1;
    }

    int right(int i)
    {
        return 2 * i + 2;
    }

    int parent(int i)
    {
        return (i - 1) / 2;
    }

    bool isEmpty()
    {
        return count == 0;
    }

    void resize(size_t newCapacity)
    {
        // cout << "Resize to " << size << endl;
        T *newArray = newCapacity == 0 ? nullptr : new T[newCapacity];

        if (capacity != 0 && newCapacity != 0)
        {
            std::copy(array, array + std::min(capacity, newCapacity), newArray);
        }

        if (capacity != 0)
            delete[] array;

        array = newArray;
        capacity = newCapacity;
    }

    void bubbleUp(int i)
    {
        int p = parent(i);
        while (i > 0 && comparer.compare(array[i], array[p]) < 0)
        {
            std::swap(array[i], array[p]);
            i = p;
            p = parent(i);
        }
    }

    void add(T&& value)
    {
        // cout << heapSize << " " << heapRealSize << endl;
        if (count + 1 > capacity)
            resize(std::max(count + 1, capacity * 2));

        // cout << heapSize << " " << heapRealSize << endl;
        array[count] = std::move(value);
        count++;
        bubbleUp(count - 1);
        // cout << "Test" << endl;
    }

    void trickleDown(int i)
    {
        while (i >= 0)
        {
            int j = -1;
            int l = left(i), r = right(i);

            if (r < count && comparer.compare(array[r], array[i]) < 0)
            {
                if (comparer.compare(array[l], array[r]) < 0)
                {
                    j = l;
                }
                else
                {
                    j = r;
                }
            }
            else if (l < count && comparer.compare(array[l], array[i]) < 0)
            {
                j = l;
            }

            if (j >= 0)
            {
                std::swap(array[i], array[j]);
            }

            i = j;
        }
    }

    T& getTop() {
        return array[0];
    }

    T removeTop()
    {
        T res = std::move(array[0]);
        array[0] = array[count - 1];
        count--;
        trickleDown(0);

        if (count < capacity / 2)
        {
            resize(std::max(capacity / 2, count));
        }

        return res;
    }

    void replaceTop(T&& value) {
        array[0] = std::move(value);
        trickleDown(0); 
    }
};