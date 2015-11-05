#include "Thread.h"
#include "AsyncLock.h"
#include <iostream>
#include <string>

class ThreadA : public Common::Thread
{
private:
	Common::AsyncLock* lock;
public:
	ThreadA(Common::AsyncLock* sharedLock) : lock(sharedLock) {}
	virtual void run() {
		time_t start, current;
		for (int i = 0; i < 100; i++) {
			lock->waitForConsumption();
			std::cout << "A" << std::endl;
			lock->finalizeConsumption();
		}
	}
};