#include "network_byte_order.h"

#include <cstdint>

bool is_big_endian() {

    static const uint32_t val = 0x01;

    static const char * pval = reinterpret_cast<const char *>(&val);

    return pval[0] == 0;
}

bool is_little_endian() {
    return !is_big_endian();
}
