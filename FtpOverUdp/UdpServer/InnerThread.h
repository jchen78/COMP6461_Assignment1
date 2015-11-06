#include "Thread.h"
#include "AsyncLock.h"
#include <iostream>
#include <string>

class InnerThread : public Common::Thread
{
private:
	std::string index;
	Common::AsyncLock* externalLock;
public:
	InnerThread() : externalLock(NULL) {}
	void setLock(int i, Common::AsyncLock* lock) { index = std::to_string(i); externalLock = lock; }
	virtual void run() {
		for (int i = 0; i < 100; i++) {
			externalLock->waitForConsumption();
			std::cout << "	Inner: " << index << " " << std::to_string(i) << std::endl;
			externalLock->finalizeConsumption();
		}
	}
};