#include "netcdf_input.h"

#include <cassert>
#include <limits>

template<typename _Ty>
_Ty read(std::istream & is) {
    _Ty x;
    is.read(reinterpret_cast<char*>(&x), sizeof(x));
    return x;
}

cdf_version to_cdf_version(uint8_t value) {
    return static_cast<cdf_version>(value);
}

netcdf_reader::netcdf_reader(std::istream * pis, bool reverseByteOrder)
    : pis(pis), reverseByteOrder(reverseByteOrder) {
}

void netcdf_reader::read_magic(magic & magic) {

    char tmp[sizeof(magic.key)];

    pis->read(tmp, sizeof(tmp));

    for (auto i = 0; i < sizeof(tmp); i++)
        if (tmp[i] != magic.key[i])
            throw std::exception("invalid file format");

    magic.version = to_cdf_version(read<int8_t>(*pis));
}

//TODO: may refactor this one...
nc_type to_nc_type(int32_t value) {
    return static_cast<nc_type>(value);
}

std::string netcdf_reader::read_text() {

    //see: http://www.unidata.ucar.edu/software/netcdf/docs_rc/file_format_specifications.html
    //TODO: notwithstanding considerations such as character sets, regex, etc
    std::string text;

    // This is the key to reading a proper name.
    auto nelems = get_reversed_byte_order(read<int32_t>(*pis));

    while (nelems-- > 0)
        text += read<int8_t>(*pis);

    int32_t readCount = text.length();

    while (try_pad_width(readCount))
        read<int8_t>(*pis);

    return text;
}

void netcdf_reader::read_named(named & named) {
    named.name = read_text();
}

bool netcdf_reader::try_read_primitive(value & value, nc_type const & type) {

    // This use case is handled elsewhere.
    assert(!(type == nc_char
        || type == nc_dimension
        || type == nc_attribute
        || type == nc_variable));

    switch (type) {

    case nc_byte:
        value.primitive.b = read<uint8_t>(*pis);
        return true;

    case nc_short:
        value.primitive.s = get_reversed_byte_order(read<int16_t>(*pis));
        return true;

    case nc_int:
        value.primitive.i = get_reversed_byte_order(read<int32_t>(*pis));
        return true;

    case nc_float:
        value.primitive.f = read<float_t>(*pis);
        return true;

    case nc_double:
        value.primitive.d = read<double_t>(*pis);
        return true;
    }

    return false;
}

value netcdf_reader::read_primitive(nc_type const & type) {

    value result;

    assert(try_read_primitive(result, type));

    return result;
}

bool netcdf_reader::try_read_typed_array_prefix(nc_type & type, int32_t & nelems) {

    type = to_nc_type(get_reversed_byte_order(read<int32_t>(*pis)));

    nelems = get_reversed_byte_order(read<int32_t>(*pis));

    return type != nc_absent;
}

dim netcdf_reader::read_dim() {

    dim result;

    read_named(result);

    result.dim_length = get_reversed_byte_order(read<int32_t>(*pis));

    return result;
}

void netcdf_reader::read_dims(dim_vector & dims) {

    nc_type type;
    int32_t nelems;

    //TODO: may want to pre-allocate these as with variable/data
    dims.clear();

    if (try_read_typed_array_prefix(type, nelems)) {

        // Assert before and after expectations.
        assert(type == nc_dimension);

        while (dims.size() != nelems)
            dims.push_back(read_dim());

        ////TODO: nothing to assert here that isn't an inherent part of being the model
        //assert(arr.get_type() == nc_dimension);
    }
}

attr netcdf_reader::read_attr() {

    attr result;

    read_named(result);

    //TODO: the examples I am downloading do not appear to adhere to the Classic or 64-bit file format... clearly we're talking at least netCDF-4 (?), maybe later ...
    result.type = to_nc_type(get_reversed_byte_order(read<int32_t>(*pis)));

    if (result.get_type() == nc_char) {
        // 'nelems' is a function of the std::string in this use case.
        result.values = { value(read_text()) };
    }
    else {

        // Otherwise read the values as they were indicated.
        auto nelems = get_reversed_byte_order(read<int32_t>(*pis));

        //TODO: may want to pre-allocate these as with variable/data
        result.values.clear();

        while (result.values.size() != nelems)
            result.values.push_back(read_primitive(result.get_type()));
    }

    return result;
}

void netcdf_reader::read_attrs(attr_vector & attrs) {

    nc_type type;
    int32_t nelems;

    //TODO: may want to pre-allocate these as with variable/data
    attrs.clear();

    if (try_read_typed_array_prefix(type, nelems)) {

        // Assert before and after expectations.
        assert(type == nc_attribute);

        while (attrs.size() != nelems)
            attrs.push_back(read_attr());

        //TODO: nothing to assert here: "type" is inherent property whether there are elements...
        //assert(arr.get_type() == nc_attribute);
    }
}

var netcdf_reader::read_var_header(dim_vector const & dims, bool useClassic) {

    auto result = var();

    read_named(result);

    auto nelems = get_reversed_byte_order(read<int32_t>(*pis));
    //TODO: may want to pre-allocate these as with variable/data
    result.dimids.clear();

    while (result.dimids.size() != nelems)
        result.dimids.push_back(get_reversed_byte_order(read<int32_t>(*pis)));

    read_attrs(result.vattrs);

    result.type = to_nc_type(get_reversed_byte_order(read<int32_t>(*pis)));

    //TODO: either redundant and/or obsolete, but still support if possible... maybe with try/catch to protect calculations
    result.vsize = get_reversed_byte_order(read<int32_t>(*pis));

    //TODO: rework the calculation ...
    ////TODO: TBD: this is an appropriate assertion?
    //assert(result.vsize == result.get_calculated_size(dims));

    /* TODO: TBD: reading "x64" files may be an issue: seems it may be fixed ...
    http://connect.microsoft.com/VisualStudio/feedback/details/627639/std-fstream-use-32-bit-int-as-pos-type-even-on-x64-platform */

    if (useClassic)
        result.offset.begin = get_reversed_byte_order(read<int32_t>(*pis));
    else
        //TODO: might need to do this one a bit differently (?)
        result.offset.begin64 = get_reversed_byte_order(read<int64_t>(*pis));

    return result;
}

void netcdf_reader::read_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic) {

    offset_t current;

    if (useClassic)
        current.begin = 0;
    else
        current.begin64 = 0LL;

    nc_type type;
    int32_t nelems;

    //TODO: may want to pre-allocate these as with variable/data
    vars.clear();

    if (try_read_typed_array_prefix(type, nelems)) {

        // Assert before and after expectations.
        assert(type == nc_variable);

        while (vars.size() != nelems) {

            vars.push_back(read_var_header(dims, useClassic));

            auto & last = vars[vars.size() - 1];

            // Verify that the fp values are properly aligned.
            if (useClassic) {
                assert(last.offset.begin > current.begin);
                current.begin = last.offset.begin;
            }
            else {
                assert(last.offset.begin64 > current.begin64);
                current.begin64 = last.offset.begin64;
            }
        }

        ////TODO: ditto inherent parts of the model
        //assert(arr.get_type() == nc_variable);
    }
}

void netcdf_reader::read_var_data(var & v, dim_vector const & dims, bool useClassic) {

    if (useClassic)
        pis->seekg(v.offset.begin, std::ios::beg);
    else
        pis->seekg(v.offset.begin64, std::ios::beg);

    const auto type = v.get_type();
    const auto nelems = v.vsize / get_primitive_value_size(type);

    v.data = std::vector<value>(nelems);

    // A little more efficient than creating on the stack and returning, copying, etc.
    for (auto i = 0; i < nelems; i++)
        assert(try_read_primitive(v.data[i], type));
}

void netcdf_reader::read_vars_data(var_vector & vars, dim_vector const & dims, bool useClassic) {

    // Read the non-record data in header-specified order.
    for (auto & v : vars)
        if (!v.is_record(dims))
            read_var_data(v, dims, useClassic);

    // Then read the record data. Should be only one, but may occur in any position AFAIK.
    for (auto & v : vars)
        if (v.is_record(dims))
            read_var_data(v, dims, useClassic);
}

netcdf_reader & netcdf_reader::read_cdf(netcdf & cdf) {

    read_magic(cdf.magic);

    //TODO: pick this one up here: look up concerning the BNF format what to expect ...
    cdf.numrecs = get_reversed_byte_order(read<int32_t>(*pis));

    read_dims(cdf.dims);

    read_attrs(cdf.gattrs);

    read_vars_header(cdf.vars, cdf.dims, cdf.magic.is_classic());

    read_vars_data(cdf.vars, cdf.dims, cdf.magic.is_classic());

    return *this;
}

netcdf_reader & operator>>(netcdf_reader & reader, netcdf & cdf) {
    return reader.read_cdf(cdf);
}
