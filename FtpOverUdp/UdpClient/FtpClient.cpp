/*************************************************************************************
*								 File Name	: Client.cpp		   			 	     *          *
*	Usage : Sends request to Server for Uploading, downloading and listing of files  *
**************************************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include "DemoClient.h"
#include <time.h>
#include <stdlib.h>
using namespace std;

/* Can not introduce once introduced cout error exception "cout is ambiguous"*/
int TIMEOUT_USEC = 300;
struct timeval timeout = { 0, 300 };



/**
* Constructor - FtpClient
* Usage: Initialize the connection status
*
* @arg: void
*/
FtpClient::FtpClient()
{
	connectionStatus = true;
	srand(time(NULL));
	clientIdentifier = rand();
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
		cerr << "gethostbyname() failed" << endl;
		return(1);
	}

	/* Return the binary, network byte ordered address */
	return *((unsigned long *)host->h_addr_list[0]);
}

/**
* Function - msgSend
* Usage: Returns the length of bytes in msg_ptr->buffer,which have been sent out successfully
*
* @arg: Msg *
*/
int FtpClient::msgSend(Msg * msg_ptr)
{
	int n;
	int expectedMsgLen = MSGHDRSIZE + msg_ptr->length;
	int addrLength = sizeof(ServAddr);
	if ((n = sendto(clientSock, (char *)msg_ptr, expectedMsgLen, 0, (SOCKADDR *)&ServAddr, addrLength)) != expectedMsgLen) {
		std::cerr << "Send MSGHDRSIZE+length Error " << endl;
		std::cerr << WSAGetLastError() << endl;
		return(-1);
	}

	return (n - MSGHDRSIZE);
}
/**
* Function - msgGet
* Usage: Blocks until the next incoming packet is completely received; returns the packet formatted as a message.
*
* @arg: void
*/
Msg* FtpClient::msgGet()
{
	char buffer[RCV_BUFFER_SIZE];
	int bufferLength;
	int addrLength = sizeof(ServAddr);

	/* Check the received Message Header */
	if ((bufferLength = recvfrom(clientSock, buffer, RCV_BUFFER_SIZE, 0, (SOCKADDR *)&ServAddr, &addrLength)) == SOCKET_ERROR)
	{
		cerr << "recvfrom(...) failed when getting message" << endl;
		cerr << WSAGetLastError() << endl;
		exit(1);
	}

	// must destruct after use!
	Msg* msg = new Msg();
	memcpy(msg, buffer, MSGHDRSIZE);
	memcpy(msg->buffer, buffer + MSGHDRSIZE, msg->length);
	return msg;
}
/**
* Function - getOperation
* Usage: Establish connection and retrieve file from server
*
* @arg: void
*/
void FtpClient::performGet()
{
	string fileName;
	cout << "Type name of file to be retrieved: ";
	getline(cin, fileName);

	sendMsg.type = REQ_GET;
	strcpy(sendMsg.buffer, fileName.c_str());
	sendMsg.length = fileName.length();
	numBytesSent = msgSend(&sendMsg);
	if (numBytesSent == SOCKET_ERROR)
	{
		cout << "Send failed.. Check the Message length.. " << endl;
		return;
	}

	cout << endl << endl << "Sent Request to " << serverName << ", Waiting... " << endl;

	int addrLength = sizeof(ServAddr);
	/* Send the message for windowsize */
	printf("\nSending request for window size.\n");
	sendto(clientSock, send_mes, strlen(send_mes), 0, (SOCKADDR *)& ServAddr, addrLength);

	/*obtaining windowsize.*/
	printf("\nWaiting for the windowsize.\n");
	recvfrom(clientSock, buffer, sizeof(buffer), 0, (SOCKADDR *)&ServAddr, &addrLength);

	Msg* serverMsg = msgGet();
	string fileInPath = string("clientFiles\\").append(fileName);
	ofstream myFile(fileInPath, ios::out | ios::binary);

	int flag, last_num = 0, t = 1;
	while (serverMsg->type != RESP_ERR) {
		/*Perform the receving window*/
		do{
			flag = checkWindow(last_num, ServAddr, t);
			t++;
			send_data(send_mes, flag, last_num);
			memset(buffer, 0, 512);
		} while (flag != 6);
		log(string("Received response with sequence number").append(to_string((_ULonglong)serverMsg->sequenceNumber))); //put the log into checkWindow()
		myFile.write(serverMsg->buffer, serverMsg->length);
		setAckMessage(serverMsg);
		delete serverMsg;

		msgSend(&sendMsg);
		serverMsg = msgGet();
	}

	if (strcmp(serverMsg->buffer, "End of file.") == 0)
		cout << "File received " << endl << endl;
	else
		cout << serverMsg->buffer << endl << endl;

	myFile.close();
	remove(fileName.c_str());
	


}

/*
Receicing data,checking whether it should be put into buffer of file
*/
int FtpClient::checkWindow(int& last_num, sockaddr_in ServAddr, int t)
{
	char buffer[RCV_BUFFER_SIZE];
	int addrLength = sizeof(ServAddr);

	if (t % 5 != 0){
		if (recvfrom(clientSock, buffer, sizeof(buffer), 0, (SOCKADDR *)&ServAddr, &addrLength) != SOCKET_ERROR){
			if (!strcmp(inet_ntoa(ServAddr.sin_addr), inet_ntoa(ServAddr.sin_addr))){
				int cur_num = buffer[1];
				if (buffer[1] <= last_num){
					cout << "Frame" << cur_num << "is duplicate,discarded..." << endl;
					return 1;
				}
				else if (buffer[1] <= last_num + WINDOW_SIZE){
					cout << "Frame" << cur_num << "has been received: " << &buffer[2] << endl;
					strcpy(buff[(buffer[1] - 1) % WINDOW_SIZE], buffer);	

					if (buffer[1] == last_num + 1){
						while (strlen(buff[(last_num) % WINDOW_SIZE])){
							strcpy(networklayer_buff[int(buff[(last_num) % WINDOW_SIZE][1]) - 1], &buff[(last_num) % WINDOW_SIZE][2]);
							cout << "Previous frame" << last_num + 1 << "have been received" << endl << endl;
							memset(buff[(last_num) % WINDOW_SIZE], 0, 256);
							if (++last_num == total)
								return 6;	
						}
						return 1;
					}
					else return 2;
				}
				cout << "Receiced frame" << cur_num << ",but Frame" << last_num + 1 << "lost.Discarded" << endl;
				return 4;
			}
			else {
				cout << "Reject sender" << inet_ntoa(ServAddr.sin_addr) << "'s data" << endl;
				return 5;
			}
		}
		else return 0;
	}
	else {
		if (last_num + 1 <= total){
			while (recvfrom(clientSock, buffer, sizeof(buffer), 0, (SOCKADDR *)&ServAddr, &addrLength) == SOCKET_ERROR);
			cout << "Frame" << last_num + 1 << "lost" << endl;
		}
		return 0;
	}
}

/*
Sending data
*/
int FtpClient::send_data(char send_mes[3], int flag, int last_num)
{
	int addrLength = sizeof(ServAddr);
	if (flag == 1 || flag == 6){
		send_mes[0] = '0';
		send_mes[1] = last_num;
		send_mes[2] = '\0';
	}
	else if (flag == 5){
		send_mes[0] = '4';
		send_mes[1] = 0;
		send_mes[2] = '\0';	
	}

	else if (!flag){
		int i = last_num + 1;
		send_mes[0] = '1';
		send_mes[1] = i;
		send_mes[2] = '\0';
		
		while (sendto(clientSock, send_mes, strlen(send_mes), 0, (SOCKADDR *)&ServAddr, addrLength) == SOCKET_ERROR)
			cout << "Fail to send:" << WSAGetLastError() << endl << "Resend" << endl;
		return 0;
	}
	while (sendto(clientSock, send_mes, strlen(send_mes), 0, (SOCKADDR *) & ServAddr, addrLength) == SOCKET_ERROR)
		cout << "Fail to send" << WSAGetLastError() << endl << "resend。" << endl;

	return flag;
}

void FtpClient::performList()
{
	cout << endl << endl << "Listing directory contents" << endl;
	cout << "==================================================================" << endl;

	sendMsg.type = REQ_LIST;
	sendMsg.length = 0;
	msgSend(&sendMsg);

	Msg* serverMsg = msgGet();
	while (serverMsg->type != RESP_ERR) {
		log(string("Received response with sequence number ").append(to_string((_ULonglong)serverMsg->sequenceNumber)));
		char* currentBuffer = serverMsg->buffer;
		for (int i = 0; i < serverMsg->length; i++)
		if (currentBuffer[i] == '\0')
			cout << endl;
		else
			cout << currentBuffer[i];

		setAckMessage(serverMsg);
		delete serverMsg;

		msgSend(&sendMsg);
		serverMsg = msgGet();
	}

	if (strcmp(serverMsg->buffer, "End of file.") == 0) {
		cout << endl << endl << "End of directory listing" << endl;
		cout << "==================================================================" << endl << endl;
	}
}



void FtpClient::terminate()
{
	sendMsg.type = TERMINATE;
	sendMsg.length = 0;
	msgSend(&sendMsg);

	// Set artificially high wait time to make sure the message is completely flushed prior to exiting.
	Sleep(250);

	exit(0);
}

void FtpClient::log(const std::string &logItem)
{
	FILE *logFile = fopen("logfile.txt", "a");
	fprintf(logFile, "Client (%d): %s\r\n", clientIdentifier, logItem.c_str());
	fclose(logFile);
}

void FtpClient::setAckMessage(Msg* previousPart)
{
	sendMsg.length = 0;
	sendMsg.type = PUT;
	sendMsg.sequenceNumber = previousPart->sequenceNumber;
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
	cout << "2 : LIST " << endl;
	cout << "3 : PUT " << endl;
	cout << "4 : EXIT " << endl;
	cout << "Please select the operation that you want to perform : ";

	/* Check if invalid value is provided and reset if cin error flag is set */
	if (!(cin >> optionVal) || optionVal < 1 || optionVal > 3)
	{
		cout << endl << "Input Types does not match " << endl;
		cin.clear();
	}

	cin.ignore(250, '\n');

	/* Based on the option selected by User, set the transfer type and invoke the appropriate function */
	switch (optionVal)
	{
	case 1:
		performGet();
		break;

	case 2:
		performList();
		break;

	case 3:
		performList();
		break;

	case 4:
		cout << "Terminating... " << endl;
		terminate();
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
bool FtpClient::startClient()
{
	/* Initialize WinSocket */
	if (WSAStartup(0x0202, &wsaData) != 0)
	{
		WSACleanup();
		cerr << "Error in starting WSAStartup()";
		return false;
	}

	/* Get Host Name */
	if (gethostname(hostName, HOSTNAME_LENGTH) != 0)
	{
		cerr << "can not get the host name,program ";
		return false;
	}

	cout << "ftp_tcp starting on host: " << hostName << endl;
	cout << "Type name of ftp server: " << endl;
	getline(cin, serverName);

	return performHandshake();
}

bool FtpClient::performHandshake()
{
	/* Socket Creation */
	if ((clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		std::cerr << "Socket Creation Error; exit" << endl;
		exit(1);
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(0);

	/* Binding the server socket to the Port Number */
	if (::bind(clientSock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		cerr << "Socket Binding Error from FtpThread, exit" << endl;
		exit(1);
	}

	memset(&ServAddr, 0, sizeof(ServAddr));
	ServAddr.sin_family = AF_INET;
	ServAddr.sin_addr.S_un.S_addr = ResolveName(serverName);
	ServAddr.sin_port = htons(HANDSHAKE_PORT);

	Msg* request = getInitialHandshakeMessage();
	msgSend(request);

	struct sockaddr_in newServer;
	Msg* reply = msgGet();
	log(string("Received handshake from server. Message: ").append(reply->buffer));
	delete request;

	request = processFinalHandshakeMessage(reply);
	msgSend(request);
	delete request;
	delete reply;

	return true;
}

Msg* FtpClient::getInitialHandshakeMessage()
{
	Msg* request = new Msg();
	request->type = HANDSHAKE;
	request->length = BUFFER_LENGTH;
	memset(request->buffer, 0, BUFFER_LENGTH);
	strcpy(request->buffer, std::to_string((_ULonglong)clientIdentifier).c_str());

	return request;
}

Msg* FtpClient::processFinalHandshakeMessage(Msg *serverHandshake)
{
	int startIndex = 0;
	for (; startIndex < serverHandshake->length && serverHandshake->buffer[startIndex] != ','; startIndex++);
	startIndex++;

	serverIdentifier = 0;
	Msg* request = new Msg();
	request->type = REQ_LIST;  //change server side to handle the third hand shake,performing list
	request->length = BUFFER_LENGTH;
	memset(request->buffer, 0, BUFFER_LENGTH);
	for (int i = 0; serverHandshake->buffer[startIndex + i] != '\0'; i++) {
		char currentDigit = serverHandshake->buffer[startIndex + i];
		serverIdentifier = serverIdentifier * 10 + (currentDigit - '0');
		request->buffer[i] = currentDigit;
	}

	return request;
}



/**
* Function - send message and check timer
* Usage: Send the message to server,buffer ACKed packet,checking the timer of un ACKed packets
*
* @arg: void
*/
void FtpClient::performPut(){

}



/** PUT
* Send data
* Usage: Perform sent data window in selective repeat protocol 
*
* @arg: int
*/
int FtpClient::send_windows(char send_buf[WINDOW_SIZE][256], int windowsize, int  totalpackets, int  totalframes)
{
	char req[50];
	int  framessend = 0, i = 0, j = 0, k, l, m, n, repacket[40];
	ack acknowledgement;
	frame f1;
	int j = 0;
	int l = 0;                                                    
	while (l<totalpackets)
	{
		//initialising the transmit buffer.
		memset((char*)&f1, 0, sizeof(f1));
		printf("\nInitialising the transmit buffer.\n");
		printf("\nThe frame to be send is %d with packets : \t", framessend);
		//Builting the frame.
		for (m = 0; m<j; m++)
		{
			//including the packets for which negative acknowledgement was received.
			printf("%d  ", repacket[m]);
			f1.packet[m] = repacket[m];
		}

		while (j<windowsize && i<totalpackets)
		{
			printf("%d  ", i);
			f1.packet[j] = i;
			i++;
			j++;
		}
		printf("\nSending frame %d\n", framessend);

		int addrLength = sizeof(ServAddr);
		//sending the frame.
		sendto(clientSock, (char*)&f1, sizeof(f1), 0, (sockaddr*)&ServAddr, addrLength);
		//Waiting for the acknowledgement.
		printf("\nWaiting for the acknowledgement.\n");
		recvfrom(clientSock, (char*)&acknowledgement, sizeof(acknowledgement), 0, (sockaddr*)&ServAddr, &addrLength);
		printf("33[H33[J");

		//Checking acknowledgement of each packet.
		j = 0;
		k = 0;
		m = 0;
		n = l;
		while (m<windowsize && n<totalpackets)
		{
			if (acknowledgement.acknowledge[m] == -1)
			{
				printf("\nNegative acknowledgement received for packet: %d\n", f1.packet[m]);
				k = 1;
				repacket[j] = f1.packet[m];
				j++;
			}
			else
			{
				l++;
			}
			m++;
			n++;
		}

		if (k == 0)
		{
			printf("\nPositive acknowledgement received for all packets within the frame : %d\n", framessend);
		}

		framessend++;
		printf("\nPRESS ENTER TO PROCEED……\n");
		fgets(req, 2, stdin);
		printf("33[H33[J");
	}

	printf("\nAll frames send successfully.\n\nClosing connection with the client.\n");
	closesocket(clientSock);
}



/** PUT
* Selective repeat
* Usage: Selective repeat protocol
*
* @arg: int

int FtpClient::selective_repeat(char send_buf[WINDOW_SIZE][256], int cur_sent)
{
	int num = (cur_sent - 1) % WINDOW_SIZE;
	cout << "Resending Frame" << cur_sent << ":" << &send_buf[num][2] << "..." << endl;
	while (sendto(s, send_buf[num], strlen(send_buf[num]), 0, (SOCKADDR *)&clientSock, sizeof(sockaddr)) == SOCKET_ERROR)
		cout << "Error,fail to sent(sendto():" << WSAGetLastError() << ")" << endl;
	return 1;
}

*/

/** PUT
* Normalize
* Usage: Normalize the data structure
*
* @arg: void

char currentBuffer[256];
void normalize(char send_buf[WINDOW_SIZE][256], int sent_num)
{
	int position = 0;
	int num = 2, i = (sent_num - 1) % WINDOW_SIZE;
	memset(send_buf[i], 0, sizeof(send_buf));
	send_buf[i][0] = '2';
	send_buf[i][1] = sent_num;
	while (currentBuffer[position] && (send_buf[i][num++] = currentBuffer[position++]) != ' ');
	send_buf[i][num] = '\0';
}
*/

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
	FtpClient * tc = new FtpClient();
	if (!tc->startClient())
		return 1;

	while (true)
	{
		tc->showMenu();
	}

	return 0;
}
