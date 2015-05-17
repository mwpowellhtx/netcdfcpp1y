
#include "var.h"

///////////////////////////////////////////////////////////////////////////////

//TODO: TBD: methinks that an enumerated rank holds little to no (less, at any rate) value for what is basically an open ended thing...
enum rank : int32_t {
    rank_dim_scalar = 0,
    rank_dim_vector = 1,
    rank_dim_matrix = 2,
    //TODO: TBD: others?
};

///////////////////////////////////////////////////////////////////////////////

var::var()
    : named()
    , attributable()
    , valuable(nc_double)
    , dimids()
    , vsize(0)
    , offset() {

    memset(&offset, sizeof(offset), 0);
}

var::var(var const & other)
    : named(other)
    , attributable(other)
    , valuable(other)
    , dimids(other.dimids)
    , vsize(other.vsize)
    , offset(other.offset) {
}

var::~var() {
}

bool var::is_scalar() const {
    return dimids.size() == rank::rank_dim_scalar;
}

bool var::is_vector() const {
    return dimids.size() == rank::rank_dim_vector;
}

bool var::is_matrix() const {
    //TODO: or greater?
    return dimids.size() == rank::rank_dim_matrix;
}

bool is_scalar(var const & theVar) {
    return theVar.is_scalar();
}

bool is_vector(var const & theVar) {
    return theVar.is_vector();
}

bool is_matrix(var const & theVar) {
    return theVar.is_matrix();
}

bool var::is_record(dim_vector const & dims) const {

    // The record dimension length is zero (0).
    for (auto & dimid : dimids) {
        if (dims[dimid].is_record())
            return true;
    }

    return false;
}
