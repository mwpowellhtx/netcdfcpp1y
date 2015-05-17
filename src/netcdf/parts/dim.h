#ifndef NETCDF_DIM_H
#define NETCDF_DIM_H

#pragma once

#include "named.h"

#include <vector>

///////////////////////////////////////////////////////////////////////////////

struct dim : public named {
    int32_t dim_length;
    bool is_record() const;
    int32_t get_dim_length_part() const;
    dim();
    dim(dim const & other);
    virtual ~dim();

private:

    dim(std::string const & name, int32_t dim_length);

    friend struct netcdf;
};

typedef std::vector<dim> dim_vector;

#endif //NETCDF_DIM_H
