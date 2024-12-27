#pragma once

#include <iostream>

/*
    Implementasi Hash Table Open Adressing dengan variasi Linear Probing dan Robin Hood

    PSL (Probe Sequence Length) adalah jarak suatu elemen berada dengan letak seharusnya (indeks ideal) elemen tersebut terdasarkan hash
    Misalkan bucket berukuran 5
    Contoh 1. Suatu key, berdasarkan hashnya berada di posisi 2 tetapi karena probing maka ditaruh di posisi ke 4
    Maka PSL elemen tersebut adalah 2 karena dari 2 menuju 4 perlu 2 loncatan
    Contoh 2. Suatu key, berdasarkan hashnya berada di posisi 3 tetapi karena probing maka ditaruh di posisi ke 1
    Maka PSL elemen tersebut adalah 2 karena dari 3 loncat ke 4 kemudian dari 4 loncat ke 1
    Catatan: Perhitungan PSL dihitung berdasarkan loncatakan ke kanan dan posisi terakhir terhubung menuju posisi pertama

    Inti dari Robin Hood adalah: "Curi dari orang kaya kemudian berikan kepada orang miskin"
    Pada Robin Hood Hash Table, elemen dengan PSL tinggi disebut miskin dan PSL rendah disebut kaya
    Prinsip ini akan digunakan pada saat melakukan operasi hashmap
    
    Implementasi ini tidak menggunakan tombstone, sehingga pada saat erase/hapus akan menggeser posisi elemen
    Implementasi ini menggunakan array dinamis dengan ketentuan seperti berikut:
    - Saat load factornya diatas 85% maka array akan diresize menjadi 2 kali lipat
    - Saat load factornya di bawah 40% maka array akan diresize menjadi 0.5 kali lipat
*/

template <typename K, typename V>
struct RobinHoodBucket
{
    K key;
    V value;
    bool filled = false;
    size_t hash;
    size_t psl;
};

/**
 * Memperkirakan nilai 85% * x
 * 870 / (2 ^ 10) = 870 / 1024 = 0.849 (mendekati 85%)
 * 
 * Catatan: x >> 10 hasilnya sama dengan x / (2 ^ 10)
 * Tetapi cari dengan bitshift ini lebih cepat dan optimal
 */
inline size_t approx85Percent(size_t x)
{
    return (x * 870) >> 10;
}

/**
 * Memperkirakan nilai 40%
 * 409 / (2 ^ 10)= 409 / 1024 = 0.399 (mendekati 40%)
 */
inline size_t approx40Percent(size_t x)
{
    return (x * 409) >> 10;
}

/**
 * Hash Table dengan Linear Probing dan Robin Hood
 * K adalah tipe data kunci
 * V adalah tipe data nilai
 * H adalah tipe data fungsi yang akan menghasilkan hash
 * H harus berupa fungsi dengan signature uint64_t H(const K& key)
 */
template <typename K, typename V, typename H>
struct RobinHoodHashMap
{
    using BucketType = RobinHoodBucket<K, V>;
    size_t count = 0;

    BucketType *buckets = nullptr;
    size_t bucketSize = 0;
    size_t minBucketSize = 524288;

    H hasher;

    /**
     * Konstruktor untuk RobinHoodHashMap
     * hasher harus berupa fungsi yang menghasilkan hash dan harus bertipe H
     */
    RobinHoodHashMap()
    {
        resize(minBucketSize);
    }

    /**
     * Mendapatkan value berdasarkan kunci
     * Proses Algoritma:
     * 1. Cari hash dari kunci
     * 2. Isi i (indeks) dengan hash % ukuran bucket
     * 3. Isi currentPsl dengan 0, currentPsl berisi jarak i ke indeks seharusnya key diletakkan
     * 4. Dapatkan bucket pada indeks ke-i
     * 5. Jika bucket kosong maka key tidak ada.
     *    Alasannya karena implementasi ini tidak menerapkn tombstone.
     *    Agar sifat ini bisa terjaga maka pada saat erase/hapus harus ada langkah tambahan
     * 6. Jika currentPsl lebih dari psl dari bucket maka key tidak ada
     *    Alasanya adalah pada Robin Hood Hashmap, isi pada array bucket akan terurut berdasarkan hash % ukursan bucket
     *    Contohnya:
     *    Hash % ukuran bucket (-1 jika kosong): -1 -1 2 2 2 2 3 7
     *    PSL                  (-1 jika kosong): -1 -1 0 1 2 3 2 0
     *    currentPsl jika dimulai dari indeks 2: 6  7  0 1 2 3 4 5
     *    Ini berarti pada saat currentPsl lebih dari psl bucket maka terjadi perubahan nilai hash % ukuran bucket
     *    Ini menyebabkan key yang dituju tidak mungkin ada setelah terjadi kasus ini (contonya pada indeks 6)
     *    Untuk menjaga sifat ini maka ada algoritma tambahan pada saat insert
     * 7. Jika hash di bucket sama dengan hash key tujuan dan key bucket sama dengan key tujuan maka nilai ditemukan
     *    Algoritma selesai
     * 8. Jika tidak, maka tambah currentPsl dengan 1. 
     *    Ini karena i akan bertambah 1 sehingga jarak dari i ke jarak ideal bertambah 1.
     * 9. Ganti nilai i menjadi (i + 1) % ukuran bucket
     * 10. Ulangi dari tahap ke 4
     */
    V* get(const K& key)
    {
        const uint64_t hash = hasher.hash(key);

        // std::cout << hash << std::endl;
        size_t currentPsl = 0, i = hash % bucketSize;

        BucketType *bucket;
         __builtin_prefetch(&buckets[i], 0, 0);
        while (true)
        {
            bucket = &buckets[i];

            if (!bucket->filled || currentPsl > bucket->psl)
                return nullptr;

            if (bucket->hash == hash && bucket->key == key)
            {
                return &bucket->value;
            }

            currentPsl++;

            i = (i + 1) % bucketSize;
            __builtin_prefetch(&buckets[i], 0, 0);
        }
    }

    /**
     * Menambahkan pasangan kunci dan nilai ke hash table
     * 
     * Algortma:
     * 1. Cari hash dari kunci
     * 2. Buat bucket baru dan isi nilai bucket dengan psl = 0, bucket ini akan disebut bucket baru
     * 3. Isi nilai i dengan hash % ukuran bucket
     * 4. Dapatkan bucket pada indeks ke-i
     * 5. Jika bucket kosong maka loncat ke tahap 11
     * 6. Jika hash bucket sama dengan hash kunci tujuan dan kunci bucket sama dengan kunci tujuan maka
     *    ganti nilai di bucket dengan nilai baru.
     *    Algoritma selesai
     * 7. Jika psl bucket baru lebih dari psl bucket ke-i, maka swap bucket ke-i dengan bucket sekarang
     *    Ini membuat bucket sekarang akan menjadi bucket ke-i dan bucket ke-i akan menjadi bucket baru yang harus kita insert
     *    Ini membuat sifat yang dibutuhkan algoritma get tahap ke-6 dapat dipenuhi
     * 8. Tambah nilai psl bucket sekarang dengan 1.
     *    Alasanya karena jarak i akan bertambah 1 sehingga jarak dari i ke indeks ideal bertambah 1
     * 9. Ganti nilai i menjadi (i + 1) % ukuran bucket
     * 10. Ulangi dari tahap ke 4
     * 11. Isi bucket ke-i dengan bucket baru. 
     *     Pada langkap ini. Sudah bisa dipastikan bahwa bucket ke-i dalam kondisi kosong
     */
    void internalInsert(const K &key, const V &value)
    {
        const uint64_t hash = hasher.hash(key);
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
        const uint64_t hash = hasher.hash(key);

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