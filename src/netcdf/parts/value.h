#ifndef NETCDF_VALUE_H
#define NETCDF_VALUE_H

#pragma once

#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////

struct valuable;

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

    friend struct valuable;
};

typedef std::vector<value> value_vector;

#endif //NETCDF_VALUE_H
