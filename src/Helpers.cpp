#include "Helpers.hpp"
#include <sstream>

namespace Help
{

#if defined(_WIN32)
#include <intrin.h>
#define impl_rdtsc() __rdtsc()
#else
#define impl_rdtsc() 0
#endif
inline uint64_t rdtsc()
{
    return impl_rdtsc();
}

std::vector<std::string> TokenizeString(std::string str)
{
    std::stringstream        ss{str};
    std::vector<std::string> result;

    while (ss.good()) {
        std::string substr;
        std::getline(ss, substr, ',');
        result.push_back(substr);
    }

    return result;
}
} // namespace Help