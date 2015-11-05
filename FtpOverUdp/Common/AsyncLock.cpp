#include "stdafx.h"
#include "AsyncLock.h"

namespace Common
{
	AsyncLock::AsyncLock(bool waitForReader, bool waitForWriter)
	{
		isAsyncReady = !waitForReader;
		isAsyncDone = !waitForWriter;
	}

	void AsyncLock::waitForConsumption() {
		while (!isAsyncReady) {
			std::unique_lock<std::mutex> locker(readLock);
			readOperation.wait(locker);
		}
	}

	void AsyncLock::finalizeConsumption() {
		isAsyncReady = false;
		isAsyncDone = true;
		writeOperation.notify_one();
	}

	void AsyncLock::waitForSignalling() {
		while (!isAsyncDone) {
			std::unique_lock<std::mutex> locker(writeLock);
			writeOperation.wait(locker);
		}
	}

	void AsyncLock::finalizeSignalling() {
		isAsyncDone = false;
		isAsyncReady = true;
		readOperation.notify_one();
	}
}