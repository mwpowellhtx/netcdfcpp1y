
#include "netcdf.h"

#include <algorithm>

///////////////////////////////////////////////////////////////////////////////

netcdf::netcdf()
    : attributable()
    , magic()
    , numrecs(0)
    , dims()
    , vars() {
}

netcdf::netcdf(netcdf const & other)
    : attributable(other)
    , magic(other.magic)
    , numrecs(other.numrecs)
    , dims(other.dims)
    , vars(other.vars) {
}

netcdf::~netcdf() {
}

void netcdf::add_dim(dim const & theDim, int32_t default_dim_length) {

    //TODO: check for name conflicts
    //TODO: TBD: may inject policies into the netcdf, add/remove/access functions ... i.e. allow_add_record_dim

    // There may be 0-1 unlimited (record) dims.
    if (theDim.is_record()) {
        for (auto & x : dims)
            if (x.is_record())
                x.dim_length = default_dim_length;
    }

    // Always add to the end for minimum impact on previously existing dims, dimids, etc.
    dims.push_back(theDim);
}

void netcdf::add_dim(std::string const & name, int32_t dim_length, int32_t default_dim_length) {
    add_dim(dim(name, dim_length), default_dim_length);
}

void netcdf::set_unlimited_dim(dim_vector::iterator dim_it, int32_t default_dim_length) {

    for (auto it = dims.begin(); it != dims.end(); it++) {

        if (it->is_record())
            it->dim_length = default_dim_length;

        if (it == dim_it)
            it->dim_length = 0;
    }
}

void netcdf::set_unlimited_dim(dim_vector::size_type const & i, int32_t default_dim_length) {

    auto dim_it = get_dim(i);

    set_unlimited_dim(dim_it, default_dim_length);
}

void netcdf::set_unlimited_dim(std::string const & name, int32_t default_dim_length) {

    auto dim_it = get_dim(name);

    set_unlimited_dim(dim_it, default_dim_length);
}

var_vector::iterator netcdf::get_var(var_vector::size_type i) {
    return vars.begin() + i;
}

var_vector::iterator netcdf::get_var(std::string const & name) {
    return std::find_if(vars.begin(), vars.end(),
        [&](var const & x) { return x.name == name; });
}

dim_vector::iterator netcdf::get_dim(dim_vector::size_type i) {
    return dims.begin() + i;
}

dim_vector::iterator netcdf::get_dim(std::string const & name) {
    return std::find_if(dims.begin(), dims.end(),
        [&](dim const & x) { return x.name == name; });
}

void netcdf::redim_var(var_vector::iterator var_it, dim_vector_iterator_vector const & dim_its) {

    //TODO: TBD: just return? or throw?
    if (var_it == vars.end()) return;

    var_it->dimids.clear();

    auto dim_begin = dims.begin();

    for (auto & dim_it : dim_its)
        var_it->dimids.push_back(dim_it - dim_begin);

    //TODO: what else to do with the type / data ?
}

void netcdf::redim_var(var_vector::size_type i, dim_vector_iterator_vector const & dim_its) {
    redim_var(get_var(i), dim_its);
}

void netcdf::redim_var(std::string const & name, dim_vector_iterator_vector const & dim_its) {
    redim_var(get_var(name), dim_its);
}
