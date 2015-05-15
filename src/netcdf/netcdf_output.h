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

    template<typename _Ty>
    static std::ostream & write(std::ostream & os, _Ty const & field) {
        //TODO: consider how to reverse byte order when necessary ...
        os.write(reinterpret_cast<const char*>(&field), sizeof(field));
        return os;
    }

private:

    void prepare_var_array(netcdf & cdf);

    void write_magic(magic const & magic);

    void write_text(std::string const & str);

    void write_named(named const & n);

    void write_primitive(value const & v, nc_type const & type);

    void write_dim(dim const & dim);

    void write_dims(dim_vector const & dims);

    void write_attr(attr const & attr);

    void write_attrs(attr_vector const & attrs);

    void write_var_header(var & v, dim_vector const & dims, bool useClassic);

    void write_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic);

    void write_var_data(var const & v, dim_vector const & dims, bool useClassic);

    void write_vars_data(var_vector const & vars, dim_vector const & dims, bool useClassic);

    template<typename _Ty>
    _Ty get_reversed_byte_order(_Ty const & x) {
        _Ty local = x;
        return reverse_byte_order ? swap_endian(local) : local;
    }

    template<typename _Vector>
    void write_typed_array_prefix(_Vector const & values, nc_type presentType) {

        // Really transparent way of accomplishing this...
        int32_t type = values.size() ? presentType : nc_absent;
        write(os, get_reversed_byte_order(type));

        int32_t nelems = values.size();
        write(os, get_reversed_byte_order(nelems));
    }
};

#endif //NETCDF_OUTPUT_H
