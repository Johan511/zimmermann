#include <boost/program_options.hpp>
#include <gtest/gtest.h>
#include <httplib.h>
#include <iostream>

int main(int argc, char *argv[])
{
    // from the found package (Boost::program_options)
    namespace po = boost::program_options;
    po::options_description desc("tpt_demo options");
    desc.add_options()("help", "show help")("count", po::value<int>()->default_value(1),
                                            "print a number");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 0;
    }
    std::cout << "count = " << vm["count"].as<int>() << std::endl;

    EXPECT_EQ(1, 1);                         // from googletest (FetchContent)
    httplib::Client cli("http://localhost"); // from cpp-httplib (find → fetch fallback)
    std::cout << "tpt_demo ok" << std::endl;
    return 0;
}
