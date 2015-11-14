#include <winsock2.h>
#include <stdio.h>
#include <iostream>

#define PORT_A	11111
//#define PORT_B 22222

#pragma comment (lib,"ws2_32.lib")

using namespace std;

void main(int argc, char **argv)

{
	cout << "the receiver is beginning" << endl;
	WSADATA wsaData; 

	SOCKET socka;	

	//SOCKET sockb;	// 

	int nPortA = PORT_A;

	//int nPortB = PORT_B;

	fd_set rfd;	

	timeval timeout;	

	sockaddr_in addr; 

	char recv_buf[1024];	

	int nRecLen; 

	sockaddr_in cli;	

	int nRet; 

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("failed to laod winsock!/n");
		return;
	}

	socka = socket(AF_INET, SOCK_DGRAM, 0);

	if (socka == INVALID_SOCKET)
	{
		printf("socket()/n");
		return;
	}



	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;	
	addr.sin_port = htons(nPortA); 
	addr.sin_addr.s_addr = htonl(INADDR_ANY); 

	if (bind(socka, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)

	{
		printf("bind()/n");
		return;
	}

	cout << "bind successful" << endl;

	timeout.tv_sec = 30;
	timeout.tv_usec = 0;
	nRet = 0;

	memset(recv_buf, 0, sizeof(recv_buf)); 

	FD_ZERO(&rfd);

	while (true)

	{
		

		// founction select

		FD_SET(socka, &rfd); 

		//FD_SET(sockb, &rfd); 

		nRet = select(0, &rfd, NULL, NULL, &timeout);// Check readability

		memset(recv_buf, 0, sizeof(recv_buf));

		if (nRet == SOCKET_ERROR)

		{
			printf("select() find soket error /n");
			return;
		}
		else if (nRet == 0)	// Time over
		{

			printf("***************time is out***************/n");
			closesocket(socka);
			/*closesocket(sockb);*/
			break;
		}

		else	// Detect there is a readable socket
		{
			if (FD_ISSET(socka, &rfd))	// socka readable

			{
				nRecLen = sizeof(cli);
				int nRecEcho = recvfrom(socka, recv_buf, sizeof(recv_buf), 0, (sockaddr*)&cli, &nRecLen);
				
				if (nRecEcho == INVALID_SOCKET)
				{
					printf("recvfrom()/n");
					break;
				}

				printf("data to port 11111: %s\n", recv_buf);

			}

		}//else

	}//while

	WSACleanup();

	system("pause");

}
