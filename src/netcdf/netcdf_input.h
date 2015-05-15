#ifndef NETCDF_INPUT_H
#define NETCDF_INPUT_H

#pragma once

#include <istream>

#include "netcdf_file.h"
#include "network_byte_order.h"

///////////////////////////////////////////////////////////////////////////////

struct cdf_reader {
private:

    std::istream * pIS;

    bool reverseByteOrder;

public:

    cdf_reader(std::istream * pIS, bool reverseByteOrder = false);

private:

    void read_magic(magic & magic);

    std::string read_text();

    void read_named(named & named);

    bool try_read_primitive(value & value, nc_type const & type);

    value read_primitive(nc_type const & type);

    bool try_read_typed_array_prefix(nc_type & type, int32_t & nelems);

    void read_dim(dim & aDim);

    void read_dims(dim_vector & dims);

    attr read_attr();

    void read_attrs(attr_vector & dims);

    //TODO: consider whether dims ought not be a first-class part of var_array...
    void read_var_header(var & aVar, dim_vector const & dims, bool useClassic);

    void read_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic);

    void read_var_data(var & v, dim_vector const & dims, bool useClassic);

    void read_vars_data(var_vector & vars, dim_vector const & dims, bool useClassic);

    cdf_reader & read_cdf(netcdf & cdf);

    friend cdf_reader & operator>>(cdf_reader & reader, netcdf & cdf);

    template<typename _Ty>
    _Ty get_reversed_byte_order(_Ty const & x) {
        _Ty local = x;
        return reverseByteOrder ? swap_endian(local) : local;
    }
};

///////////////////////////////////////////////////////////////////////////////

cdf_reader & operator>>(cdf_reader & reader, netcdf & cdf);

#endif //NETCDF_INPUT_H
