#ifndef NETCDF_CDF_BINARY_BASE_H
#define NETCDF_CDF_BINARY_BASE_H

#pragma once

#include "network_byte_order.h"

///////////////////////////////////////////////////////////////////////////////

struct cdf_binary_base {

    virtual ~cdf_binary_base();

protected:

    bool reverse_byte_order;

protected:

    cdf_binary_base(bool reverse_byte_order = true);

private:

    cdf_binary_base(cdf_binary_base const &) {}

protected:

    template<typename _Ty>
    _Ty get_reversed_byte_order(_Ty const & x) {
        _Ty local = x;
        return reverse_byte_order ? swap_endian(local) : local;
    }
};

#endif //NETCDF_CDF_BINARY_BASE_H
