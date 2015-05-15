#ifndef NETCDF_FILE_H
#define NETCDF_FILE_H

#pragma once

#include <cstdint>
#include <string>
#include <limits>
#include <vector>

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

bool is_primitive_type(nc_type type);

int32_t get_primitive_value_size(nc_type type);

//struct size_reporter {
//
//    virtual int32_t get_sizeof() const = 0;
//
//protected:
//
//    size_reporter();
//    size_reporter(size_reporter const &);
//};

struct magic /*: public size_reporter*/ {
    typedef char key_type[3];
    key_type key;
    cdf_version version;

    magic();

    magic(magic const & other);

    //virtual int32_t get_sizeof() const;

    bool is_classic() const;
    bool is_x64() const;

private:

    void init();
};

struct named {
    //int32_t nelems;
    std::string name;
    int32_t get_name_length() const;
    virtual ~named();
protected:
    named();
    named(named const & other);
    //int32_t get_sizeof() const;
};

//TODO: while technically, yes, this is what needs to get described, from a modeling perspective, it is unnecessary apart from I/O itself
//struct typed_array {
//    virtual nc_type get_type() const = 0;
//    virtual int32_t get_nelems() const = 0;
//    virtual ~typed_array();
//protected:
//    typed_array();
//    typed_array(typed_array const & other);
//    int32_t get_sizeof() const;
//};

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

    //int32_t get_sizeof(nc_type const & type) const;
};

typedef std::vector<value> value_vector;

struct dim : public named/*, public size_reporter*/ {
    int32_t dim_length;
    bool is_record() const;
    int32_t get_dim_length_part() const;
    //virtual int32_t get_sizeof() const;
    dim();
    dim(dim const & other);
    virtual ~dim();
};

typedef std::vector<dim> dim_vector;

//struct dim_array : /*public typed_array,*/ public size_reporter {
//    //nc_type type = nc_absent; //or nc_dimension
//    std::vector<dim> dims;
//    virtual nc_type get_type() const;
//    virtual int32_t get_nelems() const;
//    virtual int32_t get_sizeof() const;
//    dim_array();
//    dim_array(dim_array const & other);
//    virtual ~dim_array();
//};

struct attr : public named/*, public typed_array*//*, public size_reporter*/ {
    nc_type type;
    value_vector values;
    virtual nc_type get_type() const;
    //virtual int32_t get_nelems() const;
    //virtual int32_t get_sizeof() const;
    void set_type(nc_type const & t);
    attr();
    attr(attr const & other);
};

//// Serves as both gatt_array (global attributes) and att_array...
//struct att_array : /*public typed_array,*/ public size_reporter {
//    //nc_type type = nc_absent; //or nc_attribute
//    std::vector<attr> atts;
//    virtual nc_type get_type() const;
//    virtual int32_t get_nelems() const;
//    virtual int32_t get_sizeof() const;
//};

typedef std::vector<attr> attr_vector;

typedef union {
    int32_t begin;
    int64_t begin64;
} offset_t;

//TODO: TBD: what other interface this will require to get/set/insert/update/delete variables, in a model-compatible manner
struct var : public named/*, public size_reporter*/ {
    //See rank (dimensionality) ... rank nelems (rank alone? or always INT ...)
    std::vector<int32_t> dimids;
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
    //int32_t get_nelems() const;
    nc_type get_type() const;
    bool is_record(dim_vector const & dims) const;
    //int32_t get_expected_nelems() const;
    //int32_t get_calculated_size(dim_vector const & dims) const;
    //int32_t get_calculated_size_raw(dim_vector const & dims) const;
    //int32_t get_sizeof(bool useClassic) const;
    //int32_t get_sizeof_data() const;

//protected:
//    virtual int32_t get_sizeof() const;
};

bool is_scalar(var const & v);
bool is_vector(var const & v);
bool is_matrix(var const & v);

//struct var_array : /*public typed_array,*/ size_reporter {
//    //nc_type type = nc_absent; //or nc_variable
//    //int32_t nelems;
//    std::vector<var> vars;
//    virtual nc_type get_type() const;
//    virtual int32_t get_nelems() const;
//    int32_t get_sizeof(bool useClassic) const;
//
//protected:
//    virtual int32_t get_sizeof() const;
//};

typedef std::vector<var> var_vector;

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

    //size_t get_sizeof_header();
};

#endif //NETCDF_FILE_H
