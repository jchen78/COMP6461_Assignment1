/*************************************************************************************
*								 File Name	: Client.cpp		   			 	     *          *
*	Usage : Sends request to Server for Uploading, downloading and listing of files  *
**************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include "FtpClient.h"
using namespace std;

/**
 * Constructor - FtpClient
 * Usage: Initialize the connection status 
 *
 * @arg: void
 */
FtpClient::FtpClient()
{
	connectionStatus = true;
}
/**
 * Function - run
 * Usage: Based on the user selected option invokes the appropriate function
 *
 * @arg: void
 */
void FtpClient::run()
{	
	/* Socket Creation */
	if ((clientSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) 
	{
		cerr<<"Socket Creation Error";
		connectionStatus = false;
		return;
	}

	/* Based on the Selected option invoke the appropriate function */
	if(strcmp(transferType.c_str(),"get")==0)
	{
		cin.ignore();
		if(connectionStatus)
		{
			/* Initiate file retrieval */
			sendMsg.type=REQ_GET;
			getOperation();
		}
	}
	else
	{
		cerr<<"Wrong request type";
		return;
	}

}

/**
 * Function - ResolveName
 * Usage: Returns the binary, network byte ordered address
 *
 * @arg: string
 */
unsigned long FtpClient::ResolveName(string name)
{
	struct hostent *host;            /* Structure containing host information */

	if ((host = gethostbyname(name.c_str())) == NULL)
	{
		cerr<<"gethostbyname() failed"<<endl;
		return(1);
	}

	/* Return the binary, network byte ordered address */
	return *((unsigned long *) host->h_addr_list[0]);
}

/**
 * Function - msgSend
 * Usage: Returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
 *
 * @arg: int, Msg *
 */
int FtpClient::msgSend(int clientSocket,Msg * msg_ptr)
{
	int len;
	if((len=sendto(clientSocket,(char *)msg_ptr,MSGHDRSIZE+msg_ptr->length,0))!=(MSGHDRSIZE+msg_ptr->length))
	{
		cerr<<"Send MSGHDRSIZE+length Error";
		return(1);
	}
	/*Return the length of data in bytes, which has been sent out successfully */
	return (len-MSGHDRSIZE);

}

/**
 * Function - getOperation
 * Usage: Establish connection and retrieve file from server
 *
 * @arg: void
 */
void FtpClient::getOperation()
{ 
	/* Socket creation */
	if ((clientSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) //create the socket
	{
		cerr<<"Socket Creation Error";
		return;
	}
	/* Establish connection with Server */
	ServPort=REQUEST_PORT;
	memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
	ServAddr.sin_family      = AF_INET;             /* Internet address family */
	ServAddr.sin_addr.s_addr = ResolveName(serverIpAdd);   /* Server IP address */
	ServAddr.sin_port        = htons(ServPort); /* Server port */
	if (connect(clientSock, (struct sockaddr *) &ServAddr, sizeof(ServAddr)) < 0)
	{
		cerr<<"Socket Connection Error " << endl;
		return;
	}
	/* Get the hostname */
	if(gethostname(reqMessage.hostname,HOSTNAME_LENGTH)!=0) 
	{
		cerr << "can not get the host name " <<endl;
		return;
	}
	cout <<"Type name of file to be retrieved: "<<endl;
	getline (cin,fileName);
	strcpy(reqMessage.filename,fileName.c_str());
	memcpy(sendMsg.buffer,&reqMessage,sizeof(reqMessage));
	/* Include the length of the buffer */
	sendMsg.length=sizeof(sendMsg.buffer);
	cout << endl << endl << "Sent Request to " << serverIpAdd << ", Waiting... " << endl;
	/* Send the packed message */
	numBytesSent = msgSend(clientSock, &sendMsg);
    if (numBytesSent == SOCKET_ERROR)
	{
		cout << "Send failed.. Check the Message length.. " << endl;     
		return;
	}

	ofstream myFile (fileName, ios::out | ios::binary);
	/* Retrieve the contents of the file and write the contents to the created file */
	while((numBytesRecv = recvfrom(clientSock,receiveMsg.buffer,BUFFER_LENGTH,0))>0)
	{
		/* If the file does not exist in the server, close the connection and exit */
		if(strcmp(receiveMsg.buffer, "No such file") == 0)
		{
			cout << receiveMsg.buffer << endl;
			myFile.close();
			remove(fileName.c_str());
			closesocket(clientSock);
			return;
		}
		else /* If the file exists, start reading the contents of the file */
		{
			myFile.write (receiveMsg.buffer, numBytesRecv);
			memset (receiveMsg.buffer, 0,sizeof(receiveMsg.buffer));
		}
    }
	/* Close the connection after the file is received */
    cout << "File received "<< endl << endl;
	myFile.close();
	closesocket(clientSock);
}

/**
 * Function - showMenu
 * Usage: Display the Menu with options for the User to select based on the operation
 *
 * @arg: void
 */
void FtpClient::showMenu()
{
	int optionVal;
	cout << "1 : GET " << endl;
	cout << "2 : EXIT " << endl;
	cout << "Please select the operation that you want to perform : ";
	/* Check if invalid value is provided and reset if cin error flag is set */
	if(!(cin >> optionVal))
	{
		cout << endl << "Input Types does not match " << endl;
		cin.clear();
		cin.ignore(250, '\n');
	}
	/* Based on the option selected by User, set the transfer type and invoke the appropriate function */
	switch (optionVal)
	{
		case 1:
			transferType = "get";
			run();
			break;

		case 2:
			cout << "Terminating... " << endl; 
			exit(1);
			break;

		default:
			cout << endl << "Please select from one of the above options " << endl;
			break;
	}
	cout << endl;
}

/**
 * Function - startClient
 * Usage: Initialize WinSocket and get the host name and server IP Address to connnect
 *
 * @arg: void
 */
void FtpClient::startClient()
{
	/* Initialize WinSocket */
	if (WSAStartup(0x0202,&wsaData)!=0)
	{
		WSACleanup();
	    cerr<<"Error in starting WSAStartup()";
		return;
	}

	/* Get Host Name */
	if(gethostname(hostName,HOSTNAME_LENGTH)!=0) 
	{
		cerr<<"can not get the host name,program ";
		return;
	}
	cout <<"ftp_tcp starting on host: "<<hostName<<endl;
	cout <<"Type name of ftp server: "<<endl;
	getline (cin,serverIpAdd);
}

/**
 * Destructor - ~FtpClient
 * Usage: DeAllocate the allocated memory
 *
 * @arg: void
 */
FtpClient::~FtpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and return; */
	WSACleanup();
}

/**
 * Function - main
 * Usage: Initiates the Client
 *
 * @arg: int, char*
 */
int main(int argc, char *argv[])
{
	FtpClient * tc=new FtpClient();
	tc->startClient();
	while(1)
	{
		tc->showMenu();
	}
		
	return 0;
}
