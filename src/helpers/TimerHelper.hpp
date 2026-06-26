#pragma once
#ifndef _TIME_HELPER_HPP_
#define _TIME_HELPER_HPP_
#include <chrono>
namespace Long::Time {
	typedef std::chrono::steady_clock::time_point time_t; 
	static const time_t now() {
		return std::chrono::high_resolution_clock::now(); 
	}
	static double elapsedMs(const time_t& t) {
		return std::chrono::duration<double, std::milli>(now() - t).count(); 
	}
	static double elapsedSecond(const time_t& t) {
		return elapsedMs(t) * 0.001; 
	}
}
#endif // !_TIME_HELPER_HPP_
