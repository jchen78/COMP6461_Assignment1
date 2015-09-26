/*************************************************************************************
*								 File Name	: Server.cpp		   			 	     *
*	Usage : Handles Client request for Uploading, downloading and listing of files   *
**************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include <winsock.h>
#include <iostream>
#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <string>
#include <fstream>
#include <process.h>
#include "Thread.h"
#include "FtpServer.h"

using namespace std;

/**
 * Constructor - FtpServer
 * Usage: Initialize the socket connection 
 *
 * @arg: void
 */
FtpServer::FtpServer()
{
	WSADATA wsadata;
	/* Initialize Windows Socket information */
	if (WSAStartup(0x0202,&wsadata)!=0)
	{
		cerr << "Starting WSAStartup() error\n" << endl;
		exit(1);
	}		

	/* Display the name of the host */
	if(gethostname(serverName,HOSTNAME_LENGTH)!=0) 
	{
		cerr << "Get the host name error,exit" << endl;
		exit(1);
	}

	cout << "Server: " << serverName << " waiting to be contacted for get/put request..." << endl;

	/* Socket Creation */
	if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		std::cerr << "Socket Creation Error,exit" << endl;
		exit(1);
	}

	/* Fill-in Server Port and Address information */
	ServerPort=REQUEST_PORT;
	memset(&ServerAddr, 0, sizeof(ServerAddr));      /* Zero out structure */
	ServerAddr.sin_family = AF_INET;                 /* Internet address family */
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
	ServerAddr.sin_port = htons(ServerPort);         /* Local port */

	/* Binding the server socket to the Port Number */
    if (::bind(serverSock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
	{
		cerr << "Socket Binding Error,exit" << endl;
		exit(1);
	}

	//Successfull bind, now listen for Server requests.
	if (listen(serverSock, MAXPENDING) < 0)
	{
		cerr << "Socket Listening Error,exit" << endl;
		exit(1);
	}
}

/**
 * Destructor - ~FtpServer
 * Usage: DeAllocate the allocated memory
 *
 * @arg: void
 */
FtpServer::~FtpServer()
{
	WSACleanup();
}

/**
 * Function - start
 * Usage: Listen and handle the requests from clients
 *
 * @arg: void
 */
void FtpServer::start()
{
	for (;;) /* Run forever */
	{
		/* Set the size of the result-value parameter */
		clientLen = sizeof(ServerAddr);

		/* Accept the connection from client */
		if ((clientSock = accept(serverSock, (struct sockaddr *) &ClientAddr,
			&clientLen)) < 0)
		{
			cerr << "Connection Accept Failed ,exit" << endl;
			exit(1);
		}

        /* Create a Thread for new connection and run*/
		FtpThread * pt=new FtpThread(clientSock);
		pt->start();
	}
}

/*-------------------------------FtpThread Class--------------------------------*/
/**
 * Function - ResolveName
 * Usage: Returns the binary, network byte ordered address
 *
 * @arg: char []
 */
unsigned long FtpThread::ResolveName(char name[])
{
	struct hostent *host;            /* Structure containing host information */

	if ((host = gethostbyname(name)) == NULL)
	{
		std::cerr << "gethostbyname() failed" << endl;
	}

	/* Return the binary, network byte ordered address */
	return *((unsigned long *) host->h_addr_list[0]);
}

/**
 * Function - msgRecv
 * Usage: Returns the length of bytes in msg_ptr->buffer,which have been recevied successfully
 *
 * @arg: int, Msg *
 */
int FtpThread::msgRecv(int sock,Msg * msg_ptr)
{
	int rbytes,n;

	/* Check the received Message Header */
	for(rbytes=0;rbytes<MSGHDRSIZE;rbytes+=n)
	{
		if((n=recv(sock,(char *)msg_ptr+rbytes,MSGHDRSIZE-rbytes,0))<=0)
		{
			std::cerr << "Received MSGHDR Error" << endl;
			return (-1);
		}
	}

	/* Check the received Message Buffer */
	for(rbytes=0;rbytes<msg_ptr->length;rbytes+=n)
	{
		if((n=recv(sock,(char *)msg_ptr->buffer+rbytes,msg_ptr->length-rbytes,0))<=0)
		{
			std::cerr << "Recevier Buffer Error" << endl;
			return (-1);
		}
	}
	/*  Return the length of bytes in msg_ptr->buffer,which have been recevied successfully */
	return msg_ptr->length;
}

/**
 * Function - msgSend
 * Usage: Returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 *
 * @arg: int, Msg *
 */
int FtpThread::msgSend(int sock,Msg * msg_ptr)
{
	int n;
	if((n=send(sock,(char *)msg_ptr,MSGHDRSIZE+msg_ptr->length,0))!=(MSGHDRSIZE+msg_ptr->length))
	{
		std::cerr << "Send MSGHDRSIZE+length Error " << endl;
		return(-1);
	}
	/* Return the length of bytes in msg_ptr->buffer,which have been sent out successfully */
	return (n-MSGHDRSIZE);
}

/**
 * Function - sendFileData
 * Usage: Transfer the requested file to client
 *
 * @arg: char[]
 */
void FtpThread::sendFileData(char fName[20])
{	
	Msg sendMsg;
	Resp responseMsg;
	int numBytesSent = 0;
	ifstream fileToRead;
	int result;
	struct _stat stat_buf;
	/* Lock the code section */


	memset(responseMsg.response,0,sizeof(responseMsg));
	/* Check the file status and pack the response */
	if((result = _stat(fName,&stat_buf))!=0)
	{
		strcpy(responseMsg.response,"No such file");
		memset(sendMsg.buffer,'\0',BUFFER_LENGTH);
		memcpy(sendMsg.buffer,&responseMsg,sizeof(responseMsg));
		/* Send the contents of file recursively */
		if((numBytesSent = send(serverSocket,sendMsg.buffer,sizeof(responseMsg),0))==SOCKET_ERROR)
		{
			cout << "Socket Error occured while sending data " << endl;
			/* Close the connection and unlock the mutex if there is a Socket Error */
			closesocket(serverSocket);
			
			return;
		}
		else 
		{    
			/* Reset the buffer */
			memset(sendMsg.buffer,'\0',sizeof (sendMsg.buffer));
		}
	}
	else
	{
		fileToRead.open(fName, ios::in | ios::binary);
		if(fileToRead.is_open()) 
		{
			while(!fileToRead.eof())
			{
				memset(sendMsg.buffer,'\0',BUFFER_LENGTH);
				/* Read the contents of file and write into the buffer for transmission */
				fileToRead.read (sendMsg.buffer, BUFFER_LENGTH);
				/* Transfer the content to requested client */
				if((numBytesSent = send(serverSocket,sendMsg.buffer,BUFFER_LENGTH,0))==SOCKET_ERROR)
				{
					cout << "Socket Error occured while sending data " << endl;
					/* Close the connection and unlock the mutex if there is a Socket Error */
					closesocket(serverSocket);
					
					return;
				}
				else 
				{   
					/* Reset the buffer and use the buffer for next transmission */
					memset(sendMsg.buffer,'\0',sizeof (sendMsg.buffer));
				}
			}
			cout << "File transferred completely... " << endl;
		}
		fileToRead.close();
	}
	/* Close the connection and unlock the Mutex after successful transfer */
	closesocket(serverSocket);
}

/**
 * Function - run
 * Usage: Based on the requested operation, invokes the appropriate function
 *
 * @arg: void
 */
void FtpThread::run()
{
	Req *requestPtr; //Pointer to the Request Packet
	Msg receiveMsg; //send_message and receive_message format
	/*Start receiving the request */
	if(msgRecv(serverSocket,&receiveMsg)!=receiveMsg.length)
	{
		cerr << "Receive Req error,exit " << endl;
		return;
	}
	requestPtr = (Req *)receiveMsg.buffer;
	/* Check the type of operation and Construct the response and send to Client */
	if(receiveMsg.type==REQ_GET)
	{
		cout <<"User " << requestPtr->hostname <<" requested file "<< requestPtr->filename << " to be sent" << endl;
		/* Transfer the requested file to Client */
		sendFileData(requestPtr->filename);
	}

}

/**
 * Function - main
 * Usage: Initiates the Server
 *
 * @arg: void
 */
int main(void)
{
	FtpServer ts;
	/* Start the server and start listening to requests */
	ts.start();

	return 0;
}


