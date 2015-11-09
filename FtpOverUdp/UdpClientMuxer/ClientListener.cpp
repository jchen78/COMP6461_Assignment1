#include "stdafx.h"
#include <time.h>
#include "ClientListener.h"

#define RCV_BUFFER_SIZE 512

void ClientListener::run() {
	srand(time(NULL));
	char* buffer = new char[RCV_BUFFER_SIZE];
	int bufferLength;

	sockaddr_in currentClient;
	int clientId;
	int clientAddrLen;
	int n;

	int routerAddrLen = sizeof(routerAddress);

	while (true) {
		if ((bufferLength = recvfrom(routerSocket, buffer, RCV_BUFFER_SIZE, 0, (SOCKADDR*)&currentClient, &clientAddrLen)) == SOCKET_ERROR)
		{
			cerr << "recvfrom(...) failed when getting message" << endl;
			cerr << WSAGetLastError() << endl;
			exit(1);
		}

		memcpy(&clientId, buffer, sizeof(clientId));

		if (clientId == 0) {
			// Assign & register new client ID
			do {
				clientId = rand() % 256;
			} while (clientMap.find(clientId) != clientMap.end());

			clientMap.insert(std::make_pair(clientId, currentClient));

			// Return ID back to client
			if ((n = sendto(routerSocket, (char*)&clientId, sizeof(clientId), 0, (SOCKADDR *)&currentClient, clientAddrLen)) != sizeof(clientId)) {
				std::cerr << "ClientListener: Send error " << endl;
				std::cerr << WSAGetLastError() << endl;
				exit(1);
			}
		} else {
			if ((n = sendto(routerSocket, buffer, bufferLength, 0, (SOCKADDR *)&routerAddress, routerAddrLen)) != bufferLength) {
				std::cerr << "ClientListener: Send error " << endl;
				std::cerr << WSAGetLastError() << endl;
				exit(1);
			}
		}
	}
}