#pragma once

#include <stdint.h>

inline uint64_t MurmurHash_fmix64(uint64_t k) {
    k ^= k >> 33;
  k *= uint64_t(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= uint64_t(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;  
}