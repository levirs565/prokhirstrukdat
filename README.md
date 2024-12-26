# Proyek Akhir Strukdat

Anggota Kelompok:

- Levi Rizki Saputra (123230127)
- Danang Adiwibowo (123230143)
- Dida Attallah Elfasdi (123230145)
- Hafiz Alaudin Rasendriya (123230149)

## Struktur File

Berikut adalah penjelasan dari file-file pendukung dari proyek kami.

- Queue
  - SPSPC Queue Ring Buffer (`SPCSCQueue.hpp`)  
    Queue yang mendukung multihreading denngan 1 thread sebagai producer (yang melakukan push/enqueue)
    dan 1 thread sebagai consumer (yang melakukan pop/dequeue). Queue ini diimplementasikan dengan metode
    ring buffer. Queue ini menggunakan atomic untuk sinkronisasi antar thread dan memiliki sifat *lock-free* *wait-free*.
- Binary Search Tree
  - Red Black Tree (`RBTree.hpp`)  
    Merupakan BST yang memiliki kemampuan *auto-balance*. Setiap node akan diwarnai dengan warna merah atau hitam. Warna setiap node akan mempengaruhi arah rotasi.
- Heap
  - Heap (`Heap.hpp`)
    Implementasi heap yang dibangun di atas array dinamis. Heap ini membutuhkan fungsi pembanding. Fungsi pembanding biasa akan menghasilkan *min-heap* (nilai paling kecil berada di root). Sedangkan fungsi pembanding yang terbalik akan menghasilkan *max-heap* (nilai paling besar berada di root).
  - Top K Largest (`TopKLargest.hpp`)
    Algoritma yang dengan efisien memberikan daftar K elemen terbesar. Implementasi ini menggunakan struktur data Heap dan membutuhkan fungsi pembanding. Fungsi pembanding biasa akan meemberikan daftar K elemen terbesar dimulai dari elemen terkecil. Sedangkan fungsi pembanding terbalik akan memberikan daftar K elemen terkecil dimulai dari elemen terbesar.
- Hash
  - Fungsi Hash
    - SipHash (`HalfSipHash.h`)  
      Fungsi hash yang sudah teruji. Fungsi ini menggunakan operasi add, rotate, dan xor untuk menghasilkan hash. Fungsi SipHash menghasilkan hash berukuran 128 bit. Karena alasan kesederhanaan, file ini hanya mengimplementasikan variasi dari SipHash yang bernama HalfSipHash. HalfSipHash dapat menghasilkan hash dengan ukuran 32 bit dan 64 bit. Untuk mengurangi jumlah collision kita memilih HalfSipHash 64 bit. Kode ini diadaptasi dari [https://github.com/veorq/SipHash/blob/master/halfsiphash.c](https://github.com/veorq/SipHash/blob/master/halfsiphash.c).
  - Hash Table
    - Robin Hood Hash Table (`RobinHoodHashMap.hpp`)  
      Hash table Open Adressing dengan linear probing dan metode Robin Hood. Hash table ini menggunakan array dinamis. Jika jumlah data lebih dari 85% dari kapasitas, maka array diperbesar 2 kali. Jika jumlah data kurang dari 40% dari kapasitas, maka array diperkecil 1/2.
- Window API (Winapi)  
  Winapi merupakan API bawaan dari Windows  
  - `Winapi.hpp`, berisi exception untuk bekerja dengan Winapi
  - `UI.hpp`  
    UI Library yang dibuat dari awal dengan 100% memanfaatkan Winapi.
  - `WorkerThread`  
    Berisi thread yang dapat digunakan untuk mengerjakan tugas yang dapat menghambat pekerjaan UI. Menggunakan SPSC Queue untuk menyimpan antrian tugas.
- CSV
  - `CSVReader.hpp`  
    Berisi kelas yang digunakan untuk membaca file CSV baris per baris
- `Utils.hpp`, berisi fungsi utilitas kecil
- `TImer.hpp`, berisi utilitas untuk menghitung durasi dari sautu proses
