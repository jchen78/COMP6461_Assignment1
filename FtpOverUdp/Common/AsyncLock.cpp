#include "stdafx.h"
#include "AsyncLock.h"

namespace Common
{
	AsyncLock::AsyncLock(bool startWithConsumption)
	{
		isConsumptionState = startWithConsumption;
	}

	void AsyncLock::waitForConsumption() {
		std::unique_lock<std::mutex> locker(readLock);
		readOperation.wait(locker, [&]() { return isConsumptionState; });
	}

	void AsyncLock::finalizeConsumption() {
		isConsumptionState = false;
		writeOperation.notify_one();
	}

	void AsyncLock::waitForSignalling() {
		std::unique_lock<std::mutex> locker(writeLock);
		writeOperation.wait(locker, [&]() { return !isConsumptionState; });
	}

	void AsyncLock::finalizeSignalling() {
		isConsumptionState = true;
		readOperation.notify_one();
	}
}