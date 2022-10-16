#pragma once

#include <cstdio>
#include <string>
#include <filesystem>
#include <source_location>
#include <exception>
#include <string_view>

#define __LOG(LEVEL, str, ...) do{ \
		auto location = std::source_location::current(); \
		fprintf(stderr, (std::string{"[%s %s %s:%d '%s'] "} + str + std::string{"\n"}).c_str(), \
			#LEVEL, __TIME__, std::filesystem::path(location.file_name()).filename().c_str(), \
			location.line(), location.function_name(), ##__VA_ARGS__); \
	}while(0)
#define LOG_INFO(str, ...) __LOG(INFO, str, ##__VA_ARGS__)
#define LOG_WARNING(str, ...) __LOG(WARNING, str, ##__VA_ARGS__)
#define LOG_ERROR(str, ...) __LOG(ERROR, str, ##__VA_ARGS__)
#define LOG_ASSERT(str, ...) __LOG(ASSERT, str, ##__VA_ARGS__)
#define LOG(str, ...) LOG_INFO(str, ##__VA_ARGS__)

//#ifdef NDEBUG
#define ASSERT_ON_MSG(cond, msg) do{ \
		if(cond) [[unlikely]]{ \
			LOG_ASSERT(std::string{"Assert "} + std::string{msg} + std::string{" "} + std::string{#cond}); \
			throw std::logic_error("Assert " #cond); \
		} \
	}while(0)
#define ASSERT_ON(cond) ASSERT_ON_MSG(cond, "")

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

#define RET_ON_X(res, X) do{ auto __res = (res); if(__res == (X)){ return __res; } }while(0)
#define RET_ON_SUCCESS(res) RET_ON_X((res), Result::Success)
#define RET_ON_FAILURE(res) RET_ON_X((res), Result::Failure)
#define RET_ON_FAIL(res) do{ auto __res = (res); if(__res){ return Result::Failure; } }while(0)

template <typename T>
const T *PTR_OFFSET(const T *base, size_t offset){
	return reinterpret_cast<const T *>(reinterpret_cast<const char *>(base) + offset);
}
template <typename T>
T *PTR_OFFSET(T *base, size_t offset){
	return reinterpret_cast<T *>(reinterpret_cast<char *>(base) + offset);
}

template <typename T>
uint8_t MostSignificantBitSet(T v){
#if __has_builtin(__builtin_clzll)
  if(v == 0)
	return 0;
  uint8_t LeadingZeros = __builtin_clzll(v);
  constexpr uint8_t BitWidth = sizeof(T);
  return BitWidth - LeadingZeros - 1;
#else
#error "Unsupported CLZ command"
#endif
}

template<typename F>
class ScopeDestructor{
	F f;
public:
	ScopeDestructor(F &&f): f(std::forward<F>(f)) {}
	~ScopeDestructor(){ f(); }
};