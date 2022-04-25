#pragma once

#include <cstdio>
#include <string>
#include <filesystem>
#include <source_location>
#include <exception>

#define LOG(LEVEL, str, ...) do{ \
		auto location = std::source_location::current(); \
		fprintf(stderr, "[%s %s %s:%d '%s'] " str "\n", \
			#LEVEL, __TIME__, std::filesystem::path(location.file_name()).filename().c_str(), \
			location.line(), location.function_name(), ##__VA_ARGS__); \
	}while(0)
#define LOG_INFO(str, ...) LOG(INFO, str, ##__VA_ARGS__)
#define LOG_WARNING(str, ...) LOG(WARNING, str, ##__VA_ARGS__)
#define LOG_ERROR(str, ...) LOG(ERROR, str, ##__VA_ARGS__)
#define LOG_ASSERT(str, ...) LOG(ASSERT, str, ##__VA_ARGS__)

#ifdef NDEBUG
#define ASSERT_ON(cond)
#else
#define ASSERT_ON(cond) do{ \
		if(cond) [[unlikely]]{ \
			LOG_ASSERT("Assert " #cond); \
			throw std::logic_error("Assert " #cond); \
		} \
	}while(0)
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG == 0
#define LOG_DEBUG(str, ...)
#else
#define LOG_DEBUG(str, ...) LOG(DEBUG, str, ##__VA_ARGS__)
#endif

enum class Result{
	Success,
	Failure,
};

template <typename T>
const T *PTR_OFFSET(const T *base, size_t offset){
	return static_cast<const T *>(static_cast<const char *>(base) + offset);
}
template <typename T>
T *PTR_OFFSET(T *base, size_t offset){
	return static_cast<T *>(static_cast<char *>(base) + offset);
}
