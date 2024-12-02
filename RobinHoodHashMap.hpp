#pragma once

#include <optional>
#include <iostream>

template <typename K, typename V>
struct RobinHoodBucket
{
    K key;
    V value;
    bool filled = false;
    size_t hash;
    size_t psl;
};

inline size_t approx85Percent(size_t x)
{
    return (x * 870) >> 10;
}

inline size_t approx40Percent(size_t x)
{
    return (x * 409) >> 10;
}

template <typename K, typename V, typename H>
struct RobinHoodHashMap
{
    using BucketType = RobinHoodBucket<K, V>;
    size_t count = 0;

    BucketType *buckets = nullptr;
    size_t bucketSize = 0;
    size_t minBucketSize = 524288;

    H computeHash;

    RobinHoodHashMap(H hasher) : computeHash(hasher)
    {
        resize(minBucketSize);
    }

    std::optional<V> get(const K& key)
    {
        const uint64_t hash = computeHash(key);

        // std::cout << hash << std::endl;
        size_t currentPsl = 0, i = hash % bucketSize;

        BucketType *bucket;
         __builtin_prefetch(&buckets[i], 0, 0);
        while (true)
        {
            bucket = &buckets[i];

            if (!bucket->filled || currentPsl > bucket->psl)
                return std::nullopt;

            if (bucket->hash == hash && bucket->key == key)
            {
                return bucket->value;
            }

            currentPsl++;

            i = (i + 1) % bucketSize;
            __builtin_prefetch(&buckets[i], 0, 0);
        }
    }

    void internalInsert(const K &key, const V &value)
    {
        const uint64_t hash = computeHash(key);
        // std::cout << hash << std::endl;

        BucketType current;
        current.key = key;
        current.filled = true;
        current.hash = hash;
        current.value = value;
        current.psl = 0;

        size_t i = hash % bucketSize;
        BucketType *bucket;
         __builtin_prefetch(&buckets[i], 0, 0);
        while (true)
        {
            bucket = &buckets[i];
            if (!bucket->filled)
                break;

            if (bucket->hash == hash && bucket->key == key)
            {
                bucket->value = value;
                return;
            }

            if (current.psl > bucket->psl)
            {
                // Optimasi tidak terlalu berguna
                std::swap(bucket->key, current.key);
                std::swap(bucket->value, current.value);
                std::swap(bucket->filled, current.filled);
                std::swap(bucket->hash, current.hash);
                std::swap(bucket->psl, current.psl);
            }
            current.psl++;

            i = (i + 1) % bucketSize;
             __builtin_prefetch(&buckets[i], 0, 0);
        }

        *bucket = current;
        count++;
    }

    void resize(size_t newSize)
    {
        std::cout << "Resize " << bucketSize << " " << newSize << std::endl;
        BucketType *oldBuckets = buckets;
        size_t oldSize = bucketSize;

        buckets = new BucketType[newSize];
        bucketSize = newSize;
        count = 0;

        for (size_t i = 0; i < oldSize; i++)
        {
            BucketType *bucket = &oldBuckets[i];

            if (!bucket->filled)
                continue;

            internalInsert(bucket->key, bucket->value);
        }

        if (oldBuckets && oldSize > 0)
        {
            delete[] oldBuckets;
        }
    }

    void put(const K& key, const V& value)
    {
        size_t threshold = approx85Percent(bucketSize);

        if (count >= threshold)
        {
            resize(bucketSize * 2);
        }

        internalInsert(key, value);
    }

    bool remove(const K& key) {
        const uint64_t hash = computeHash(key);

        size_t currentPsl = 0, i = hash % bucketSize;
        BucketType *bucket;

        while (true) {
            bucket = &buckets[i];
            if (!bucket->filled || currentPsl > bucket->psl) {
                return false;
            }

            if (bucket->hash != hash || bucket->key != key) {
                currentPsl++;
                i = (i + 1) % bucketSize;
                continue;
            }

            bucket->filled = false;
            count--;

            BucketType* nextBucket;
            while (true) {
                i = (i + 1) % bucketSize;
                nextBucket = &buckets[i];

                if (!nextBucket->filled || nextBucket->psl == 0) {
                    break;
                }

                nextBucket->psl--;
                *bucket = *nextBucket;
                bucket = nextBucket;
            }

            return true;
        }
    }
};