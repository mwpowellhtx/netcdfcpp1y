
#include "netcdf_file.h"
#include "netcdf_input.h"
#include "netcdf_output.h"
#include "network_byte_order.h"

#include <fstream>
#include <cassert>

int main(int argc, char* argv[]) {

    {
        assert(!is_big_endian());
        assert(is_little_endian());
    }

    {
        auto cdf = netcdf{};

        std::ifstream ifs("Data/sresa1b_ncar_ccsm3-example.nc", std::ios::binary);

        cdf_reader(&ifs, true) >> cdf;

        std::ofstream ofs("Data/testing.nc", std::ios::binary);

        cdf_writer(&ofs, true) << cdf;
    }

    {
        attributable & aVar = var();

        aVar.add_attr<byte_vector>("some_bytes", { static_cast<uint8_t>(1), static_cast<uint8_t>(2), static_cast<uint8_t>(3), static_cast<uint8_t>(4), static_cast<uint8_t>(5) });
        aVar.add_attr<short_vector>("some_shorts", { static_cast<int16_t>(2), static_cast<int16_t>(3), static_cast<int16_t>(4), static_cast<int16_t>(5) });
        aVar.add_attr<int_vector>("some_ints", { static_cast<int32_t>(3), static_cast<int32_t>(4), static_cast<int32_t>(5) });
        aVar.add_attr<float_vector>("some_floats", { static_cast<float_t>(4), static_cast<float_t>(5) });
        aVar.add_attr<double_vector>("some_doubles", { static_cast<double_t>(5) });

        assert(aVar.attrs.size() == 5);

        assert(aVar.attrs[0].get_type() == nc_byte);
        assert(aVar.attrs[1].get_type() == nc_short);
        assert(aVar.attrs[2].get_type() == nc_int);
        assert(aVar.attrs[3].get_type() == nc_float);
        assert(aVar.attrs[4].get_type() == nc_double);

        assert(aVar.attrs[0].values.size() == 5);
        assert(aVar.attrs[1].values.size() == 4);
        assert(aVar.attrs[2].values.size() == 3);
        assert(aVar.attrs[3].values.size() == 2);
        assert(aVar.attrs[4].values.size() == 1);

        assert(aVar.attrs[0].values.front().primitive.b == 1);
        assert(aVar.attrs[1].values.front().primitive.s == 2);
        assert(aVar.attrs[2].values.front().primitive.i == 3);
        assert(aVar.attrs[3].values.front().primitive.f == 4);
        assert(aVar.attrs[4].values.front().primitive.d == 5);
    }

    {
        auto cdf = netcdf{};

        std::ofstream ofs("Data/testing2.nc", std::ios::binary);

        cdf_writer(&ofs, false) << cdf;
    }

    return 0;
}
