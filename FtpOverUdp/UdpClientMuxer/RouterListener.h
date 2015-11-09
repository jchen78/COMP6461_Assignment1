#pragma once
#include <map>
#include <winsock.h>
#include "Thread.h"

using namespace std;

class RouterListener : public Common::Thread
{
private:
	int routerSocket;				// Router socket
	int clientSocket;				// Client socket
	map<int, sockaddr_in> *clients; // Map between client ID and address
public:
	RouterListener(int routerSocket, int clientSocket, map<int, sockaddr_in> *clientMap);
	virtual void run();
	~RouterListener();
};

