#include "netcdf_output.h"
#include "algorithm.hpp"

#include <functional>
#include <vector>
#include <cassert>

///////////////////////////////////////////////////////////////////////////////

cdf_writer::cdf_writer(std::ostream * pOS, bool reverse_byte_order)
    : pOS(pOS), reverse_byte_order(reverse_byte_order) {
}

///////////////////////////////////////////////////////////////////////////////

int32_t __sizeof() {
    return 0;
}

int32_t get_nelem() {
    return 0;
}

typedef decltype(__sizeof()) sizeof_type;

typedef decltype(get_nelem()) nelem_type;

///////////////////////////////////////////////////////////////////////////////

sizeof_type __sizeof(magic const & magic) {

    // magic   := 'C' 'D' 'F'               VERSION_BYTE
    auto result = sizeof(magic::key_type) + sizeof(magic.version);
    return result;
}

sizeof_type __sizeof(named const & named) {

    // name    :=        nelems        [chars]
    auto result = sizeof(nelem_type) + named.name.length();
    return pad_width(result);
}

sizeof_type __sizeof(dim const & dim) {

    // dim     := name
    auto result = __sizeof(reinterpret_cast<named const &>(dim))
        //       <32-bit signed integer, Bigendian, two's complement, with non-negative value>
        + sizeof(dim.dim_length);
    return result;
}

sizeof_type __sizeof(dim_vector const & dims) {

    // dim_array :=      ABSENT | NC_DIMENSION nelems
    auto result = sizeof(nc_type) + sizeof(nelem_type)
        //                            [dim ...]
        + aggregate<dim, sizeof_type>(dims, 0,
        [&](sizeof_type const & g, dim const & x) { return g + __sizeof(x); });
    return result;
}

sizeof_type __sizeof(value const & value, nc_type const & type) {

    // values  := [bytes] | [chars] | [shorts] | [ints] | [floats] | [doubles]

    /* This one is a bit different in that we either aggregate
    several primitive values or a single text (i.e. several chars) value. */

    if (is_primitive_type(type))
        return get_primitive_value_size(type);

    /* Remember that the nc_type is appart from the value(s)
    themselves. With char being a bit of a special case. */

    if (type == nc_char)
        return pad_width(value.text.length());

    throw std::exception("unsupported type");
}

sizeof_type __sizeof(attr const & attr) {

    const auto type = attr.get_type();

    // attr             := name
    auto result = __sizeof(reinterpret_cast<named const &>(attr))
        //       nc_type  nelems
        + sizeof(type) + sizeof(nelem_type);

    /*  The model says we either aggregage a single element vector
    (chars, or string, text), or a vector of primitives */
    //                                                      [values]
    auto sizeof_attr_values = aggregate<value, sizeof_type>(attr.values, 0,
        [&](sizeof_type const & g, value const & x) { return g + __sizeof(x, type); });

    // Pad the values out to the nearest width.
    result += pad_width(sizeof_attr_values);

    return result;
}

sizeof_type __sizeof(attr_vector const & attrs) {

    // att_array  :=  ABSENT | NC_ATTRIBUTE nelems
    return sizeof(nc_type) + sizeof(nelem_type)
        //                             [attr ...]
        + aggregate<attr, sizeof_type>(attrs, 0,
        [&](sizeof_type const & g, attr const & x) { return g + __sizeof(x); });
}

sizeof_type __sizeof_header(var const & var, dim_vector const & dims, bool useClassic) {

    // var     :=          name                                           nelems
    auto result = __sizeof(reinterpret_cast<named const &>(var)) + sizeof(nelem_type);

    //                                                       [dimid ...]
    const auto sizeof_dims = aggregate<int32_t, sizeof_type>(var.dimids, 0,
        [&](sizeof_type const & g, int32_t const & x) { return g + sizeof(dims[x].dim_length); });

    //                 vatt_array           nc_type            vsize
    result += __sizeof(var.vattrs) + sizeof(var.type) + sizeof(var.vsize);

    //                     OFFSET := <INT with non-negative value> (classic) | <INT64 with non - negative value> (64-bit)
    const auto sizeof_begin_offset = useClassic ? sizeof(var.offset.begin) : sizeof(var.offset.begin64);

    return result + sizeof_dims + sizeof_begin_offset;
}

sizeof_type __sizeof_header(var_vector const & vars, dim_vector const & dims, bool useClassic) {

    // var_array  :=  ABSENT | NC_VARIABLE nelems
    return sizeof(nc_type) + sizeof(nelem_type)
        //                            [var ...]
        + aggregate<var, sizeof_type>(vars, 0,
        [&](sizeof_type const & g, var const & x) { return g + __sizeof_header(x, dims, useClassic); });
}

sizeof_type __sizeof_header(netcdf const & cdf) {

    // header    := magic
    return __sizeof(cdf.magic)
        //       numrecs
        + sizeof(cdf.numrecs)
        //         dim_array
        + __sizeof(cdf.dims)
        //         gatt_array
        + __sizeof(cdf.gattrs)
        //                var_array
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

///////////////////////////////////////////////////////////////////////////////

void cdf_writer::prepare_var_array(netcdf & cdf) {

    offset_t current = { 0LL };

    /* Python netcdf is using the actual file pOSition to inform the begin value
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
        *pOS << k;

    write(*pOS, magic.version);
}

void cdf_writer::write_text(std::string const & str) {

    int32_t written = str.length();

    //Starting with the length...
    write(*pOS, get_reversed_byte_order(static_cast<int32_t>(str.length())));

    // Not including the terminating null char as far as I know.
    for (auto const & c : str)
        write(*pOS, c);

    while (try_pad_width(written))
        write(*pOS, static_cast<uint8_t>(0x0));
}

void cdf_writer::write_named(named const & n) {
    write_text(n.name);
}

void cdf_writer::write_dim(dim const & dim) {

    write_named(dim);

    write(*pOS, get_reversed_byte_order(dim.dim_length));
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
        write(*pOS, v.primitive.b);
        break;

    case nc_type::nc_short:
        write(*pOS, get_reversed_byte_order(v.primitive.s));
        break;

    case nc_type::nc_int:
        write(*pOS, get_reversed_byte_order(v.primitive.i));
        break;

    case nc_type::nc_float:
        write(*pOS, v.primitive.f);
        break;

    case nc_type::nc_double:
        write(*pOS, v.primitive.d);
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

        write(*pOS, get_reversed_byte_order(type));

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
    write(*pOS, get_reversed_byte_order(dimids_nelems));

    for (const auto & dimid : v.dimids)
        write(*pOS, get_reversed_byte_order(dimid));

    write_attrs(v.vattrs);
    
    write(*pOS, get_reversed_byte_order(v.get_type()));

    // Assume that the vsize has already been recalculated.
    write(*pOS, get_reversed_byte_order(v.vsize));

    if (useClassic)
        write(*pOS, get_reversed_byte_order(v.offset.begin));
    else 
        write(*pOS, get_reversed_byte_order(v.offset.begin64));
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
        write(*pOS, static_cast<uint8_t>(0x0));
}

void cdf_writer::write_vars_data(var_vector const & vars, dim_vector const & dims, bool useClassic) {

    // Read the non-record data in header-specified order.
    for (auto const & v : vars)
        if (!v.is_record(dims))
            write_var_data(v, dims, useClassic);

    // Then read the record data. Should be only one, but may occur in any pOSition AFAIK.
    for (auto const & v : vars)
        if (v.is_record(dims))
            write_var_data(v, dims, useClassic);
}

cdf_writer & cdf_writer::operator<<(netcdf & cdf) {

    prepare_var_array(cdf);

    write_magic(cdf.magic);

    write(*pOS, get_reversed_byte_order(cdf.numrecs));

    write_dims(cdf.dims);

    write_attrs(cdf.gattrs);

    write_vars_header(cdf.vars, cdf.dims, cdf.magic.is_classic());

    write_vars_data(cdf.vars, cdf.dims, cdf.magic.is_classic());

    return *this;
}
