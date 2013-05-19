#include "ccmp.h"

#include "polarssl-aes.h"

#include <cstring>

static void xor_block(uint8_t* dst, const uint8_t* key, int len) {
    while (len--) {
        *dst++ ^= *key++;
    }
}

bool CcmpDecrypt(const uint8_t* pkt, int len, const uint8_t* tk, uint8_t* out,
                 int* out_len) {
    aes_context aes;
    aes_setkey_enc(&aes, tk, 128);

    uint8_t computed_mic[16];
    uint8_t pad[16];

    // QoS packets have 2 additional bytes in the 802.11 header. We need to
    // know if the packet is a QoS packet
    bool is_qos = (pkt[0] & 0xf0) == 0x80;

    const uint8_t* ccmp_pkt = pkt + (is_qos ? 26 : 24);
    int ccmp_len = len - (is_qos ? 26 : 24);
    int data_len = ccmp_len - 16; // 8 bytes header, 8 bytes trailer

    // CCMP Initial Block
    uint8_t ib[16] = {
        0x59,
        (uint8_t)(is_qos ? pkt[24] & 0x0f : 0x00),
        pkt[10], pkt[11], pkt[12], pkt[13], pkt[14], pkt[15], // src addr
        ccmp_pkt[7], ccmp_pkt[6], ccmp_pkt[5],
        ccmp_pkt[4], ccmp_pkt[1], ccmp_pkt[0],
        (uint8_t)(data_len >> 8), (uint8_t)(data_len & 0xFF)
    };

    // Associated Authenticated Data
    uint8_t aad[32] = {
        0x00, (uint8_t)(is_qos ? 24 : 22),
        (uint8_t)(pkt[0] & 0x8f), (uint8_t)(pkt[1] & 0xc7),
        pkt[4], pkt[5], pkt[6], pkt[7], pkt[8], pkt[9], // bssid
        pkt[10], pkt[11], pkt[12], pkt[13], pkt[14], pkt[15], // src addr
        pkt[16], pkt[17], pkt[18], pkt[19], pkt[20], pkt[21], // dst addr
        (uint8_t)(pkt[22] & 0x0F), 0x00,
        (uint8_t)(is_qos ? pkt[24] & 0x0f : 0x00), 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    aes_crypt_ecb(&aes, AES_ENCRYPT, ib, computed_mic);
    xor_block(computed_mic, aad, 16);
    aes_crypt_ecb(&aes, AES_ENCRYPT, computed_mic, computed_mic);
    xor_block(computed_mic, aad + 16, 16);
    aes_crypt_ecb(&aes, AES_ENCRYPT, computed_mic, computed_mic);

    ib[0] &= 0x07;
    ib[14] = ib[15] = 0;
    aes_crypt_ecb(&aes, AES_ENCRYPT, ib, pad);

    uint8_t mic[8];
    memcpy(mic, pkt + len - 8, 8);
    xor_block(mic, pad, 8);

    int i = 1;
    const uint8_t* src_ptr = ccmp_pkt + 8; // skip the header
    uint8_t* dst_ptr = out;
    int space = data_len;

    *out_len = 0;
    while (space) {
        int block_len = space < 16 ? space : 16;

        // Decrypt a block
        ib[14] = (i >> 8) & 0xff;
        ib[15] = (i & 0xff);
        aes_crypt_ecb(&aes, AES_ENCRYPT, ib, pad);
        memcpy(dst_ptr, src_ptr, block_len);
        xor_block(dst_ptr, pad, block_len);
        xor_block(computed_mic, dst_ptr, block_len);
        aes_crypt_ecb(&aes, AES_ENCRYPT, computed_mic, computed_mic);

        src_ptr += block_len;
        dst_ptr += block_len;
        *out_len += block_len;
        space -= block_len;
        i++;
    }

    return memcmp(mic, computed_mic, 8) == 0;
}
