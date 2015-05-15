#include "netcdf_output.h"
#include "algorithm.hpp"

#include <functional>
#include <vector>
#include <cassert>

cdf_writer::cdf_writer(std::ostream & os, bool reverse_byte_order)
    : os(os), reverse_byte_order(reverse_byte_order) {
}

int32_t __sizeof() {
    return 0;
}

int32_t get_nelem() {
    return 0;
}

typedef decltype(__sizeof()) sizeof_type;

typedef decltype(get_nelem()) nelem_type;

sizeof_type __sizeof(magic const & magic) {
    return sizeof(magic::key_type)
        + sizeof(magic.version);
}

sizeof_type __sizeof(named const & named) {
    auto result = sizeof(nelem_type) + named.name.length();
    return pad_width(result);
}

sizeof_type __sizeof(dim const & dim) {
    return __sizeof(reinterpret_cast<named const &>(dim)) + sizeof(dim.dim_length);
}

sizeof_type __sizeof(dim_vector const & dims) {
    return sizeof(nc_type) + sizeof(nelem_type)
        + aggregate<dim, sizeof_type>(dims, 0,
        [&](sizeof_type const & g, dim const & x) { return g + __sizeof(x); });
}

sizeof_type __sizeof(value const & value, nc_type const & type) {

    if (is_primitive_type(type))
        return get_primitive_value_size(type);

    /* Remember that the nc_type is appart from the value(s) themselves.
    With char being a bit of a special case. */
    if (type == nc_char)
        return pad_width(value.text.length());

    throw std::exception("unsupported type");
}

sizeof_type __sizeof(attr const & attr) {

    const auto type = attr.get_type();

    auto sizeof_attr_values = aggregate<value, sizeof_type>(attr.values, 0,
        [&](sizeof_type const & g, value const & x) { return g + __sizeof(x, type); });

    //TODO: TBD: should probably be padded, but we won't worry about this one.
    if (is_primitive_type(type))
        sizeof_attr_values = pad_width(sizeof_attr_values);

    // Now we should pick up the number of elements size.
    return __sizeof(reinterpret_cast<named const &>(attr))
        + sizeof(type) + sizeof(nelem_type) + sizeof_attr_values;
}

sizeof_type __sizeof(attr_vector const & attrs) {
    return sizeof(nc_type) + sizeof(nelem_type)
        + aggregate<attr, sizeof_type>(attrs, 0,
        [&](sizeof_type const & g, attr const & x) { return g + __sizeof(x); });
}

sizeof_type __sizeof_header(var const & var, dim_vector const & dims, bool useClassic) {

    const auto sizeof_dims = aggregate<int32_t, sizeof_type>(var.dimids, 0,
        [&](sizeof_type const & g, int32_t const & x) { return g + sizeof(dims[x].dim_length); });

    const auto sizeof_var_begin = useClassic ? sizeof(var.offset.begin) : sizeof(var.offset.begin64);

    return __sizeof(reinterpret_cast<named const &>(var))
        + sizeof(nelem_type) + sizeof_dims
        + __sizeof(var.vattrs) + sizeof(var.type)
        + sizeof(var.vsize) + sizeof_var_begin;
}

sizeof_type __sizeof_header(var_vector const & vars, dim_vector const & dims, bool useClassic) {
    return sizeof(nc_type) + sizeof(nelem_type)
        + aggregate<var, sizeof_type>(vars, 0,
        [&](sizeof_type const & g, var const & x) { return g + __sizeof_header(x, dims, useClassic); });
}

sizeof_type __sizeof_header(netcdf const & cdf) {
    return __sizeof(cdf.magic) + sizeof(cdf.numrecs)
        + __sizeof(cdf.dims) + __sizeof(cdf.gattrs)
        + __sizeof_header(cdf.vars, cdf.dims, cdf.magic.is_classic());
}

typedef decltype(var::vsize) vsize_type;

vsize_type __sizeof_data(var const & var, dim_vector const & dims, bool useClassic) {

    const auto type = var.get_type();

    vsize_type result = get_primitive_value_size(type);

    // http://cucis.ece.northwestern.edu/projects/PnetCDF/CDF-5.html#NOTEVSIZE5
    // http://cucis.ece.northwestern.edu/projects/PnetCDF/doc/pnetcdf-c/CDF_002d2-file-format-specification.html#NOTEVSIZE
    if (!var.is_record(dims)) {
        result *= var.data.size();
    }
    else {

        // Omitting record dimension.
        auto __normalized = [](decltype(dim::dim_length) len) {return len ? len : 1; };

        result *= aggregate<int32_t, vsize_type>(var.dimids, 1,
            [&](vsize_type const & g, int32_t const & x) { return g * __normalized(dims[x].dim_length); });
    }

    return pad_width(result);
}

void cdf_writer::prepare_var_array(netcdf & cdf) {

    offset_t current = { 0LL };

    /* Python netcdf is using the actual file position to inform the begin value
    then packing that. that's an interesting way of doing it...
    http://afni.nimh.nih.gov/pub/dist/src/pkundu/meica.libs/nibabel/externals/netcdf.py */

    auto & vars = cdf.vars;

    const auto & dims = cdf.dims;
    const auto useClassic = cdf.magic.is_classic();

    // All of the calculations depend upon the vsize being calculated regardless whether record.
    for (auto & v : vars)
        v.vsize = __sizeof_data(v, dims, useClassic);

    auto is_first = true;
    const auto sizeof_header = __sizeof_header(cdf);

    // Calculate the begin offsets for non-record data.
    for (auto i = 0U, j = 0U; i < vars.size(); i++) {

        if (vars[i].is_record(dims)) continue;

        // For all non-record data.
        if (is_first) {

            if (useClassic)
                vars[i].offset.begin = sizeof_header;
            else
                vars[i].offset.begin64 = sizeof_header;

            is_first = false;
        }
        else {

            if (useClassic)
                vars[i].offset.begin = vars[j].offset.begin + vars[j].vsize;
            else
                vars[i].offset.begin64 = vars[j].offset.begin64 + vars[j].vsize;;
        }

        // Keep track of the last index j.
        j = i;
    }
}

void cdf_writer::write_magic(magic const & magic) {

    for (auto k : magic.key)
        os << k;

    write(os, magic.version);
}

void cdf_writer::write_text(std::string const & str) {

    int32_t written = str.length();

    //Starting with the length...
    write(os, get_reversed_byte_order(static_cast<int32_t>(str.length())));

    // Not including the terminating null char as far as I know.
    for (auto const & c : str)
        write(os, c);

    while (try_pad_width(written))
        write(os, static_cast<uint8_t>(0x0));
}

void cdf_writer::write_named(named const & n) {
    write_text(n.name);
}

void cdf_writer::write_dim(dim const & dim) {

    write_named(dim);

    write(os, get_reversed_byte_order(dim.dim_length));
}

void cdf_writer::write_dims(dim_vector const & dims) {

    write_typed_array_prefix(dims, nc_dimension);

    // Followed by the dims themselves.
    for (auto const & dim : dims)
        write_dim(dim);
}

void cdf_writer::write_primitive(value const & v, nc_type const & type) {

    // This being a special use case, usually.
    assert(type != nc_char);

    switch (type) {

    default:
        throw std::exception("unsupported nc_type");

    case nc_type::nc_byte:
        write(os, v.primitive.b);
        break;

    case nc_type::nc_short:
        write(os, get_reversed_byte_order(v.primitive.s));
        break;

    case nc_type::nc_int:
        write(os, get_reversed_byte_order(v.primitive.i));
        break;

    case nc_type::nc_float:
        write(os, get_reversed_byte_order(v.primitive.f));
        break;

    case nc_type::nc_double:
        write(os, get_reversed_byte_order(v.primitive.d));
        break;
    }
}

void cdf_writer::write_attr(attr const & attr) {

    write_named(attr);

    /* TODO: might see about reorganizing the model just a bit: "types" (so-called) really belong
    close to the value (union, plus other parts, like text) for best language level results. */

    // This is kind of a preculiar use case.
    const auto type = attr.get_type();

    // Handles the text use case especially, followed by the primitive types.
    if (type == nc_char) {

        assert(attr.values.size() == 1);

        write(os, get_reversed_byte_order(type));

        write_text(attr.values.front().text);
    }
    else {

        write_typed_array_prefix(attr.values, type);

        for (auto const & v : attr.values)
            write_primitive(v, type);
    }
}

void cdf_writer::write_attrs(attr_vector const & attrs) {

    write_typed_array_prefix(attrs, nc_attribute);

    for (auto const & attr : attrs)
        write_attr(attr);
}

void cdf_writer::write_var_header(var & v, dim_vector const & dims, bool useClassic) {

    write_named(v);

    int32_t dimids_nelems = static_cast<int32_t>(v.dimids.size());
    write(os, get_reversed_byte_order(dimids_nelems));

    for (const auto & dimid : v.dimids)
        write(os, get_reversed_byte_order(dimid));

    write_attrs(v.vattrs);
    
    write(os, get_reversed_byte_order(v.get_type()));

    // Assume that the vsize has already been recalculated.
    write(os, get_reversed_byte_order(v.vsize));

    if (useClassic)
        write(os, get_reversed_byte_order(v.offset.begin));
    else 
        write(os, get_reversed_byte_order(v.offset.begin64));
}

void cdf_writer::write_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic) {

    /* TODO: seriously consider whether the struct/container/to-vector pattern isn't adding too much complexity to the overall model,
    especially considering ctor/dtor times involved, it's a lot of time and overhead that doesn't need to be there ? */
    write_typed_array_prefix(vars, nc_variable);

    for (auto & v : vars)
        write_var_header(v, dims, useClassic);
}

void cdf_writer::write_var_data(var const & v, dim_vector const & dims, bool useClassic) {

    auto type = v.get_type();
    //auto nelems = v.get_expected_nelems();

    // A little more efficient than creating on the stack and returning, copying, etc.
    for (auto const & value : v.data)
        write_primitive(value, type);

    // Here we do need to take variable data padding into consideration.
    int32_t writtenCount = v.data.size() * get_primitive_value_size(type);

    while (try_pad_width(writtenCount))
        write(os, static_cast<uint8_t>(0x0));
}

void cdf_writer::write_vars_data(var_vector const & vars, dim_vector const & dims, bool useClassic) {

    // Read the non-record data in header-specified order.
    for (auto const & v : vars)
        if (!v.is_record(dims))
            write_var_data(v, dims, useClassic);

    // Then read the record data. Should be only one, but may occur in any position AFAIK.
    for (auto const & v : vars)
        if (v.is_record(dims))
            write_var_data(v, dims, useClassic);
}

cdf_writer & cdf_writer::operator<<(netcdf & cdf) {

    prepare_var_array(cdf);

    write_magic(cdf.magic);

    write(os, get_reversed_byte_order(cdf.numrecs));

    write_dims(cdf.dims);

    write_attrs(cdf.gattrs);

    write_vars_header(cdf.vars, cdf.dims, cdf.magic.is_classic());

    write_vars_data(cdf.vars, cdf.dims, cdf.magic.is_classic());

    return *this;
}
