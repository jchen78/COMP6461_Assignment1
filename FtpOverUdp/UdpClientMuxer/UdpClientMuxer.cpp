// UdpClientMuxer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <map>
#include <winsock.h>
#include <Common.h>
#include "ClientListener.h"
#include "RouterListener.h"

#define CLIENT_PORT 5002
#define FROM_ROUTER_PORT 5001
#define TO_ROUTER_PORT 7001

using namespace std;

unsigned long ResolveName(string name)
{
	struct hostent *host;            /* Structure containing host information */

	if ((host = gethostbyname(name.c_str())) == NULL)
	{
		cerr << "gethostbyname() failed" << endl;
		return(1);
	}

	/* Return the binary, network byte ordered address */
	return *((unsigned long *)host->h_addr_list[0]);
}

int createSocket(int listeningPort)
{
	int newSocket;
	if ((newSocket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		std::cerr << "Socket Creation Error,exit" << endl;
		exit(1);
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(listeningPort);

	/* Binding the server socket to the Port Number */
	if (::bind(newSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		cerr << "Socket Binding Error from FtpThread, exit" << endl;
		exit(1);
	}

	return newSocket;
}

int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsadata;
	/* Initialize Windows Socket information */
	if (WSAStartup(0x0202, &wsadata) != 0)
	{
		cerr << "Starting WSAStartup() error\n" << endl;
		exit(1);
	}

	/* Display the name of the host */
	char muxerName[20];
	if (gethostname(muxerName, 20) != 0)
	{
		cerr << "Get the host name error,exit" << endl;
		exit(1);
	}

	cout << "Client multiplexer starting on host " << muxerName << endl;

	string hostname;
	cout << "Please enter the router host name: ";
	getline(cin, hostname);

	int clientSocket = createSocket(CLIENT_PORT); // Socket for packets coming from clients to the muxer.
	int fromRouterSocket = createSocket(FROM_ROUTER_PORT); // Socket for packets coming from router to muxer.

	struct sockaddr_in routerAddr; // Address for messages from muxer to router
	memset(&routerAddr, 0, sizeof(routerAddr));
	routerAddr.sin_family = AF_INET;
	routerAddr.sin_addr.S_un.S_addr = ResolveName(hostname);
	routerAddr.sin_port = htons(TO_ROUTER_PORT);

	ClientListener cl = ClientListener(clientSocket, fromRouterSocket, routerAddr);
	RouterListener rl = RouterListener(fromRouterSocket, clientSocket, cl.getMap());
	rl.start();
	cl.start();

	cout << "Client multiplexer now operational on host " << muxerName << endl;
	cout << "Target set to host " << hostname << endl;

	while (true);

	return 0;
}