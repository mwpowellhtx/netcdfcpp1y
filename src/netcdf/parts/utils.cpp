
#include "utils.hpp"
#include "value.h"

///////////////////////////////////////////////////////////////////////////////

int32_t pad_width(int32_t width) {
    while (try_pad_width(width)) {}
    return width;
}

bool try_pad_width(int32_t & width) {
    /* Padding can be ignore during read, but needs to be included during write.
    Which is really just a function of nelems multiplied by nc_type size. */
    if (!(width % sizeof(int32_t))) return false;
    width++;
    return true;
}

bool is_endian_type(nc_type type) {
    switch (type) {
    case nc_short:
    case nc_int: return true;
    }
    return false;
}

bool is_primitive_type(nc_type type) {
    switch (type) {
    case nc_byte:
    case nc_short:
    case nc_int:
    case nc_float:
    case nc_double: return true;
    }
    return false;
}

int32_t get_primitive_value_size(nc_type type) {
    const value v;
    switch (type) {
    case nc_byte: return sizeof(v.primitive.b);
    case nc_short: return sizeof(v.primitive.s);
    case nc_int: return sizeof(v.primitive.i);
    case nc_float: return sizeof(v.primitive.f);
    case nc_double: return sizeof(v.primitive.d);
    }
    throw new std::exception("unsupported type");
}
