
#include "dim.h"

///////////////////////////////////////////////////////////////////////////////

dim::dim()
    : named()
    , dim_length(0) {
}

dim::dim(std::string const & name, int32_t dim_length)
    : named(name)
    , dim_length(dim_length) {
}

dim::dim(dim const & other)
    : named(other)
    , dim_length(other.dim_length) {
}

dim::~dim() {
}

bool dim::is_record() const {
    return !dim_length;
}

int32_t dim::get_dim_length_part() const {
    return is_record() ? 1 : dim_length;
}
