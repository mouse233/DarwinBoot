// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef DARWINBOOT_WINDOW_PROBE_H
#define DARWINBOOT_WINDOW_PROBE_H

// Sparse diagnostic only: one 64-bit word per 2 MiB and the final word.
// All address tags are written before reading any of them, detecting aliases
// between sampled addresses as well as stuck bits. The caller owns the entire
// allocation; firmware never probes arbitrary FDT addresses or free memory.
// READ/WRITE include cache maintenance in firmware; tests substitute faulty RAM.
#define DARWIN_PROBE_STEP 0x200000ULL
static inline unsigned long long DarwinProbeOffset(unsigned long long index,
                                                   unsigned long long size) {
  return index < size / DARWIN_PROBE_STEP ? index * DARWIN_PROBE_STEP : size - 8;
}
static inline int DarwinWindowProbe(unsigned long long base, unsigned long long size,
                                    unsigned long long *saved, unsigned long long *failed) {
  unsigned long long count = size / DARWIN_PROBE_STEP + 1;
  unsigned long long i, address, expected;
  unsigned pass;
  int ok = 1;
  for (i = 0; i < count; ++i) {
    address = base + DarwinProbeOffset(i, size);
    saved[i] = DARWIN_PROBE_READ(address);
  }
  for (pass = 0; pass < 2 && ok; ++pass) {
    for (i = 0; i < count; ++i) {
      address = base + DarwinProbeOffset(i, size);
      expected = address ^ 0xa55aa55a5aa55aa5ULL;
      DARWIN_PROBE_WRITE(address, pass ? ~expected : expected);
    }
    for (i = 0; i < count; ++i) {
      address = base + DarwinProbeOffset(i, size);
      expected = address ^ 0xa55aa55a5aa55aa5ULL;
      if (DARWIN_PROBE_READ(address) != (pass ? ~expected : expected)) {
        *failed = address;
        ok = 0;
        break;
      }
    }
  }
  for (i = 0; i < count; ++i) {
    address = base + DarwinProbeOffset(i, size);
    DARWIN_PROBE_WRITE(address, saved[i]);
  }
  return ok;
}
#endif
