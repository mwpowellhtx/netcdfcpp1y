#ifndef NETCDF_MAGIC_H
#define NETCDF_MAGIC_H

#pragma once

#include "enums.h"

///////////////////////////////////////////////////////////////////////////////

struct magic {
    typedef char key_type[3];
    key_type key;
    cdf_version version;

    magic();

    magic(magic const & other);

    bool is_classic() const;
    bool is_x64() const;

private:

    void init();
};

#endif //NETCDF_MAGIC_H
