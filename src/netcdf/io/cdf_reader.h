#ifndef NETCDF_CDF_READER_H
#define NETCDF_CDF_READER_H

#pragma once

#include "../netcdf.h"
#include "cdf_binary_base.h"

#include <istream>

///////////////////////////////////////////////////////////////////////////////

struct cdf_reader : public cdf_binary_base {
private:

    std::istream * pIS;

public:

    cdf_reader(std::istream * pIS, bool reverse_byte_order = false);

private:

    void read_magic(magic & magic);

    std::string read_text();

    void read_named(named & named);

    bool try_read_primitive(value & value, nc_type const & type);

    value read_primitive(nc_type const & type);

    bool try_read_typed_array_prefix(nc_type & type, int32_t & nelems);

    void read_dim(dim & aDim);

    void read_dims(dim_vector & dims);

    void read_attr(attr & anAttr);

    void read_attrs(attr_vector & dims);

    void read_dimids(dimid_vector & dimids);

    //TODO: consider whether dims ought not be a first-class part of var_array...
    void read_var_header(var & aVar, dim_vector const & dims, bool useClassic);

    void read_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic);

    void read_var_data(var & aVar, dim_vector const & dims, bool useClassic);

    void read_vars_data(var_vector & vars, dim_vector const & dims, bool useClassic);

    cdf_reader & read_cdf(netcdf & cdf);

    friend cdf_reader & operator>>(cdf_reader & reader, netcdf & cdf);
};

///////////////////////////////////////////////////////////////////////////////

cdf_reader & operator>>(cdf_reader & reader, netcdf & cdf);

#endif //NETCDF_CDF_READER_H
