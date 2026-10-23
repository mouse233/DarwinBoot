// SPDX-License-Identifier: BSD-2-Clause-Patent
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define BASE 0xa00004000ULL
#define SIZE 0x300000000ULL
#define COUNT (SIZE / 0x200000ULL + 1)
static unsigned long long words[COUNT], before[COUNT], saved[COUNT];
static int alias, stuck;
static unsigned long long slot(unsigned long long address) {
  assert(address >= BASE && address <= BASE + SIZE - 8);
  unsigned long long offset = address - BASE;
  assert(offset == SIZE - 8 || offset % 0x200000ULL == 0);
  unsigned long long index = offset == SIZE - 8 ? COUNT - 1 : offset / 0x200000ULL;
  if (alias && index == COUNT - 2) index = 0;
  return index;
}
static unsigned long long read_word(unsigned long long address) {
  return words[slot(address)];
}
static void write_word(unsigned long long address, unsigned long long value) {
  words[slot(address)] = stuck ? value & ~1ULL : value;
}
#define DARWIN_PROBE_READ(address) read_word(address)
#define DARWIN_PROBE_WRITE(address, value) write_word(address, value)
#include "../DarwinBootPkg/Driver/WindowProbe.h"
int main(void) {
  unsigned long long failed = 0;
  for (unsigned i = 0; i < COUNT; ++i) words[i] = before[i] = i * 0x12345678ULL;
  assert(DarwinWindowProbe(BASE, SIZE, saved, &failed));
  assert(memcmp(words, before, sizeof(words)) == 0);
  alias = 1;
  assert(!DarwinWindowProbe(BASE, SIZE, saved, &failed));
  assert(failed == BASE);
  assert(memcmp(words, before, sizeof(words)) == 0);
  alias = 0; stuck = 1;
  assert(!DarwinWindowProbe(BASE, SIZE, saved, &failed));
  assert(failed >= BASE && failed < BASE + SIZE);
  assert(memcmp(words, before, sizeof(words)) == 0);
  printf("PASS: 12 GiB range, %llu samples, address alias/stuck-bit detection, restoration\n", COUNT);
}
