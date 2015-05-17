#ifndef NETCDF_ATTR_H
#define NETCDF_ATTR_H

#pragma once

#include "named.h"
#include "valuable.h"

///////////////////////////////////////////////////////////////////////////////

struct attributable;

typedef std::vector<uint8_t> byte_vector;
typedef std::vector<int16_t> short_vector;
typedef std::vector<int32_t> int_vector;
typedef std::vector<float_t> float_vector;
typedef std::vector<double_t> double_vector;

struct attr : public named, public valuable {

    attr();
    attr(attr const & other);

    // this is a special case for attr, and not valuable, in general
    void set_text(std::string const & text);

private:

    attr(std::string const & name, nc_type type = nc_absent);
    attr(std::string const & name, std::string const & text);

    friend struct attributable;

    static bool is_supported_type(nc_type type);
};

typedef std::vector<attr> attr_vector;

#endif //NETCDF_ATTR_H
