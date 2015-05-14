
#include "netcdf_file.h"
#include "algorithm.hpp"

#include <algorithm>

int32_t pad_width(int32_t width) {

    const int32_t boundary = sizeof(int32_t);

    auto padding = 0;

    auto do_nothing = [](){};

    /* Padding can be ignore during read, but needs to be included during write.
    Which is really just a function of nelems multiplied by nc_type size. */

    while ((width + padding++) % boundary) do_nothing();

    return width;
}

bool try_pad_width(int32_t & width) {
    const int32_t boundary = sizeof(int32_t);
    auto result = width++ % boundary;
    return result;
}

bool is_primitive_type(nc_type type) {
    switch (type) {
    case nc_byte: return sizeof(uint8_t);
    case nc_short: return sizeof(int16_t);
    case nc_int: return sizeof(int32_t);
    case nc_float: return sizeof(float_t);
    case nc_double: return sizeof(double_t);
    }
    return 0;
}

int32_t get_primitive_value_size(nc_type type) {
    switch (type) {
    case nc_byte: return sizeof(uint8_t);
    case nc_short: return sizeof(int16_t);
    case nc_int: return sizeof(int32_t);
    case nc_float: return sizeof(float_t);
    case nc_double: return sizeof(double_t);
    }
    throw new std::exception("unsupported type");
}

size_reporter::size_reporter() {
}

size_reporter::size_reporter(size_reporter const &) {
}

magic::magic()
    : size_reporter()
    , version(classic) {
    init();
}

magic::magic(magic const & other)
    : size_reporter(other)
    , version(other.version) {
    init();
}

void magic::init() {
    // When to (or to not) use auto... auto took some liberties with this.
    char tmp[3] = { 'C', 'D', 'F' };
    memcpy(&key, &tmp, sizeof(key));
}

int32_t magic::get_sizeof() const {
    return sizeof(char[3]) + sizeof(version);
}

bool magic::is_classic() const {
    return version == classic;
}

bool magic::is_x64() const {
    return version == x64;
}

int32_t named::get_name_length() const {
    return static_cast<int32_t>(name.size());
}

named::named() {
}

named::named(named const & other)
    : name(other.name) {
}

named::~named() {
    //name = std::string();
}

int32_t named::get_sizeof() const {
    return sizeof(int32_t) + name.length();
}

typed_array::typed_array() {
}

typed_array::typed_array(typed_array const & other) {
}

typed_array::~typed_array() {
}

int32_t typed_array::get_sizeof() const {
    return sizeof(nc_type) + sizeof(int32_t);
}

enum rank : int32_t {
    dim_scalar = 0,
    dim_vector = 1,
    dim_matrix = 2,
    //TODO: TBD: others?
};

value::value()
    : text() {
    memset(&primitive, 0, sizeof(primitive));
}

value::value(value const & other)
    : primitive(other.primitive)
    , text(other.text) {
}

value::value(std::string const & text)
    : text(text) {
    memset(&primitive, 0, sizeof(primitive));
}

value::~value() {
    //text.clear();
}

int32_t value::get_sizeof(nc_type const & type) const {

    if (is_primitive_type(type))
        return get_primitive_value_size(type);

    if (type == nc_char)
        return pad_width(sizeof(int32_t) + text.length());

    throw std::exception("unsupported type");
}

dim::dim()
    : named()
    , size_reporter()
    , dim_length(0) {
}

dim::dim(dim const & other)
    : named(other)
    , size_reporter(other)
    , dim_length(other.dim_length) {
}

dim::~dim() {
}

bool dim::is_record() const {
    return !dim_length;
}

int32_t dim::get_dim_length_part() const {
    return is_record() ? 1 : dim_length;
}

int32_t dim::get_sizeof() const {
    return named::get_sizeof() + sizeof(int32_t);
}

dim_array::dim_array()
    : typed_array()
    , size_reporter()
    , dims() {
}

dim_array::dim_array(dim_array const & other)
    : typed_array(other)
    , size_reporter(other)
    , dims(other.dims) {
}

dim_array::~dim_array() {
    //dims.clear();
}

nc_type dim_array::get_type() const {
    return dims.size() ? nc_dimension : nc_absent;
}

int32_t dim_array::get_nelems() const {
    return static_cast<int32_t>(dims.size());
}

int32_t dim_array::get_sizeof() const {
    return sizeof(int32_t) + sizeof(int32_t)
        + aggregate<dim, int32_t>(dims, (int32_t)0,
        [&](int32_t & g, dim const & x) { g += x.get_sizeof(); });
}

attr::attr()
    : named()
    , typed_array()
    , size_reporter()
    , type(nc_absent)
    , values() {
}

attr::attr(attr const & other)
    : named(other)
    , typed_array(other)
    , size_reporter(other)
    , type(other.type)
    , values(other.values) {
}

nc_type attr::get_type() const {
    return type;
}

int32_t attr::get_nelems() const {
    return static_cast<int32_t>(values.size());
}

int32_t attr::get_sizeof() const {
    auto arr_sizeof_sum = 0;
    return named::get_sizeof() + typed_array::get_sizeof()
        + aggregate<value, int32_t>(values, (int32_t)0,
        [&](int32_t & g, value const & x) { g += x.get_sizeof(get_type()); });
}

void attr::set_type(nc_type const & t) {
    type = t;
}

nc_type att_array::get_type() const {
    return atts.size() ? nc_attribute : nc_absent;
}

int32_t att_array::get_nelems() const {
    return static_cast<int32_t>(atts.size());
}

int32_t att_array::get_sizeof() const {
    return typed_array::get_sizeof()
        + aggregate<attr, int32_t>(atts, (int32_t)0,
        [&](int32_t & g, attr const & x) { g += x.get_sizeof(); });
}

var::var()
    : dimids()
    , vatt_array()
    , type(nc_double)
    , vsize(0)
    , offset()
    , data() {

    memset(&offset, sizeof(offset), 0);
}

var::var(var const & other)
    : named(other)
    , dimids(other.dimids)
    , vatt_array(other.vatt_array)
    , type(other.type)
    , vsize(other.vsize)
    , offset(other.offset)
    , data(other.data) {
}

var::~var() {
    //dimids.clear();
    //data.clear();
}

int32_t var::get_nelems() const {
    return static_cast<int32_t>(dimids.size());
}

nc_type var::get_type() const {
    return type;
}

bool var::is_scalar() const {
    return dimids.size() == rank::dim_scalar;
}

bool var::is_vector() const {
    return dimids.size() == rank::dim_vector;
}

bool var::is_matrix() const {
    //TODO: or greater?
    return dimids.size() == rank::dim_matrix;
}

bool is_scalar(var const & v) {
    return v.is_scalar();
}

bool is_vector(var const & v) {
    return v.is_vector();
}

bool is_matrix(var const & v) {
    return v.is_matrix();
}

bool var::is_record(dim_array const & dims) const {

    // The record dimension length is zero (0).
    for (auto & dimid : dimids)
        if (dims.dims[dimid].is_record())
            return true;

    return false;
}

int32_t var::get_expected_nelems() const {
    return vsize / get_primitive_value_size(get_type());
}

int32_t var::get_calculated_size_raw(dim_array const & dims) const {

    ////TODO: TBD: comprehention of max?
    //const auto max_expected = std::numeric_limits<int32_t>::max();

    //TODO: how to 64-bit prevent overflow
    int64_t vexpected = get_primitive_value_size(get_type());

    //TODO: TBD: may want to capture this dimension component as a method on dim ...
    //TODO: if any of these are zero (0) then we're talking about the "record" dimension, along which the variable can grow
    for (auto & dimid : dimids)
        vexpected *= dims.dims[dimid].get_dim_length_part();

    return static_cast<int32_t>(vexpected);
}

int32_t var::get_calculated_size(dim_array const & dims) const {
    return pad_width(get_calculated_size_raw(dims));
}

int32_t var::get_sizeof() const {
    return named::get_sizeof()
        + sizeof(int32_t)
        + sizeof(int32_t)*dimids.size()
        + vatt_array.get_sizeof()
        + sizeof(nc_type)
        + sizeof(int32_t);
    // Includes everything but the BEGIN field (which depends whether classic versus x64).
}

int32_t var::get_sizeof(bool useClassic) const {
    return get_sizeof()
        + useClassic
        ? sizeof(offset.begin)
        : sizeof(offset.begin64);
}

int32_t var::get_sizeof_data() const {
    // Size of the data starting from the data elements themselves.
    return pad_width(data.size() * value().get_sizeof(get_type()));
}

nc_type var_array::get_type() const {
    return vars.size() ? nc_variable : nc_absent;
}

int32_t var_array::get_nelems() const {
    return static_cast<int32_t>(vars.size());
}

int32_t var_array::get_sizeof() const {
    return typed_array::get_sizeof();
    //Can only report the array and dimensions.
}

int32_t var_array::get_sizeof(bool useClassic) const {
    return get_sizeof()
        + aggregate<var, int32_t>(vars, (int32_t)0,
        [&](int32_t & g, var const & x) { g += x.get_sizeof(useClassic); });
}

netcdf::netcdf()
    : magic()
    , numrecs(0)
    , dim_array()
    , gatt_array()
    , var_array() {
}

netcdf::netcdf(netcdf const & other)
    : magic(other.magic)
    , numrecs(other.numrecs)
    , dim_array(other.dim_array)
    , gatt_array(other.gatt_array)
    , var_array(other.var_array) {
}

netcdf::~netcdf() {
}

int32_t netcdf::get_sizeof_header() {
    return magic.get_sizeof()
        + sizeof(numrecs)
        + dim_array.get_sizeof()
        + gatt_array.get_sizeof()
        + var_array.get_sizeof(magic.is_classic());
}
