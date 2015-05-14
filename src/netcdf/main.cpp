
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

        std::ifstream ifs("sresa1b_ncar_ccsm3-example.nc", std::ios::binary);

        netcdf_reader(ifs, true) >> cdf;

        std::ofstream ofs("testing.nc", std::ios::binary);

        cdf_writer(ofs, true) << cdf;
    }

    {
        auto cdf = netcdf{};

        std::ofstream ofs("test.netcdf", std::ios::binary);

        cdf_writer(ofs, false) << cdf;
    }

    return 0;
}
