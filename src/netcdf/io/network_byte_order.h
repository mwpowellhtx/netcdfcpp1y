#ifndef NETWORK_BYTE_ORDER_H
#define NETWORK_BYTE_ORDER_H

#pragma once

#include <algorithm>

bool is_little_endian();
bool is_big_endian();

template <typename _Ty>
_Ty swap_endian(_Ty & x) {
    char & raw = reinterpret_cast<char &>(x);
    std::reverse(&raw, &raw + sizeof(_Ty));
    return x;
}

#endif //NETWORK_BYTE_ORDER_H
