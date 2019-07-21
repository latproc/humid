
#include <iostream>

//#include <dru_boost_dep.hpp>
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/io/time_point_io.hpp>
#include <boost/chrono/chrono.hpp>
#include <boost/filesystem/path.hpp>

class X
{
public:

};
int main(int argc, char **argv)
{
    std::cout << "Hello Windows. I'm Linux.\n";
    boost::shared_ptr<X> test(new X());
    std::cout << "boost shared ptr seems to be working...\n";

    boost::chrono::system_clock::time_point time = boost::chrono::system_clock::now();
    std::cout << "boost time: " << time << "\n";

    boost::filesystem::path x("/");

    return 0;
}
