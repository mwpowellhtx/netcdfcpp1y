#include "valuable.h"

///////////////////////////////////////////////////////////////////////////////

valuable::valuable(nc_type theType)
    : type(theType)
    , values() {
}

valuable::valuable(valuable const & other)
    : type(other.type)
    , values(other.values) {
}

valuable::~valuable() {
}

nc_type valuable::get_type() const {
    return type;
}

void valuable::set_type(nc_type const & theType) {
    type = theType;
}
