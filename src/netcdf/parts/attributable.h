#ifndef NETCDF_ATTRIBUTABLE_H
#define NETCDF_ATTRIBUTABLE_H

#pragma once

#include "attr.h"

///////////////////////////////////////////////////////////////////////////////

struct attributable {

    attr_vector attrs;

    virtual ~attributable();

    virtual void add_attr(attr const & anAttr);

    // strings are kind of like vectors but they are different enough that the use cases cannot be easily co-mingled and still be useful
    virtual void add_text_attr(std::string const & name, std::string const & text);

    //TODO: TBD: methinks that type should simply be an overloaded, inherency about how to work with attributes
    template<class _Vector>
    void add_attr(std::string const & name, _Vector const & values) {
        auto & theAttr = attr(name);
        theAttr.set_values(values);
        add_attr(theAttr);
    }

    virtual attr_vector::iterator get_attr(attr_vector::size_type i);
    virtual attr_vector::iterator get_attr(std::string const & name);

protected:

    attributable();
    attributable(attributable const & other);
};

#endif //NETCDF_ATTRIBUTABLE_H
