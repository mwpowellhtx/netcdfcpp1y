#ifndef NETCDF_VAR_H
#define NETCDF_VAR_H

#pragma once

#include "dim.h"
#include "valuable.h"
#include "attributable.h"

///////////////////////////////////////////////////////////////////////////////

typedef union {
    int32_t begin;
    int64_t begin64;
} offset_t;

typedef std::vector<int32_t> dimid_vector;

//TODO: TBD: what other interface this will require to get/set/insert/update/delete variables, in a model-compatible manner
struct var : public named, public attributable, public valuable {
    //See rank (dimensionality) ... rank nelems (rank alone? or always INT ...)
    //TODO: TBD: may consider whether it is feasible to store a pointer or even iterator to iself: what happens when redimming happens, or items added to vector, that invalidates the iterator/pointer? probably...
    dimid_vector dimids;
    //TODO: may not support vsize after all? does it make sense to? especially with backward/forward compatibility growth concerns...
    int32_t vsize;
    //TODO: TBD: this one could be tricky ...
    offset_t offset;

    var();
    var(var const & other);

    virtual ~var();

    bool is_scalar() const;
    bool is_vector() const;
    bool is_matrix() const;

    bool is_record(dim_vector const & dims) const;
};

bool is_scalar(var const & aVar);
bool is_vector(var const & aVar);
bool is_matrix(var const & aVar);

typedef std::vector<var> var_vector;

#endif //NETCDF_VAR_H
