#ifndef CRC32_H
#define CRC32_H

#include <cstddef>
#include <cstdint>

/*
 * A function that calculates the CRC-32 based on the table above is
 * given below for documentation purposes. An equivalent implementation
 * of this function that's actually used in the kernel can be found
 * in sys/libkern.h, where it can be inlined.
 */
uint32_t crc32(const void *, size_t);

#endif /* CRC32_H */
