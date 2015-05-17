#ifndef NETCDF_ENUMS_H
#define NETCDF_ENUMS_H

#pragma once

#include <cstdint>

//http://www.unidata.ucar.edu/software/netcdf/docs/netcdf/File-Format-Specification.html
/*
Fields such as nelems has implied meaning for the model, but not to the file format.
nelems fields should preceed the thing it is counting, and are always INT (int32_t).
In the model, this is inherent in the fact that such collections are represented as vectors.

Same generally goes for nc_type specifications for the array structs. For file I/O purposes,
this can be discerned, and all we need to do is reference the model at that time, either
returning nc_absent or the array type designation, as appropriate.
*/

enum cdf_version : uint8_t {
    classic = 0x1,
    x64 = 0x2
};

enum nc_type : int32_t {
    nc_absent = 0x0,
    nc_byte = 0x1,
    nc_char = 0x2,
    nc_short = 0x3,
    nc_int = 0x4,
    nc_float = 0x5,
    nc_double = 0x6,
    nc_dimension = 0xa,
    nc_variable = 0xb,
    nc_attribute = 0xc,
};

#endif // NETCDF_ENUMS_H
