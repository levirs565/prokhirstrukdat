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

## Penjelasan Tugas

## Buku

Ada di `Perpustakaan.cpp`.

Red Black Tree digunakan untuk menyimpan data Buku yang diurutkan berdasarkan Judul dan ISBN. Data buku akan terurut ascending berdasarkan Judul. Jika ada Judul yang sama maka akan diurutkan berdasarkan ISBN.

Hash digunakan untuk menyimpan buku dengan ISBN sebagai kunci dan data buku sebagai value.

Terdapat fitur untuk pencarian rentang berdasarkan judul. Urutan karakter adalah AaBbCcDd... dst. Ini berarti jika mencari judul dari rentang a sampai b akan mencakup buku dengan judul a, B, dan b. Tetapi tidak termasuk buku berjudul ba. Alasanya adalah karena menurut urutan kamus ba berada setelah b.

Terdapat fitur yang memberikan daftar buku dengan tahun terbit tertua. Ini mengunakan algoritma Top K Largest.

## Rumah Sakit

Ada di `Rumah_Sakit.cpp`

Red Black Tree digunakan untuk menyimpan data Pasien yang diurutkan berdasarkan Nama dan ID. Data pasien akan terurut ascending berdasarkan Nama. Jika ada Nama yang sama maka akan diurutkan berdasarkan ID.

Hash digunakan untuk menyimpan pasien dengan ID sebagai kunci dan data pasien sebagai value.

Terdapat fitur pencarian rentang berdasarkan nama pasien. Aturannya urutan sama seperti di tugas Buku.

Terdapat fitur yang memberikan daftar pasien dengan durasi rawat inap terlama. Ini menggunakan algoritma Top K Largest.

## Sistem Manajemen Toko

Ada di `Kelontong.cpp`

Red Black Tree digunakan untuk menyimpan data produk yang diurutkan berdasarkan Nama dan SKU. Data produk akan terurut ascending berdasarkan Nama. Jika ada Nama yang sama maka akan diurutkan berdasarkan SKU. Terdapat pilihan untuk menampilkan data secara preorder, inorder, dan postorder. Perlu diingat bahwa metode postorder tidak menghasilkan urutan descending

Hash digunakan untuk menyimpan produl dengan SKU sebagai kunci dan data produk sebagai value.

Terdapat fitur pencarian rentang berdasarkan nama produk. Aturannya urutan sama seperti di tugas Buku.

Untuk mencari produk dengan nama awal "susu" bisa digunakan trik dengan cara pencarian rentang dengan nama awalnya adalah "susu" dan nama akhirnya adalah "susv". Bisa juga dengan cara nama awalnya "susu" dan nama akhirnya adalah "susuz". Ini lebih manusiawai daripada cara pertama.

Terdapat fitur yang memberikan daftar produk dengan harga termurah. Ini menggunakan algoritma Top K Largest.

### PPDB

Ada di `ppdb.cpp`.

Red Black Tree digunakan untuk menyimpan data Siswa yang diurutkan berdasarkan Jalur Masuk dan NISN. Data siswa akan terurut ascending berdasarkan Jalur Masuk. Jika ada Jalur Masuk yang sama maka diurutkan berdasarkan NISN. Ini berarti, data Siswa akan terutut ascending berdasarkan NISN jika mempunyai Jalur Masuk sama.

Hash digunakan untuk menyimpan data dengan kunci adalah NISN dan nilai adalah data Siswa.

Terdapat fitur untuk mencari siswa berdasarkan Jalur Masuk dan NISN Awal. Ini akan melakukan pencarian rentang dengan Jalur Masuk sesuai dan NISN dimulai dari NISN input sampai NISN input yang telah diisi nilai 9 dibelakang. Misalkan mencari siswa dengan Jalur Masuk A dan NISN dimulai dari 123 maka pencarian rentang akan dilakukan dengan Jalur Masuk Awal A, Jalur Masuk Akhir B, NISN Awal 123, NISN Akhir 1239999999. Penambahan angka 9 terjadi secara otomatis.

Terdapat fitur autentikasi dengan menggunakan hash. Tetapi fitur ini tidak digunakan pada saat masuk ke aplikasi.