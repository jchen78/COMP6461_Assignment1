#pragma once

#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif

#include <mutex>
#include <condition_variable>

namespace Common
{
	// Implementation adapted from: http://codexpert.ro/blog/2013/03/01/cpp11-concurrency-condition-variables/
	class COMMON_API AsyncLock
	{
	private:
		bool isConsumptionState;
		std::mutex switchLock;

		std::mutex readLock;
		std::condition_variable readOperation;

		std::mutex writeLock;
		std::condition_variable writeOperation;
	public:
		AsyncLock(bool startWithConsumption);

		void waitForConsumption();
		void finalizeConsumption();

		void waitForSignalling();
		void finalizeSignalling();
	};
}