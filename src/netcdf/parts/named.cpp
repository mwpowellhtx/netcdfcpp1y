#include "named.h"

///////////////////////////////////////////////////////////////////////////////

named::named() {
}

named::named(std::string const & name)
    : name(name) {
}

named::named(named const & other)
    : name(other.name) {
}

named::~named() {
}

int32_t named::get_name_length() const {
    return static_cast<int32_t>(name.size());
}
