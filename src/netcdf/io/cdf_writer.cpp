#include "cdf_writer.h"

#include <functional>
#include <numeric>
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

sizeof_type __sizeof(magic const & theMagic) {

    // magic   := 'C' 'D' 'F'               VERSION_BYTE
    auto result = sizeof(magic::key_type) + sizeof(theMagic.version);
    return result;
}

sizeof_type __sizeof(named const & theNamed) {

    // name    :=        nelems        [chars]
    auto result = sizeof(nelem_type) + theNamed.name.length();
    return pad_width(result);
}

sizeof_type __sizeof(dim const & theDim) {

    // dim     := name
    auto result = __sizeof(reinterpret_cast<named const &>(theDim))
        //       <32-bit signed integer, Bigendian, two's complement, with non-negative value>
        + sizeof(theDim.dim_length);
    return result;
}

sizeof_type __sizeof(dim_vector const & dims) {

    // dim_array :=      ABSENT | NC_DIMENSION nelems
    auto result = sizeof(nc_type) + sizeof(nelem_type)
        //               [dim ...]
        + std::accumulate(dims.begin(), dims.end(), static_cast<int32_t>(0),
        [&](int32_t const & g, dim const & x) { return g + __sizeof(x); });
    return result;
}

sizeof_type __sizeof(value const & theValue, nc_type const & type) {

    // values  := [bytes] | [chars] | [shorts] | [ints] | [floats] | [doubles]

    /* This one is a bit different in that we either aggregate
    several primitive values or a single text (i.e. several chars) value. */

    if (is_primitive_type(type))
        return get_primitive_value_size(type);

    /* Remember that the nc_type is appart from the value(s)
    themselves. With char being a bit of a special case. */

    if (type == nc_char)
        return pad_width(theValue.text.length());

    throw std::exception("unsupported type");
}

sizeof_type __sizeof(attr const & theAttr) {

    const auto type = theAttr.get_type();

    // attr             := name
    auto result = __sizeof(reinterpret_cast<named const &>(theAttr))
        //       nc_type  nelems
        + sizeof(type) + sizeof(nelem_type);

    /*  The model says we either aggregage a single element vector
    (chars, or string, text), or a vector of primitives */
    //                                       [values]
    auto sizeof_attr_values = std::accumulate(theAttr.values.begin(), theAttr.values.end(),
        static_cast<sizeof_type>(0),
        [&](sizeof_type const & g, value const & x) { return g + __sizeof(x, type); });

    // Pad the values out to the nearest width.
    result += pad_width(sizeof_attr_values);

    return result;
}

sizeof_type __sizeof(attr_vector const & attrs) {

    // att_array  :=  ABSENT | NC_ATTRIBUTE nelems
    return sizeof(nc_type) + sizeof(nelem_type)
        //               [attr ...]
        + std::accumulate(attrs.begin(), attrs.end(), static_cast<sizeof_type>(0),
        [&](sizeof_type const & g, attr const & x) { return g + __sizeof(x); });
}

sizeof_type __sizeof_header(var const & theVar, dim_vector const & dims, bool useClassic) {

    // var     :=          name                                              nelems
    auto result = __sizeof(reinterpret_cast<named const &>(theVar)) + sizeof(nelem_type);

    //                                      [dimid ...]
    const auto sizeof_dims = std::accumulate(theVar.dimids.begin(), theVar.dimids.end(),
        static_cast<sizeof_type>(0),
        [&](sizeof_type const & g, int32_t const & x) { return g + sizeof(dims[x].dim_length); });

    //                 vatt_array             nc_type               vsize
    result += __sizeof(theVar.attrs) + sizeof(theVar.type) + sizeof(theVar.vsize);

    //                     OFFSET := <INT with non-negative value> (classic) | <INT64 with non - negative value> (64-bit)
    const auto sizeof_begin_offset = useClassic ? sizeof(theVar.offset.begin) : sizeof(theVar.offset.begin64);

    return result + sizeof_dims + sizeof_begin_offset;
}

sizeof_type __sizeof_header(var_vector const & vars, dim_vector const & dims, bool useClassic) {

    // var_array  :=  ABSENT | NC_VARIABLE nelems
    return sizeof(nc_type) + sizeof(nelem_type)
        //               [var ...]
        + std::accumulate(vars.begin(), vars.end(), static_cast<sizeof_type>(0),
        [&](sizeof_type const & g, var const & x) { return g + __sizeof_header(x, dims, useClassic); });
}

sizeof_type __sizeof_header(netcdf const & theCdf) {

    // header    := magic
    return __sizeof(theCdf.magic)
        //       numrecs
        + sizeof(theCdf.numrecs)
        //         dim_array
        + __sizeof(theCdf.dims)
        //         gatt_array
        + __sizeof(theCdf.attrs)
        //                var_array
        + __sizeof_header(theCdf.vars, theCdf.dims, theCdf.magic.is_classic());
}

typedef decltype(var::vsize) vsize_type;

vsize_type __sizeof_data(var const & theVar, dim_vector const & dims, bool useClassic) {

    const auto type = theVar.get_type();

    vsize_type result = get_primitive_value_size(type);

    // http://cucis.ece.northwestern.edu/projects/PnetCDF/CDF-5.html#NOTEVSIZE5
    // http://cucis.ece.northwestern.edu/projects/PnetCDF/doc/pnetcdf-c/CDF_002d2-file-format-specification.html#NOTEVSIZE
    if (!theVar.is_record(dims)) {
        result *= theVar.values.size();
    }
    else {

        // Omitting record dimension.
        auto __normalized = [](decltype(dim::dim_length) len) {return len ? len : 1; };

        result *= std::accumulate(theVar.dimids.begin(), theVar.dimids.end(), static_cast<vsize_type>(1),
            [&](vsize_type const & g, int32_t const & x) { return g * __normalized(dims[x].dim_length); });
    }

    return pad_width(result);
}

///////////////////////////////////////////////////////////////////////////////

void cdf_writer::prepare_var_array(netcdf & theCdf) {

    /* Python netcdf is using the actual file pOSition to inform the begin value
    then packing that. that's an interesting way of doing it...
    http://afni.nimh.nih.gov/pub/dist/src/pkundu/meica.libs/nibabel/externals/netcdf.py */

    auto & theVars = theCdf.vars;

    const auto & theDims = theCdf.dims;
    const auto useClassic = theCdf.magic.is_classic();

    // All of the calculations depend upon the vsize being calculated regardless whether record.
    for (auto & aVar : theVars)
        aVar.vsize = __sizeof_data(aVar, theDims, useClassic);

    // This is a little book keeping, that helps the subsequent operations flow much more smoothly.
    std::vector<var_vector::iterator> record_its, its;

    // TODO: TBD: may need/want to rearrange the vars according to record/non-record...
    // TODO: using the variables, there are how many record data? that can't be right ...
    for (auto it = theVars.begin(); it != theVars.end(); it++) {
        if (it->is_record(theDims))
            record_its.push_back(it);
        else
            its.push_back(it);
    }

    // Concatenate the record vars to the end of the non-record vars.
    its.insert(its.end(), record_its.begin(), record_its.end());

    // Initialize the current with the size of the header.
    const auto sizeof_header = __sizeof_header(theCdf);
    offset_t current = { { sizeof_header } };

    // Calculate the begin offsets for non-record data.
    for (auto bm = its.begin(); bm != its.end(); bm++) {

        // Drill through the bookmark to the true inner iterator.
        auto var_it = *bm;

        // Just assign the current offset and be on with it.
        var_it->offset = current;

        // Then simply tally the size with the current offset.
        if (useClassic)
            current.begin += var_it->vsize;
        else
            current.begin64 += var_it->vsize;
    }

    // TODO: TBD: then do something with the record_it ...
}

void cdf_writer::write_magic(magic const & theMagic) {

    for (auto k : theMagic.key)
        *pOS << k;

    write(*pOS, theMagic.version);
}

void cdf_writer::write_text(std::string const & theStr) {

    int32_t writtenCount = theStr.length();

    //Starting with the length...
    write(*pOS, get_reversed_byte_order(static_cast<int32_t>(writtenCount)));

    // Not including the terminating null char as far as I know.
    for (auto const & c : theStr)
        write(*pOS, c);

    while (try_pad_width(writtenCount))
        write(*pOS, static_cast<int8_t>(0x0));
}

void cdf_writer::write_named(named const & theNamed) {
    write_text(theNamed.name);
}

void cdf_writer::write_dim(dim const & theDim) {

    write_named(theDim);

    write(*pOS, get_reversed_byte_order(theDim.dim_length));
}

void cdf_writer::write_dims(dim_vector const & dims) {

    write_typed_array_prefix(dims, nc_dimension);

    // Followed by the dims themselves.
    for (auto const & dim : dims)
        write_dim(dim);
}

void cdf_writer::write_primitive(value const & theValue, nc_type const & type) {

    // This being a special use case, usually.
    assert(type != nc_char);

    switch (type) {

    default:
        throw std::exception("unsupported nc_type");

    case nc_byte:
        write(*pOS, theValue.primitive.b);
        break;

    case nc_short:
        write(*pOS, get_reversed_byte_order(theValue.primitive.s));
        break;

    case nc_int:
        write(*pOS, get_reversed_byte_order(theValue.primitive.i));
        break;

    case nc_float:
        write(*pOS, theValue.primitive.f);
        break;

    case nc_double:
        write(*pOS, theValue.primitive.d);
        break;
    }
}

void cdf_writer::write_attr(attr const & theAttr) {

    write_named(theAttr);

    /* TODO: might see about reorganizing the model just a bit: "types" (so-called) really belong
    close to the value (union, plus other parts, like text) for best language level results. */

    // This is kind of a preculiar use case.
    const auto type = theAttr.get_type();

    // Handles the text use case especially, followed by the primitive types.
    if (type == nc_char) {

        assert(theAttr.values.size() == 1);

        write(*pOS, get_reversed_byte_order(type));

        write_text(theAttr.values.front().text);
    }
    else {

        write_typed_array_prefix(theAttr.values, type);

        for (auto const & v : theAttr.values)
            write_primitive(v, type);
    }
}

void cdf_writer::write_attrs(attr_vector const & attrs) {

    write_typed_array_prefix(attrs, nc_attribute);

    for (auto const & attr : attrs)
        write_attr(attr);
}

void cdf_writer::write_var_header(var & theVar, dim_vector const & dims, bool useClassic) {

    write_named(theVar);

    int32_t dimids_nelems = static_cast<int32_t>(theVar.dimids.size());
    write(*pOS, get_reversed_byte_order(dimids_nelems));

    for (const auto & dimid : theVar.dimids)
        write(*pOS, get_reversed_byte_order(dimid));

    write_attrs(theVar.attrs);
    
    write(*pOS, get_reversed_byte_order(theVar.get_type()));

    // Assume that the vsize has already been recalculated.
    write(*pOS, get_reversed_byte_order(theVar.vsize));

    if (useClassic)
        write(*pOS, get_reversed_byte_order(theVar.offset.begin));
    else 
        write(*pOS, get_reversed_byte_order(theVar.offset.begin64));
}

void cdf_writer::write_vars_header(var_vector & vars, dim_vector const & dims, bool useClassic) {

    /* TODO: seriously consider whether the struct/container/to-vector pattern isn't adding too much complexity to the overall model,
    especially considering ctor/dtor times involved, it's a lot of time and overhead that doesn't need to be there ? */
    write_typed_array_prefix(vars, nc_variable);

    for (auto & v : vars)
        write_var_header(v, dims, useClassic);
}

void cdf_writer::write_var_data(var const & theVar, dim_vector const & dims, bool useClassic) {

    auto type = theVar.get_type();
    //auto nelems = v.get_expected_nelems();

    // A little more efficient than creating on the stack and returning, copying, etc.
    for (auto const & value : theVar.values)
        write_primitive(value, type);

    // Here we do need to take variable data padding into consideration.
    int32_t writtenCount = theVar.values.size() * get_primitive_value_size(type);

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

cdf_writer & cdf_writer::operator<<(netcdf & theCdf) {

    prepare_var_array(theCdf);

    write_magic(theCdf.magic);

    write(*pOS, get_reversed_byte_order(theCdf.numrecs));

    write_dims(theCdf.dims);

    write_attrs(theCdf.attrs);

    const auto useClassic = theCdf.magic.is_classic();

    write_vars_header(theCdf.vars, theCdf.dims, useClassic);

    write_vars_data(theCdf.vars, theCdf.dims, useClassic);

    return *this;
}
