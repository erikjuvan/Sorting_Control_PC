#pragma once

#include <cstdint>
#include <intrin.h>
#include <chrono>
#include <string>

static uint64_t rdtsc(){
    return __rdtsc();
}

#define TIME_IT(x) \
	{auto s1 = std::chrono::high_resolution_clock::now(); \
	x; \
	auto s2 = std::chrono::high_resolution_clock::now(); \
	auto str = std::string(#x) + ": " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(s2 - s1).count()) + " us"; \
	qInfo(str.c_str());}