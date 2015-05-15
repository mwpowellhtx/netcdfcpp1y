#ifndef NETCDF_FILE_H
#define NETCDF_FILE_H

#pragma once

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

bool is_endian_type(nc_type type);

bool is_primitive_type(nc_type type);

int32_t get_primitive_value_size(nc_type type);

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
    named(named const & other);
};

///////////////////////////////////////////////////////////////////////////////

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
    value(value const & other);
    value(std::string const & text);

    virtual ~value();
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
};

typedef std::vector<dim> dim_vector;

struct attr : public named {
    nc_type type;
    value_vector values;
    virtual nc_type get_type() const;
    void set_type(nc_type const & t);
    attr();
    attr(attr const & other);
};

typedef std::vector<attr> attr_vector;

///////////////////////////////////////////////////////////////////////////////

typedef union {
    int32_t begin;
    int64_t begin64;
} offset_t;

typedef std::vector<int32_t> dimid_vector;

//TODO: TBD: what other interface this will require to get/set/insert/update/delete variables, in a model-compatible manner
struct var : public named {
    //See rank (dimensionality) ... rank nelems (rank alone? or always INT ...)
    dimid_vector dimids;
    attr_vector vattrs;
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

struct netcdf {

    magic magic;

    //TODO: upwards of MAX_INT32 (yes, that's 0xffffffff, or STREAMING) ...
    //TODO: TBD: numrecs could (/should) be more of a dynamic get function?
    int32_t numrecs;

    dim_vector dims;

    attr_vector gattrs;

    var_vector vars;

    netcdf();

    netcdf(netcdf const & other);

    virtual ~netcdf();
};

/* TODO: TBD: still to come, how to work with the "shape" of data via the netcdf;
that's the whole point of utilizing the format, that data can be shaped and worked with
TBD: could be some "operators" that permit definition, mutation, access, and so on;
might be interesting to consider whether variadics hold any value for accessing values
along a range of values */

#endif //NETCDF_FILE_H
