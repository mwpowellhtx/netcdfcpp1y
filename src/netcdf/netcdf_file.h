#ifndef NETCDF_FILE_H
#define NETCDF_FILE_H

#pragma once

#include "algorithm.hpp"

#include <cstdint>
#include <string>
#include <limits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////

int32_t pad_width(int32_t width);

bool try_pad_width(int32_t & width);
    
//http://www.unidata.ucar.edu/software/netcdf/docs/netcdf/File-Format-Specification.html
/*
Fields such as nelems has implied meaning for the model, but not to the file format.
nelems fields should preceed the thing it is counting, and are always INT (int32_t).
In the model, this is inherent in the fact that such collections are represented as vectors.

Same generally goes for nc_type specifications for the array structs. For file I/O purposes,
this can be discerned, and all we need to do is reference the model at that time, either
returning nc_absent or the array type designation, as appropriate.
*/

enum cdf_version : uint8_t {
    classic = 0x1,
    x64 = 0x2
};

enum nc_type : int32_t {
    nc_absent = 0x0,
    nc_byte = 0x1,
    nc_char = 0x2,
    nc_short = 0x3,
    nc_int = 0x4,
    nc_float = 0x5,
    nc_double = 0x6,
    nc_dimension = 0xa,
    nc_variable = 0xb,
    nc_attribute = 0xc,
};

template<typename _Ty>
nc_type get_type_for() {

    auto type = nc_absent;
    auto & tid = typeid(_Ty);

    // nc_char is a special case that must be handled apart from these types.

    if (tid == typeid(uint8_t))
        type = nc_byte;
    else if (tid == typeid(int16_t))
        type = nc_short;
    else if (tid == typeid(int32_t))
        type = nc_int;
    else if (tid == typeid(float_t))
        type = nc_float;
    else if (tid == typeid(double_t))
        type = nc_double;

    return type;
}

template<typename _Ty>
bool try_get_type_for(nc_type & type) {
    type = get_type_for<_Ty>();
    return type != nc_absent;
}

bool is_endian_type(nc_type type);

bool is_primitive_type(nc_type type);

int32_t get_primitive_value_size(nc_type type);

struct netcdf;

///////////////////////////////////////////////////////////////////////////////

struct magic {
    typedef char key_type[3];
    key_type key;
    cdf_version version;

    magic();

    magic(magic const & other);

    bool is_classic() const;
    bool is_x64() const;

private:

    void init();
};

///////////////////////////////////////////////////////////////////////////////

struct named {
    std::string name;
    int32_t get_name_length() const;
    virtual ~named();
protected:
    named();
    named(std::string const & name);
    named(named const & other);
};

///////////////////////////////////////////////////////////////////////////////

struct attr;

struct value {
    /* This is generally ill advised having a union hanging out here without a closely
    adjoined type selector, but for file format purposes, this is just fine, since the
    scope of that decision is slightly beyond the scope of the values themselves. */
    union {
        uint8_t b;
        int16_t s;
        int32_t i;
        float_t f;
        double_t d;
    } primitive;

    std::string text;

    value();
    value(std::string const & text);
    value(value const & other);

    virtual ~value();

private:

    void init(std::string const & text = "");

    //TODO: should these be more exposed: i.e. for writer/reader purposes?
    value(uint8_t x);
    value(int16_t x);
    value(int32_t x);
    value(float_t x);
    value(double_t x);

    friend struct attr;
};

typedef std::vector<value> value_vector;

///////////////////////////////////////////////////////////////////////////////

struct dim : public named {
    int32_t dim_length;
    bool is_record() const;
    int32_t get_dim_length_part() const;
    dim();
    dim(dim const & other);
    virtual ~dim();

private:

    dim(std::string const & name, int32_t dim_length);

    friend struct netcdf;
};

typedef std::vector<dim> dim_vector;

struct var;

typedef std::vector<uint8_t> byte_vector;
typedef std::vector<int16_t> short_vector;
typedef std::vector<int32_t> int_vector;
typedef std::vector<float_t> float_vector;
typedef std::vector<double_t> double_vector;

struct attr : public named {

    nc_type type;
    value_vector values;

    virtual nc_type get_type() const;
    void set_type(nc_type const & aType);

    attr();
    attr(attr const & other);

    template<class _Vector>
    void set_values(_Vector const & values) {

        nc_type type;

        //TODO: TBD: may throw an exception here instead...
        if (!try_get_type_for<_Vector::value_type>(type))
            return;

        set_type(type);

        // Do it this way. This is way more efficient than the aggregate function.
        auto & this_values = this->values;

        this_values.clear();

        std::for_each(values.cbegin(), values.cend(),
            [&](_Vector::value_type x) { this_values.push_back(value(x)); });

        //typedef std::function<value_vector(value_vector, _Vector::value_type)> aggregate_func;

        //this->values = aggregate<_Vector::value_type, value_vector, aggregate_func>(
        //    values, value_vector(),
        //    [&](value_vector g, _Vector::value_type x) {
        //    g.push_back(value(x));
        //    return g;
        //});
    }

    void set_text(std::string const & text);

private:

    attr(std::string const & name, nc_type type = nc_absent);
    attr(std::string const & name, std::string const & text);

    friend struct attributable;

    static bool is_supported_type(nc_type type);
};

typedef std::vector<attr> attr_vector;

///////////////////////////////////////////////////////////////////////////////

struct attributable {

    attr_vector attrs;

    virtual ~attributable();

    virtual void add_attr(attr const & anAttr);

    // strings are kind of like vectors but they are different enough that the use cases cannot be easily co-mingled and still be useful
    virtual void add_text_attr(std::string const & name, std::string const & text);

    //TODO: TBD: methinks that type should simply be an overloaded, inherency about how to work with attributes
    template<class _Vector>
    void add_attr(std::string const & name, _Vector const & values) {
        auto & theAttr = attr(name);
        theAttr.set_values(values);
        add_attr(theAttr);
    }

    virtual attr_vector::iterator get_attr(attr_vector::size_type i);
    virtual attr_vector::iterator get_attr(std::string const & name);

protected:

    attributable();
    attributable(attributable const & other);
};


///////////////////////////////////////////////////////////////////////////////

typedef union {
    int32_t begin;
    int64_t begin64;
} offset_t;

typedef std::vector<int32_t> dimid_vector;

//TODO: TBD: what other interface this will require to get/set/insert/update/delete variables, in a model-compatible manner
struct var : public named, public attributable {
    //See rank (dimensionality) ... rank nelems (rank alone? or always INT ...)
    dimid_vector dimids;
    nc_type type;
    //TODO: may not support vsize after all? does it make sense to? especially with backward/forward compatibility growth concerns...
    int32_t vsize;
    //TODO: TBD: this one could be tricky ...
    offset_t offset;
    //TODO: it's not a question of header/data, record/non-record, although it is: from a domain model perspective, data belongs to each variable
    std::vector<value> data;

    var();

    //TODO: possible to declare Vector as a friend?
    var(var const & other);

    virtual ~var();

    bool is_scalar() const;
    bool is_vector() const;
    bool is_matrix() const;

    // Not to be confused with typed_array, even though these are the same sort of interface the semantics are clearly different.
    nc_type get_type() const;
    bool is_record(dim_vector const & dims) const;
};

bool is_scalar(var const & v);
bool is_vector(var const & v);
bool is_matrix(var const & v);

typedef std::vector<var> var_vector;

///////////////////////////////////////////////////////////////////////////////

struct netcdf : public attributable {
public:

    typedef std::vector<dim_vector::iterator> dim_vector_iterator_vector;

    magic magic;

    //TODO: upwards of MAX_INT32 (yes, that's 0xffffffff, or STREAMING) ...
    //TODO: TBD: numrecs could (/should) be more of a dynamic get function?
    int32_t numrecs;

    dim_vector dims;

    var_vector vars;

    netcdf();
    netcdf(netcdf const & other);

    virtual ~netcdf();

    virtual void add_dim(dim const & aDim, int32_t default_dim_length = 1);
    virtual void add_dim(std::string const & name, int32_t dim_length = 1, int32_t default_dim_length = 1);

    virtual dim_vector::iterator get_dim(dim_vector::size_type i);
    virtual dim_vector::iterator get_dim(std::string const & name);

    virtual void set_unlimited_dim(dim_vector::iterator dim_it, int32_t default_dim_length = 1);
    virtual void set_unlimited_dim(dim_vector::size_type const & i, int32_t default_dim_length = 1);
    virtual void set_unlimited_dim(std::string const & name, int32_t default_dim_length = 1);

    virtual var_vector::iterator get_var(var_vector::size_type i);
    virtual var_vector::iterator get_var(std::string const & name);

    virtual void redim_var(var_vector::iterator var_it, dim_vector_iterator_vector const & dim_its);
    virtual void redim_var(var_vector::size_type i, dim_vector_iterator_vector const & dim_its);
    virtual void redim_var(std::string const & name, dim_vector_iterator_vector const & dim_its);
};

/* TODO: TBD: still to come, how to work with the "shape" of data via the netcdf;
that's the whole point of utilizing the format, that data can be shaped and worked with
TBD: could be some "operators" that permit definition, mutation, access, and so on;
might be interesting to consider whether variadics hold any value for accessing values
along a range of values */

#endif //NETCDF_FILE_H
