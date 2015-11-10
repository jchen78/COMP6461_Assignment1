#pragma once

#include <map>
#include <winsock.h>
#include <AsyncLock.h>
#include <Thread.h>

using namespace std;

class ClientListener : public Common::Thread
{
private:
	int clientSocket;
	int routerSocket;
	sockaddr_in routerAddress;
	map<int, sockaddr_in> clientMap;
	Common::AsyncLock clientLock;
public:
	ClientListener(int clientSocket, int routerSocket, sockaddr_in routerAddress) : clientSocket(clientSocket), routerSocket(routerSocket), routerAddress(routerAddress), clientLock(false, -1) {}
	map<int, sockaddr_in>* getMap() { return &clientMap; }
	virtual void run();
	~ClientListener() {}
};

