
#include "attr.h"
#include "utils.hpp"

///////////////////////////////////////////////////////////////////////////////

attr::attr()
    : named()
    , valuable() {
}

attr::attr(std::string const & name, nc_type type)
    : named(name)
    , valuable(type) {
}

attr::attr(std::string const & name, std::string const & text)
    : named(name)
    , valuable() {

    set_text(text);
}

attr::attr(attr const & other)
    : named(other)
    , valuable(other) {
}

void attr::set_text(std::string const & text) {
    set_type(nc_char);
    values = { value(text) };
}

bool attr::is_supported_type(nc_type type) {
    return type == nc_char
        || is_primitive_type(type);
}
