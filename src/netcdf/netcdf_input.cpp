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
    switch (value) {
    case classic: return classic;
    case x64: return x64;
    }
    throw std::exception("unsupported cdf version");
}

cdf_reader::cdf_reader(std::istream * pIS, bool reverseByteOrder)
    : pIS(pIS), reverseByteOrder(reverseByteOrder) {
}

void cdf_reader::read_magic(magic & magic) {

    char tmp[sizeof(magic.key)];

    pIS->read(tmp, sizeof(tmp));

    for (auto i = 0; i < sizeof(tmp); i++)
        if (tmp[i] != magic.key[i])
            throw std::exception("invalid file format");

    magic.version = to_cdf_version(read<int8_t>(*pIS));
}

//TODO: may refactor this one...
nc_type to_nc_type(int32_t value) {
    return static_cast<nc_type>(value);
}

std::string cdf_reader::read_text() {

    //see: http://www.unidata.ucar.edu/software/netcdf/docs_rc/file_format_specifications.html
    //TODO: notwithstanding considerations such as character sets, regex, etc
    std::string text;

    // This is the key to reading a proper name.
    auto nelems = get_reversed_byte_order(read<int32_t>(*pIS));

    while (nelems-- > 0)
        text += read<int8_t>(*pIS);

    int32_t readCount = text.length();

    while (try_pad_width(readCount))
        read<int8_t>(*pIS);

    return text;
}

void cdf_reader::read_named(named & named) {
    named.name = read_text();
}

bool cdf_reader::try_read_primitive(value & value, nc_type const & type) {

    // This use case is handled elsewhere.
    assert(!(type == nc_char
        || type == nc_dimension
        || type == nc_attribute
        || type == nc_variable));

    switch (type) {

    case nc_byte:
        value.primitive.b = read<uint8_t>(*pIS);
        return true;

    case nc_short:
        value.primitive.s = get_reversed_byte_order(read<int16_t>(*pIS));
        return true;

    case nc_int:
        value.primitive.i = get_reversed_byte_order(read<int32_t>(*pIS));
        return true;

    case nc_float:
        value.primitive.f = read<float_t>(*pIS);
        return true;

    case nc_double:
        value.primitive.d = read<double_t>(*pIS);
        return true;
    }

    return false;
}

value cdf_reader::read_primitive(nc_type const & type) {

    value result;

    assert(try_read_primitive(result, type));

    return result;
}

bool cdf_reader::try_read_typed_array_prefix(nc_type & type, int32_t & nelems) {

    type = to_nc_type(get_reversed_byte_order(read<int32_t>(*pIS)));

    nelems = get_reversed_byte_order(read<int32_t>(*pIS));

    return type != nc_absent;
}

void cdf_reader::read_dim(dim & theDim) {

    read_named(theDim);

    theDim.dim_length = get_reversed_byte_order(read<int32_t>(*pIS));
}

void cdf_reader::read_dims(dim_vector & dims) {

    nc_type type;
    int32_t nelems;

    //TODO: may want to pre-allocate these as with variable/data
    dims.clear();

    if (try_read_typed_array_prefix(type, nelems)) {

        dims = dim_vector(nelems);

        // Assert before and after expectations.
        assert(type == nc_dimension);

        for (auto i = 0; i < nelems; i++) {
            auto & aDim = dims[i];
            read_dim(aDim);
        }
    }
}

attr cdf_reader::read_attr() {

    attr result;

    read_named(result);

    //TODO: the examples I am downloading do not appear to adhere to the Classic or 64-bit file format... clearly we're talking at least netCDF-4 (?), maybe later ...
    result.type = to_nc_type(get_reversed_byte_order(read<int32_t>(*pIS)));

    if (result.get_type() == nc_char) {
        // 'nelems' is a function of the std::string in this use case.
        result.values = { value(read_text()) };
    }
    else {

        // Otherwise read the values as they were indicated.
        auto nelems = get_reversed_byte_order(read<int32_t>(*pIS));

        //TODO: may want to pre-allocate these as with variable/data
        result.values.clear();

        while (result.values.size() != nelems)
            result.values.push_back(read_primitive(result.get_type()));
    }

    return result;
}

void cdf_reader::read_attrs(attr_vector & attrs) {

    nc_type type;
    int32_t nelems;

    //TODO: may want to pre-allocate these as with variable/data
    attrs.clear();

    if (try_read_typed_array_prefix(type, nelems)) {

        // Assert before and after expectations.
        assert(type == nc_attribute);

        while (attrs.size() != nelems)
            attrs.push_back(read_attr());
    }
}

void cdf_reader::read_var_header(var & theVar, dim_vector const & dims, bool useClassic) {

    read_named(theVar);

    auto nelems = get_reversed_byte_order(read<int32_t>(*pIS));
    //TODO: may want to pre-allocate these as with variable/data
    theVar.dimids.clear();

    while (theVar.dimids.size() != nelems)
        theVar.dimids.push_back(get_reversed_byte_order(read<int32_t>(*pIS)));

    read_attrs(theVar.vattrs);

    theVar.type = to_nc_type(get_reversed_byte_order(read<int32_t>(*pIS)));

    //TODO: either redundant and/or obsolete, but still support if possible... maybe with try/catch to protect calculations
    theVar.vsize = get_reversed_byte_order(read<int32_t>(*pIS));

    /* TODO: TBD: may want to refactor sizeof calculators for verification purposes. This is providing the calculation
    is correct, which I beleive it is now, and would be a good cross-check, maintaining validity of the file format(s)
    on the way in/out of processing. */

    /* TODO: TBD: reading "x64" files may be an issue: seems it may be fixed ...
    http://connect.microsoft.com/VisualStudio/feedback/details/627639/std-fstream-use-32-bit-int-as-pos-type-even-on-x64-platform */

    if (useClassic)
        theVar.offset.begin = get_reversed_byte_order(read<int32_t>(*pIS));
    else
        //TODO: might need to do this one a bit differently (?)
        theVar.offset.begin64 = get_reversed_byte_order(read<int64_t>(*pIS));
}

void cdf_reader::read_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic) {

    offset_t current = { 0 };

    nc_type type;
    int32_t nelems;

    //TODO: may want to pre-allocate these as with variable/data
    vars.clear();

    if (try_read_typed_array_prefix(type, nelems)) {

        // Assert before and after expectations.
        assert(type == nc_variable);

        vars = var_vector(nelems);

        for (auto i = 0; i < nelems; i++) {

            auto & aVar = vars[i];

            read_var_header(aVar, dims, useClassic);

            //TODO: TBD: is this a dangerous assumption to be making concerning record/non-record data, position in the vector, etc?

            // Verify that the fp values are properly aligned.
            if (useClassic) {
                assert(aVar.offset.begin > current.begin);
                current.begin = aVar.offset.begin;
            }
            else {
                assert(aVar.offset.begin64 > current.begin64);
                current.begin64 = aVar.offset.begin64;
            }
        }
    }
}

void cdf_reader::read_var_data(var & v, dim_vector const & dims, bool useClassic) {

    const auto way = std::ios::beg;

    if (useClassic)
        pIS->seekg(v.offset.begin, way);
    else
        pIS->seekg(v.offset.begin64, way);

    const auto type = v.get_type();
    const auto nelems = v.vsize / get_primitive_value_size(type);

    v.data = std::vector<value>(nelems);

    // A little more efficient than creating on the stack and returning, copying, etc.
    for (auto i = 0; i < nelems; i++)
        assert(try_read_primitive(v.data[i], type));
}

void cdf_reader::read_vars_data(var_vector & vars, dim_vector const & dims, bool useClassic) {

    // Read the non-record data in header-specified order.
    for (auto & v : vars)
        if (!v.is_record(dims))
            read_var_data(v, dims, useClassic);

    // Then read the record data. Should be only one, but may occur in any position AFAIK.
    for (auto & v : vars)
        if (v.is_record(dims))
            read_var_data(v, dims, useClassic);
}

cdf_reader & cdf_reader::read_cdf(netcdf & cdf) {

    read_magic(cdf.magic);

    //TODO: pick this one up here: look up concerning the BNF format what to expect ...
    cdf.numrecs = get_reversed_byte_order(read<int32_t>(*pIS));

    read_dims(cdf.dims);

    read_attrs(cdf.gattrs);

    read_vars_header(cdf.vars, cdf.dims, cdf.magic.is_classic());

    read_vars_data(cdf.vars, cdf.dims, cdf.magic.is_classic());

    return *this;
}

cdf_reader & operator>>(cdf_reader & reader, netcdf & cdf) {
    return reader.read_cdf(cdf);
}
