#ifndef __DRC_CAP_CRC32_H_
#define __DRC_CAP_CRC32_H_

#include <cstdint>
#include <cstdlib>

uint32_t crc32(uint32_t crc, const uint8_t* buf, size_t size);

#endif
