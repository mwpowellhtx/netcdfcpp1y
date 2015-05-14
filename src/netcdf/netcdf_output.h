#ifndef NETCDF_OUTPUT_H
#define NETCDF_OUTPUT_H

#pragma once

#include <ostream>

#include "netcdf_file.h"
#include "network_byte_order.h"

struct cdf_writer {
private:

    std::ostream & os;

    std::string path;

    bool reverse_byte_order;

public:

    cdf_writer(std::ostream & os, bool reverse_byte_order = true);

    cdf_writer & operator<<(netcdf & cdf);

private:

    void prepare_var_array(netcdf & cdf);

    void write_magic(magic const & magic);

    void write_text(std::string const & str);

    void write_named(named const & n);

    void write_primitive(value const & v, nc_type const & type);
    
    void write_typed_array_prefix(typed_array const & arr);

    void write_dim(dim const & dim);

    void write_dim_array(dim_array const & arr);

    void write_attr(attr const & attr);

    void write_att_array(att_array const & arr);

    void write_var_header(var & v, dim_array const & dims, bool useClassic);

    void write_var_array_header(var_array & arr, dim_array const & dims, bool useClassic);

    void write_var_data(var const & v, dim_array const & dims, bool useClassic);

    void write_var_array_data(var_array const & arr, dim_array const & dims, bool useClassic);

    template<typename _Ty>
    _Ty get_reversed_byte_order(_Ty const & x) {
        _Ty local = x;
        return reverse_byte_order ? swap_endian(local) : local;
    }
};

#endif //NETCDF_OUTPUT_H
