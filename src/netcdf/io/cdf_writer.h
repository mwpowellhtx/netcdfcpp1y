#ifndef NETCDF_CDF_WRITER_H
#define NETCDF_CDF_WRITER_H

#pragma once

#include "../netcdf.h"
#include "cdf_binary_base.h"

#include <ostream>

///////////////////////////////////////////////////////////////////////////////

struct cdf_writer : public cdf_binary_base {
private:

    std::ostream * pOS;

    bool reverse_byte_order;

public:

    cdf_writer(std::ostream * pOS, bool reverse_byte_order = true);

    cdf_writer & operator<<(netcdf & aCdf);

private:

    // This has to be in the header file on account of the write_typed_array_prefix function.
    // Otherwise it could be in the source file. We'll guard its access via the private keyword
    // in the meantime.
    template<typename _Ty>
    static std::ostream & write(std::ostream & os, _Ty const & field) {
        //TODO: consider how to reverse byte order when necessary ...
        os.write(reinterpret_cast<const char*>(&field), sizeof(field));
        return os;
    }

private:

    void prepare_var_array(netcdf & aCdf);

    void write_magic(magic const & aMagic);

    void write_text(std::string const & aStr);

    void write_named(named const & aNamed);

    void write_primitive(value const & aValue, nc_type const & type);

    void write_dim(dim const & aDim);

    void write_dims(dim_vector const & dims);

    void write_attr(attr const & anAttr);

    void write_attrs(attr_vector const & attrs);

    void write_var_header(var & aVar, dim_vector const & dims, bool useClassic);

    void write_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic);

    void write_var_data(var const & aVar, dim_vector const & dims, bool useClassic);

    void write_vars_data(var_vector const & vars, dim_vector const & dims, bool useClassic);

    template<typename _Vector>
    void write_typed_array_prefix(_Vector const & theValues, nc_type presentType) {

        // Really transparent way of accomplishing this...
        int32_t type = theValues.size() ? presentType : nc_absent;
        write(*pOS, get_reversed_byte_order(type));

        int32_t nelems = theValues.size();
        write(*pOS, get_reversed_byte_order(nelems));
    }
};

#endif //NETCDF_CDF_WRITER_H
