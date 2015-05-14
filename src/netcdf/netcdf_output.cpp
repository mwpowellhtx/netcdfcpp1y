#include "netcdf_output.h"

#include <functional>
#include <vector>
#include <cassert>

template<typename _Ty>
std::ostream & write(std::ostream & os, _Ty const & field, bool reverseByteOrder = false) {
    //TODO: consider how to reverse byte order when necessary ...
    os.write(reinterpret_cast<const char*>(&field), sizeof(field));
    return os;
}

cdf_writer::cdf_writer(std::ostream & os, bool reverse_byte_order)
    : os(os), reverse_byte_order(reverse_byte_order) {
}

void cdf_writer::prepare_var_array(netcdf & cdf) {

    offset_t current = { 0 };

    auto useClassic = cdf.magic.is_classic();

    for (auto i = 0U; i < cdf.var_array.vars.size(); i++) {

        if (!i) {

            auto sizeof_header = cdf.get_sizeof_header();

            if (useClassic)
                cdf.var_array.vars[i].offset.begin = sizeof_header;
            else
                cdf.var_array.vars[i].offset.begin64 = sizeof_header;
        }
        else {

            auto sizeof_data = cdf.var_array.vars[i - 1].get_sizeof_data();

            if (useClassic)
                cdf.var_array.vars[i].offset.begin
                = cdf.var_array.vars[i - 1].offset.begin + sizeof_data;
            else
                cdf.var_array.vars[i].offset.begin64
                = cdf.var_array.vars[i - 1].offset.begin64 + sizeof_data;
        }
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

void cdf_writer::write_typed_array_prefix(typed_array const & arr) {

    // Really transparent way of accomplishing this...
    write(os, get_reversed_byte_order(arr.get_type()));

    write(os, get_reversed_byte_order(arr.get_nelems()));
}

void cdf_writer::write_dim(dim const & dim) {

    write_named(dim);

    write(os, get_reversed_byte_order(dim.dim_length));
}

void cdf_writer::write_dim_array(dim_array const & arr) {

    write_typed_array_prefix(arr);

    // Followed by the dims themselves.
    for (auto const & dim : arr.dims)
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
    auto type = attr.get_type();

    // Handles the text use case especially, followed by the primitive types.
    if (type == nc_char) {

        assert(attr.values.size() == 1);

        write(os, get_reversed_byte_order(type));

        write_text(attr.values.front().text);
    }
    else {

        write_typed_array_prefix(attr);

        for (auto const & v : attr.values)
            write_primitive(v, type);
    }
}

void cdf_writer::write_att_array(att_array const & arr) {

    write_typed_array_prefix(arr);

    for (auto const & attr : arr.atts)
        write_attr(attr);
}

void cdf_writer::write_var_header(var & v, dim_array const & dims, bool useClassic) {

    write_named(v);

    write(os, get_reversed_byte_order(v.get_nelems()));

    for (const auto & dimid : v.dimids)
        write(os, get_reversed_byte_order(dimid));

    write_att_array(v.vatt_array);
    
    write(os, get_reversed_byte_order(v.get_type()));

    //TODO: may consider whether a "prepare" step is necessary just prior to, in order to preserve const-ness integrity of overall operation
    // Must recalculate the size here.
    v.vsize = v.get_calculated_size(dims);
    write(os, get_reversed_byte_order(v.vsize));

    if (useClassic)
        write(os, get_reversed_byte_order(v.offset.begin));
    else 
        write(os, get_reversed_byte_order(v.offset.begin64));
}

void cdf_writer::write_var_array_header(var_array & arr, dim_array const & dims, bool useClassic) {

    /* TODO: seriously consider whether the struct/container/to-vector pattern isn't adding too much complexity to the overall model,
    especially considering ctor/dtor times involved, it's a lot of time and overhead that doesn't need to be there ? */
    write(os, get_reversed_byte_order(arr.get_type()));

    write(os, get_reversed_byte_order(arr.get_nelems()));

    for (auto & v : arr.vars)
        write_var_header(v, dims, useClassic);
}

void cdf_writer::write_var_data(var const & v, dim_array const & dims, bool useClassic) {

    auto type = v.get_type();
    auto nelems = v.get_expected_nelems();

    // A little more efficient than creating on the stack and returning, copying, etc.
    for (auto i = 0; i < nelems; i++)
        write_primitive(v.data[i], type);

    // Here we do need to take variable data padding into consideration.
    auto writtenCount = v.get_calculated_size_raw(dims);

    while (try_pad_width(writtenCount))
        write(os, static_cast<uint8_t>(0x0));
}

void cdf_writer::write_var_array_data(var_array const & arr, dim_array const & dims, bool useClassic) {

    // Read the non-record data in header-specified order.
    for (auto const & v : arr.vars)
        if (!v.is_record(dims))
            write_var_data(v, dims, useClassic);

    // Then read the record data. Should be only one, but may occur in any position AFAIK.
    for (auto const & v : arr.vars)
        if (v.is_record(dims))
            write_var_data(v, dims, useClassic);
}

cdf_writer & cdf_writer::operator<<(netcdf & cdf) {

    prepare_var_array(cdf);

    write_magic(cdf.magic);

    write(os, get_reversed_byte_order(cdf.numrecs));

    write_dim_array(cdf.dim_array);

    write_att_array(cdf.gatt_array);

    write_var_array_header(cdf.var_array, cdf.dim_array, cdf.magic.is_classic());

    write_var_array_data(cdf.var_array, cdf.dim_array, cdf.magic.is_classic());

    return *this;
}
