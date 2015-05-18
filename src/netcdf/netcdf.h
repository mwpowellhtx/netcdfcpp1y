#ifndef NETCDF_FILE_H
#define NETCDF_FILE_H

#pragma once

#include "parts/magic.h"
#include "parts/dim.h"
#include "parts/var.h"

///////////////////////////////////////////////////////////////////////////////

struct netcdf : public attributable {
public:

    typedef std::vector<dim_vector::iterator> dim_vector_iterator_vector;

    magic magic;

    //TODO: upwards of MAX_INT32 (yes, that's 0xffffffff, or STREAMING) ...
    //TODO: TBD: numrecs could (/should) be more of a dynamic get function?
    int32_t numrecs;

    dim_vector dims;

    var_vector vars;

    netcdf();
    netcdf(netcdf const & other);

    virtual ~netcdf();

    virtual dim_vector::iterator add_dim(dim const & aDim, int32_t default_dim_length = 1);
    virtual dim_vector::iterator add_dim(std::string const & name, int32_t dim_length = 1, int32_t default_dim_length = 1);

    virtual dim_vector::iterator get_dim(dim_vector::size_type i);
    virtual dim_vector::iterator get_dim(std::string const & name);

    virtual void set_unlimited_dim(dim_vector::iterator dim_it, int32_t default_dim_length = 1);
    virtual void set_unlimited_dim(dim_vector::size_type const & i, int32_t default_dim_length = 1);
    virtual void set_unlimited_dim(std::string const & name, int32_t default_dim_length = 1);

    virtual var_vector::iterator add_var(var const & aVar);
    virtual var_vector::iterator add_var(std::string  & name, nc_type aType);

    virtual var_vector::iterator get_var(var_vector::size_type i);
    virtual var_vector::iterator get_var(std::string const & name);

    virtual void redim_var(var_vector::iterator var_it, dim_vector_iterator_vector const & dim_its);
    virtual void redim_var(var_vector::size_type i, dim_vector_iterator_vector const & dim_its);
    virtual void redim_var(std::string const & name, dim_vector_iterator_vector const & dim_its);
};

/* TODO: TBD: still to come, how to work with the "shape" of data via the netcdf;
that's the whole point of utilizing the format, that data can be shaped and worked with
TBD: could be some "operators" that permit definition, mutation, access, and so on;
might be interesting to consider whether variadics hold any value for accessing values
along a range of values */

#endif //NETCDF_FILE_H
