#include "Helpers.hpp"
#include <sstream>

namespace Help
{

inline uint64_t rdtsc()
{
    return __rdtsc();
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