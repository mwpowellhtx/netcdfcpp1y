
#include "netcdf_file.h"

#include <algorithm>

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

///////////////////////////////////////////////////////////////////////////////

magic::magic()
    : /*size_reporter()
    , */version(classic) {
    init();
}

magic::magic(magic const & other)
    : /*size_reporter(other)
    , */version(other.version) {
    init();
}

void magic::init() {
    // When to (or to not) use auto... auto took some liberties with this.
    char tmp[3] = { 'C', 'D', 'F' };
    memcpy(&key, &tmp, sizeof(key));
}

bool magic::is_classic() const {
    return version == classic;
}

bool magic::is_x64() const {
    return version == x64;
}

///////////////////////////////////////////////////////////////////////////////

named::named() {
}

named::named(std::string const & name)
    : name(name) {
}

named::named(named const & other)
    : name(other.name) {
}

named::~named() {
}

int32_t named::get_name_length() const {
    return static_cast<int32_t>(name.size());
}

///////////////////////////////////////////////////////////////////////////////

enum rank : int32_t {
    rank_dim_scalar = 0,
    rank_dim_vector = 1,
    rank_dim_matrix = 2,
    //TODO: TBD: others?
};

///////////////////////////////////////////////////////////////////////////////

value::value() {
    init();
}

value::value(value const & other)
    : primitive(other.primitive)
    , text(other.text) {
}

value::value(std::string const & text) {
    init(text);
}

value::value(uint8_t x) {
    init();
    primitive.b = x;
}

value::value(int16_t x) {
    init();
    primitive.s = x;
}

value::value(int32_t x) {
    init();
    primitive.i = x;
}

value::value(float_t x) {
    init();
    primitive.f = x;
}

value::value(double_t x) {
    init();
    primitive.d = x;
}


value::~value() {
}

void value::init(std::string const & text) {
    this->text = text;
    memset(&primitive, 0, sizeof(primitive));
}

///////////////////////////////////////////////////////////////////////////////

valuable::valuable(nc_type theType)
    : type(theType)
    , values() {
}

valuable::valuable(valuable const & other)
    : type(other.type)
    , values(other.values) {
}

valuable::~valuable() {
}

nc_type valuable::get_type() const {
    return type;
}

void valuable::set_type(nc_type const & theType) {
    type = theType;
}

///////////////////////////////////////////////////////////////////////////////

dim::dim()
    : named()
    , dim_length(0) {
}

dim::dim(std::string const & name, int32_t dim_length)
    : named(name)
    , dim_length(dim_length) {
}

dim::dim(dim const & other)
    : named(other)
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

///////////////////////////////////////////////////////////////////////////////

attr::attr()
    : named()
    , valuable() {
}

attr::attr(std::string const & name, nc_type type)
    : named(name)
    , valuable(type) {
}

attr::attr(std::string const & name, std::string const & text)
    : named(name)
    , valuable() {

    set_text(text);
}

attr::attr(attr const & other)
    : named(other)
    , valuable(other) {
}

void attr::set_text(std::string const & text) {
    set_type(nc_char);
    values = { value(text) };
}

bool attr::is_supported_type(nc_type type) {
    return type == nc_char
        || is_primitive_type(type);
}

///////////////////////////////////////////////////////////////////////////////

attributable::attributable()
    : attrs() {
}

attributable::attributable(attributable const & other)
    : attrs(other.attrs) {
}

attributable::~attributable() {
}

void attributable::add_attr(attr const & theAttr) {
    //TODO: check for things like name conflicts, etc
    attrs.push_back(theAttr);
}

void attributable::add_text_attr(std::string const & name, std::string const & text) {
    add_attr(attr(name, text));
}

attr_vector::iterator attributable::get_attr(attr_vector::size_type i) {
    return attrs.begin() + i;
}

attr_vector::iterator attributable::get_attr(std::string const & name) {
    return std::find_if(attrs.begin(), attrs.end(),
        [&](attr const & x) { return x.name == name; });
}

///////////////////////////////////////////////////////////////////////////////

var::var()
    : named()
    , attributable()
    , valuable(nc_double)
    , dimids()
    , vsize(0)
    , offset() {

    memset(&offset, sizeof(offset), 0);
}

var::var(var const & other)
    : named(other)
    , attributable(other)
    , valuable(other)
    , dimids(other.dimids)
    , vsize(other.vsize)
    , offset(other.offset) {
}

var::~var() {
}

bool var::is_scalar() const {
    return dimids.size() == rank::rank_dim_scalar;
}

bool var::is_vector() const {
    return dimids.size() == rank::rank_dim_vector;
}

bool var::is_matrix() const {
    //TODO: or greater?
    return dimids.size() == rank::rank_dim_matrix;
}

bool is_scalar(var const & theVar) {
    return theVar.is_scalar();
}

bool is_vector(var const & theVar) {
    return theVar.is_vector();
}

bool is_matrix(var const & theVar) {
    return theVar.is_matrix();
}

bool var::is_record(dim_vector const & dims) const {

    // The record dimension length is zero (0).
    for (auto & dimid : dimids) {
        if (dims[dimid].is_record())
            return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

netcdf::netcdf()
    : attributable()
    , magic()
    , numrecs(0)
    , dims()
    , vars() {
}

netcdf::netcdf(netcdf const & other)
    : attributable(other)
    , magic(other.magic)
    , numrecs(other.numrecs)
    , dims(other.dims)
    , vars(other.vars) {
}

netcdf::~netcdf() {
}

void netcdf::add_dim(dim const & theDim, int32_t default_dim_length) {

    //TODO: check for name conflicts
    //TODO: TBD: may inject policies into the netcdf, add/remove/access functions ... i.e. allow_add_record_dim

    // There may be 0-1 unlimited (record) dims.
    if (theDim.is_record()) {
        for (auto & x : dims)
            if (x.is_record())
                x.dim_length = default_dim_length;
    }

    // Always add to the end for minimum impact on previously existing dims, dimids, etc.
    dims.push_back(theDim);
}

void netcdf::add_dim(std::string const & name, int32_t dim_length, int32_t default_dim_length) {
    add_dim(dim(name, dim_length), default_dim_length);
}

void netcdf::set_unlimited_dim(dim_vector::iterator dim_it, int32_t default_dim_length) {

    for (auto it = dims.begin(); it != dims.end(); it++) {

        if (it->is_record())
            it->dim_length = default_dim_length;

        if (it == dim_it)
            it->dim_length = 0;
    }
}

void netcdf::set_unlimited_dim(dim_vector::size_type const & i, int32_t default_dim_length) {

    auto dim_it = dims.begin() + i;

    set_unlimited_dim(dim_it, default_dim_length);
}

void netcdf::set_unlimited_dim(std::string const & name, int32_t default_dim_length) {

    auto dim_it = std::find_if(dims.begin(), dims.end(),
        [&](dim const & x) { return x.name == name; });

    set_unlimited_dim(dim_it, default_dim_length);
}

var_vector::iterator netcdf::get_var(var_vector::size_type i) {
    return vars.begin() + i;
}

var_vector::iterator netcdf::get_var(std::string const & name) {
    return std::find_if(vars.begin(), vars.end(),
        [&](var const & x) { return x.name == name; });
}

dim_vector::iterator netcdf::get_dim(dim_vector::size_type i) {
    return dims.begin() + i;
}

dim_vector::iterator netcdf::get_dim(std::string const & name) {
    return std::find_if(dims.begin(), dims.end(),
        [&](dim const & x) { return x.name == name; });
}

void netcdf::redim_var(var_vector::iterator var_it, dim_vector_iterator_vector const & dim_its) {

    //TODO: TBD: just return? or throw?
    if (var_it == vars.end()) return;

    var_it->dimids.clear();

    auto dim_begin = dims.begin();

    for (auto & dim_it : dim_its)
        var_it->dimids.push_back(dim_it - dim_begin);

    //TODO: what else to do with the type / data ?
}

void netcdf::redim_var(var_vector::size_type i, dim_vector_iterator_vector const & dim_its) {
    redim_var(get_var(i), dim_its);
}

void netcdf::redim_var(std::string const & name, dim_vector_iterator_vector const & dim_its) {
    redim_var(get_var(name), dim_its);
}
