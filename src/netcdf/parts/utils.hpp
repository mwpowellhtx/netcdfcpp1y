#ifndef NETCDF_UTILS_H
#define NETCDF_UTILS_H

#pragma once

#include "enums.h"

///////////////////////////////////////////////////////////////////////////////

int32_t pad_width(int32_t width);

bool try_pad_width(int32_t & width);

template<typename _Ty>
nc_type get_type_for() {

    auto type = nc_absent;
    auto & tid = typeid(_Ty);

    // nc_char is a special case that must be handled apart from these types.

    if (tid == typeid(uint8_t))
        type = nc_byte;
    else if (tid == typeid(int16_t))
        type = nc_short;
    else if (tid == typeid(int32_t))
        type = nc_int;
    else if (tid == typeid(float_t))
        type = nc_float;
    else if (tid == typeid(double_t))
        type = nc_double;

    return type;
}

template<typename _Ty>
bool try_get_type_for(nc_type & type) {
    type = get_type_for<_Ty>();
    return type != nc_absent;
}

bool is_endian_type(nc_type type);

bool is_primitive_type(nc_type type);

int32_t get_primitive_value_size(nc_type type);

#endif //NETCDF_UTILS_H
