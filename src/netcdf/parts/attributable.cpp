
#include "attributable.h"

#include <algorithm>

///////////////////////////////////////////////////////////////////////////////

attributable::attributable()
    : attrs() {
}

attributable::attributable(attributable const & other)
    : attrs(other.attrs) {
}

attributable::~attributable() {
}

void attributable::add_attr(attr const & theAttr) {
    //TODO: check for things like name conflicts, etc
    attrs.push_back(theAttr);
}

void attributable::add_text_attr(std::string const & name, std::string const & text) {
    add_attr(attr(name, text));
}

attr_vector::iterator attributable::get_attr(attr_vector::size_type i) {
    return attrs.begin() + i;
}

attr_vector::iterator attributable::get_attr(std::string const & name) {
    return std::find_if(attrs.begin(), attrs.end(),
        [&](attr const & x) { return x.name == name; });
}
