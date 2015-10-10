/*************************************************************************************
*								 File Name	: Client.cpp		   			 	     *          *
*	Usage : sendtos request to Server for Uploading, downloading and listing of files  *
**************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#define CHUNK_SIZE (64*1024) 
#include "Client.h"
#include <time.h>
#include <afx.h>



using namespace std;


/*Create random ack and syn number*/

int createRand(){
	srand((unsigned)time(NULL));
	int num = rand() % 256;
	return num;
}




/*Create first sequnce number by using existing ack*/
int getFirstSequnceNumber(int i){
	if (i % 2 == 1){
		return 1;
	}
	else return 0;
}




/**
* Function - main
* Usage: Initiates the Client
*
* @arg: int, char*
*/
int main(int argc, char *argv[])
{
	UdpClient * tc = new UdpClient();
	/*Initialize the client and establish threeWayHandShake*/
	tc->startClient();
	tc->threeWayHandShake();

	/*Main founction*/
	while (1)
	{
		tc->showMenu();
	}

	return 0;
}


/**
* Function - startClient
* Usage: Initialize WinSocket and get the host name and server IP Address to connnect
*
* @arg: void
*/
void UdpClient::startClient()
{
	/* Initialize WinSocket */
	if (WSAStartup(0x0202, &wsaData) != 0)
	{
		WSACleanup();
		cerr << "Error in starting WSAStartup()";
		return;
	}

	/* Get Host Name */
	if (gethostname(hostName, HOSTNAME_LENGTH) != 0)
	{
		cerr << "can not get the host name,program ";
		return;
	}
	cout << "ftp_Udp starting on host: " << hostName << endl;
	cout << "Type name of ftp server: " << endl;
	getline(cin, serverIpAdd);



}

/**
* Constructor - UdpClient
* Usage: Initialize the connection status
*
* @arg: void
*/
UdpClient::UdpClient()
{

	/* Socket Creation */
	if ((clientSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		cerr << "Socket Creation Error";
		connectionStatus = false;
		return;
	}


	/* Establish connection with Server */
	ServPort = REQUEST_PORT;
	memset(&ServAddr, 0, sizeof(ServAddr));     /* Zero out structure */
	ServAddr.sin_family = AF_INET;             /* Internet address family */
	ServAddr.sin_addr.s_addr = ResolveName(serverIpAdd);   /* Server IP address */
	ServAddr.sin_port = htons(ServPort); /* Server port */


	connectionStatus = true;

}


/**
* Function - threeWayHandShake
* Usage: Establish and realize ThreeWayHandShake with server
*
* @arg: void
*/
void UdpClient::threeWayHandShake()
{
	/*Create the socket*/
	if ((clientHandShakeSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		cerr << "Socket Creation Error";
		connectionStatus = false;
		;
	}

	/* Establish connection with Server */
	ServPort = REQUEST_PORT;
	memset(&ServAddr, 0, sizeof(ServAddr));    /* Zero out structure */
	ServAddr.sin_family = AF_INET;             /* Internet address family */
	ServAddr.sin_addr.s_addr = ResolveName(serverIpAdd);   /* Server IP address */
	ServAddr.sin_port = htons(ServPort);       /* Server port */


	Msg *handShakeMessage = new Msg();



	/*Client send request message to the server*/
	memcpy(handShakeMessage, &(syn = createRand()), sizeof(syn = createRand()));
	if (msgsendto(clientHandShakeSock, handShakeMessage) == SOCKET_ERROR)
	{
		cerr << "Client three way hand shake request error";
		exit(1);
	}


	/*Client receive response message from server*/
	if (msgGetResponse(clientHandShakeSock, ServAddr) == NULL)
	{
		cerr << "Client three way hand shake get response error";
		exit(1);
	}

	/*Check the ACK from server,if false send acknowledge number to the server*/
	if (handShakeMessage != msgGetResponse(clientHandShakeSock, ServAddr))
	{
		cerr << "ACK can not match SYN ";
		exit(1);
	}
	else
	{
		memset(handShakeMessage, '/0', sizeof(*handShakeMessage));
		handShakeMessage = msgGetResponse(clientHandShakeSock, ServAddr);
		/*Store the binary number of ACK sent from client to server*/
		sequenceNumber = (int)handShakeMessage->buffer;
		msgsendto(clientHandShakeSock, handShakeMessage);
		exit(0);
	}


}


/**
* Function - showMenu
* Usage: Display the Menu with options for the User to select based on the operation
*
* @arg: void
*/
void UdpClient::showMenu()
{
	int optionVal;
	cout << "1 : GET " << endl;
	cout << "2 : PUT " << endl;
	cout << "3 : LIST " << endl;
	cout << "4 : EXIT " << endl;
	cout << "Please select the operation that you want to perform : ";
	/* Check if invalid value is provided and reset if cin error flag is set */
	if (!(cin >> optionVal))
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
		get();
		break;

	case 2:
		transferType = "put";
		put();
		break;

	case 3:
		transferType = "list";
		list();
		break;


	case 4:
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
* Function - get
* Usage: Based on the user selected option invokes the appropriate function
*
* @arg: void
*/
void UdpClient::get()
{



	/* Based on the Selected option invoke the appropriate function */
	if (strcmp(transferType.c_str(), "get") == 0)
	{
		cin.ignore();
		if (connectionStatus)
		{
			/* Initiate file retrieval */
			sendtoMsg.type = REQ_GET;
			getDataOperation();
		}
	}
	else
	{
		cerr << "Wrong request type";
		return;
	}

}


/**
* Destructor - ~UdpClient
* Usage: DeAllocate the allocated memory
*
* @arg: void
*/
UdpClient::~UdpClient()
{
	/* When done uninstall winsock.dll (WSACleanup()) and return; */
	WSACleanup();
}





/**
* Function - ResolveName
* Usage: Returns the binary, network byte ordered address
*
* @arg: string
*/
unsigned long UdpClient::ResolveName(string name)
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

/**
* Function - sendMsgto
* Usage: Returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*
* @arg: int, Msg *
*/
int UdpClient::msgsendto(int clientSocket, Msg * msg_ptr)
{


	int len;
	if ((len = sendto(clientSocket, (char *)msg_ptr, MSGHDRSIZE + msg_ptr->length, 0, (SOCKADDR *)&ServAddr, addrLength)) != (MSGHDRSIZE + msg_ptr->length))
	{
		cerr << "sendto MSGHDRSIZE+length Error";
		return(1);
	}
	/*Return the length of data in bytes, which has been sent out successfully */
	return (len - MSGHDRSIZE);

}

/**
* Function - getResponseMsg
* Usage: Blocks until the next incoming response is completely received; returns the response.
* @arg: void
*/
Msg* UdpClient::msgGetResponse(SOCKET sock, struct sockaddr_in ServAddr)
{
	char buffer[512];
	int bufferLength;
	memcpy(ServAddr.sin_zero, ServAddr.sin_zero, 8);
	addrLength = sizeof(ServAddr);

	/* Check the received Message Header */
	if ((bufferLength = recvfrom(sock, buffer, BUFFER_LENGTH, 0, (SOCKADDR *)&ServAddr, &addrLength)) == SOCKET_ERROR)
	{
		cerr << "recvfrom(...) failed when getting message" << endl;
		exit(1);
	}

	// destructor 
	Msg* response = new Msg();
	memcpy(response, buffer, MSGHDRSIZE);
	memcpy(response->buffer, buffer + MSGHDRSIZE, response->length);
	return response;
}



/**
* Function - getDataOperation
* Usage: Establish connection and retrieve file from server
*
* @arg: void
*/
void UdpClient::getDataOperation()
{
	Msg *getData = new Msg();

	/* Get the hostname */
	if (gethostname(reqMessage.hostname, HOSTNAME_LENGTH) != 0)
	{
		cerr << "can not get the host name " << endl;
		return;
	}
	cout << "Type name of file to be retrieved: " << endl;
	getline(cin, fileName);
	strcpy(reqMessage.filename, fileName.c_str());
	memcpy(sendtoMsg.buffer, &reqMessage, sizeof(reqMessage));
	/* Include the length of the buffer */
	sendtoMsg.length = sizeof(sendtoMsg.buffer);
	cout << endl << endl << "Sent Request to " << serverIpAdd << ", Waiting... " << endl;
	/* sendto the packed message */
	numBytesSent = msgsendto(clientSock, &sendtoMsg);
	if (numBytesSent == SOCKET_ERROR)
	{
		cout << "sendto failed.. Check the Message length.. " << endl;
		return;
	}

	ofstream myFile(fileName, ios::out | ios::binary);
	/* Retrieve the contents of the file and write the contents to the created file */
	while ((numBytesRecvfrom = recvfrom(clientSock, receiveMsg.buffer, BUFFER_LENGTH, 0, (SOCKADDR *)&ServAddr, &addrLength))>0)
	{
		/* If the file does not exist in the server, close the connection and exit */
		if (strcmp(receiveMsg.buffer, "No such file") == 0)
		{
			cout << receiveMsg.buffer << endl;
			myFile.close();
			remove(fileName.c_str());
			closesocket(clientSock);
			return;
		}
		else /* If the file exists, start reading the contents of the file */
		{
			myFile.write(receiveMsg.buffer, numBytesRecvfrom);
			memset(receiveMsg.buffer, 0, sizeof(receiveMsg.buffer));
			getData->ACK = sequenceNumber % 2 + 1;
			sequenceNumber +=
				msgsendto(clientSock, getData);
		}
	}
	/* Close the connection after the file is received */
	cout << "File received " << endl << endl;
	myFile.close();
	closesocket(clientSock);
}

/**
* Function - put
* Usage: upload data to server
* Reference:http://blog.csdn.net
* @arg: void
*/
void UdpClient::putDataOperation()
{
	DWORD   GetFileProc(COMMAND command, SOCKET client);

	COMMAND  cmd;
	FILEINFO fi;
	memset((char*)&fi, 0, sizeof(fi));
	memset((char*)&cmd, 0, sizeof(cmd));
	cout << "Type name of file to be uploaded: " << endl;
	getline(cin, fileName);
	/*cmd.ID = fileName;*/


	CFile file;
	int nChunkCount = 0; //Number of chunks

	if (file.Open((char*)cmd.lparam, CFile::modeRead | CFile::typeBinary))//Open file
	{
		int FileLen = file.GetLength(); // Read file lengh
		fi.FileLen = file.GetLength();
		strcpy((char*)fi.FileName, file.GetFileName()); //  Get file name
		memcpy((char*)&cmd.lparam, (char*)&fi, sizeof(fi));
		send(clientSock, (char*)&cmd, sizeof(cmd), 0); //Send file name and lengh

		nChunkCount = FileLen / CHUNK_SIZE; // Chunk numbers of file
		if (FileLen%nChunkCount != 0)
			nChunkCount++;
		char *date = new char[CHUNK_SIZE]; // Create buffer
		for (int i = 0; i<nChunkCount; i++) //Sent chunks 
		{
			int nLeft;
			if (i + 1 == nChunkCount) // The last chunk
				nLeft = FileLen - CHUNK_SIZE*(nChunkCount - 1);
			else
				nLeft = CHUNK_SIZE;
			int idx = 0;
			file.Read(date, CHUNK_SIZE); // Read the file
			while (nLeft>0)
			{
				int ret = send(clientSock, &date[idx], nLeft, 0);//Send the file
				if (ret == SOCKET_ERROR)
				{
					break;
				}
				nLeft -= ret;
				idx += ret;
			}
		}
		file.Close();
		delete[] date;
	}

}

}

