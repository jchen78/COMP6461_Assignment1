#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define SERVER_PORT_A 11111	// Server port A
//#define SERVER_PORT_B 22222	// Server port B

#pragma comment (lib,"ws2_32.lib")

using namespace std;

typedef struct tagSERVER	
{
	char* ip;	
	int nPort;	
} SERVER, *PSERVER;


int main(int argc, char **argv)
{
	int i;

	WSADATA wsaData;	
	SOCKET sClient;	

	cout << "the client begin" << endl;

	char send_buf[] = "Test timer demo!  Hello world!";	

	int nSend; 

	SERVER sers[] = { "127.0.0.1", SERVER_PORT_A };

	int nSerCount = 4; //send the times to the server 

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)	
	{
	  printf("failed to start up socket!/n");
		return 0;
	}



	sClient = socket(AF_INET, SOCK_DGRAM, 0);

	if (sClient == INVALID_SOCKET)
	{
		printf("socket() is invalid socket/n");
    	return 0;
	}

	sockaddr_in ser;	

	ser.sin_family = AF_INET;	
	ser.sin_port = htons(11111);	
	ser.sin_addr.s_addr = inet_addr("127.0.0.1");	/

	int nLen = sizeof(ser);	
	int t = sizeof(send_buf);

	for (i = 0; i < nSerCount; i++)
	{

		nSend = sendto(sClient, (char *)send_buf, sizeof(send_buf)+1, 0, (sockaddr*)&ser, nLen);
		cout << "the counter is " << i << endl;
		if (nSend == 0)	// Sending fail
		{
			return 0;
		}
		else if (nSend == SOCKET_ERROR)	// Socket error
		{
			printf("sendto()/n");
			return 0;
		}

	}

	closesocket(sClient);	

	WSACleanup(); 

	return 0;





}
