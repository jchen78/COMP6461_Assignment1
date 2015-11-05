#include "Thread.h"
#include "AsyncLock.h"
#include <iostream>
#include <string>

class ThreadB : public Common::Thread
{
private:
	Common::AsyncLock* lock;
public:
	ThreadB(Common::AsyncLock* sharedLock) : lock(sharedLock) {}
	virtual void run() {
		time_t start, current;
		for (int i = 0; i < 100; i++) {
			lock->waitForSignalling();
			std::cout << "B" << std::endl;
			lock->finalizeSignalling();
		}
	}
};