#ifndef NETCDF_NAMED_H
#define NETCDF_NAMED_H

#pragma once

#include <cstdint>
#include <string>

///////////////////////////////////////////////////////////////////////////////

struct named {
    std::string name;
    int32_t get_name_length() const;
    virtual ~named();
protected:
    named();
    named(std::string const & name);
    named(named const & other);
};

#endif //NETCDF_NAMED_H
