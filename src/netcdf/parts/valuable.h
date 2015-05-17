#ifndef NETCDF_VALUABLE_H
#define NETCDF_VALUABLE_H

#pragma once

#include "enums.h"
#include "value.h"
#include "utils.hpp"

///////////////////////////////////////////////////////////////////////////////

struct valuable {

    nc_type type;

    value_vector values;

    virtual nc_type get_type() const;
    void set_type(nc_type const & aType);

    template<class _Vector>
    void set_values(_Vector const & theValues) {

        nc_type type;

        //TODO: TBD: may throw an exception here instead...
        if (!try_get_type_for<_Vector::value_type>(type))
            return;

        set_type(type);

        // Do it this way. This is way more efficient than the aggregate function.
        values.clear();

        std::for_each(theValues.cbegin(), theValues.cend(),
            [&](_Vector::value_type x) { values.push_back(value(x)); });
    }

    virtual ~valuable();

protected:

    valuable(nc_type aType = nc_absent);
    valuable(valuable const & other);
};

#endif //NETCDF_VALUABLE_H
