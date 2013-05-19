#ifndef __DRC_CAP_CCMP_H_
#define __DRC_CAP_CCMP_H_

#include <cstdint>

bool CcmpDecrypt(const uint8_t* pkt, int len, const uint8_t* tk, uint8_t* out,
                 int* out_len);

#endif
