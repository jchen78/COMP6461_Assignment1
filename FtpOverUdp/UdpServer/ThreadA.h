#include "Thread.h"
#include "AsyncLock.h"
#include "InnerThread.h"
#include <iostream>
#include <string>

class ThreadA : public Common::Thread
{
private:
	Common::AsyncLock* lock;
	InnerThread innerThreads[100];
	Common::AsyncLock innerLocks[100];
public:
	ThreadA(Common::AsyncLock* sharedLock) : lock(sharedLock) {
		for (int i = 0; i < 100; i++) {
			innerThreads[i].setLock(i, &innerLocks[i]);
			innerThreads[i].start();
		}
	}
	
	virtual void run() {
		time_t start, current;
		for (int i = 0; i < 100; i++) {
			lock->waitForConsumption();
			std::cout << "A: " << std::to_string(i) << std::endl;
			for (int j = 0; j < 100; j++) {
				innerLocks[j].finalizeSignalling();
				innerLocks[j].waitForSignalling();
			}
			lock->finalizeConsumption();
		}
	}
};