// Author : Dr. Foster
// Purpose : desmonstration of winsock API using a simple server
//
//
//
//  Citation : Based off of sample code found at https://www.binarytides.com/winsock-socket-programming-tutorial/
//  Reference : http://beej.us/guide/bgnet/ has a decent explanation of the concepts, just be warned that the sample
//  code is intended for Linux and needs occasional tweaks to run on Windows.

#include <stdio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <process.h>  // Windows multithreading functions
#include <stdio.h>    // needed to print to the screen
#include <stdlib.h>   // ascii to integer conversion
#include <time.h>


#pragma comment(lib,"ws2_32.lib") //Winsock Library - don't touch this line.

// create some constants for error codes if the program dies...
#define ERROR_WINSOCK_INIT_FAILURE				1;
#define ERROR_WINSOCK_SOCKET_CREATE_FAILURE		2;
#define ERROR_WINSOCK_SOCKET_CONNECT_FAILURE	3;
#define ERROR_WINSOCK_SOCKET_SEND_FAILURE       4;
#define ERROR_WINSOCK_SOCKET_BIND_FAILURE       5;

struct sockaddr_in socketServer, socketClient;   // sockets are used to access the network
char * msgSend;
#define MSGRECVLENGTH 100
char msgRecv[MSGRECVLENGTH];
CHAR ipv4addr[INET_ADDRSTRLEN] = "127.0.0.1";  // hard-coded just to test functionality
//NOTE - If your project is using Unicode instead of ASCII, you will probably need to change CHAR to WCHAR in the line above

typedef struct
{
	SOCKET soc;
	int connections;
	int test;
}thread_data;

#define MSGSENDLENGTH 100		   
CHAR message[MSGSENDLENGTH];
#define MAX_THREADS 5
SOCKET s;

DWORD WINAPI workerThread(void *pMyThreadData)
{
	thread_data* myThreadData = pMyThreadData;

	if (myThreadData->soc == INVALID_SOCKET)
	{
		printf("Accept failed with error code : %d", WSAGetLastError());
		_endthreadex(-1);
		return ERROR_WINSOCK_SOCKET_CONNECT_FAILURE; // Something went wrong, so the code will loop back to the accept without doing anything.
	}

	{  // Again, ugly looking function to convert the client's IP address as a 32-bit number to printable text
		InetNtop(AF_INET, &(socketClient.sin_addr), (PSTR)ipv4addr, INET_ADDRSTRLEN);    // Again, ugly looking function to convert the client's IP address as a 32-bit number to printable text 
		printf("Accepted a connection from IP address %s port %d...\n", ipv4addr, ntohs(socketClient.sin_port));
	}
	// At this point, the server has two sockets open. Since it probably needs to send and receive data with s_new to talk to the client, it can't accept another connection,
	// since both recv() and accept() block. This is the spot in the code to create a new thread and pass it s_new, and the thread code can send and recv. The main program
	// can then loop back and try to accept() a new call.
	//
	// Since the main focus isn't multithreading calls, create a fixed number of threads (at least 5), and sequentially create them as new calls come in.
	// Once the last thread has been created, this main program should print a message that it is out of threads, and the user should wait to close
	// the main program until the clients are done.
	//
	// This section should be moved to the thread function. Note that the sequence of send() and recv() functions will be dictated by the 
	// sequence of messages the program needs to exchange. In this example, once the connection is established, the client sends first.

	sprintf_s(message, MSGSENDLENGTH, "You are connection %d.\0", myThreadData->connections); // the sprintf is like printf, but the string goes into
	// a variable (the first arugument message), and the _s means this is the safe version that limits the copied characters to the 
	// value in the second argument MSGSENDLENGTH)
	//send(s_new, message, (int)strlen(message), 0);
	
	if (send(myThreadData->soc, message, (int)strlen(message), 0) < 0) // negative return values indicate an error. For this demo/project, we don't need to troubleshoot, so just "scream and die" 
	{
		wchar_t *s = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&s, 0, NULL);

		printf("Error sending message, error message:\n");
		printf("%S\n", s);
		//return ERROR_WINSOCK_SOCKET_SEND_FAILURE;
		_endthreadex(-1);
		return ERROR_WINSOCK_SOCKET_SEND_FAILURE;
	}

	unsigned int bytesRecv;
	printf("The server will block here until a message is received...\n");
	bytesRecv = recv(myThreadData->soc, msgRecv, MSGRECVLENGTH, 0);  // blocks here until something is received on the socket.
	if(bytesRecv < MSGRECVLENGTH)
		msgRecv[bytesRecv] = 0;
	// The return value is the count of the number of bytes. The characters are stored in the array msgRecv, and it will read at most MSGRECVLENGTH characters
	

	// The sample chatClient has a 5 second pause after connecting to show that this program is waiting on recv().

#if 1
	if ((bytesRecv == 1) && (msgRecv[0] == 'Q'))
	{
		printf("Closing up the sockets and exiting gracefully...\n");
		closesocket(myThreadData->soc);
		Sleep(5000);
		closesocket(s);
		WSACleanup();
		exit(0);
	}
#endif // 0

	else if (bytesRecv > 0)
	{
		printf("%s\n", msgRecv);
	}
	int n = 0;
	int max = 0;
	sscanf_s(msgRecv, "%d,%d",&max, &n );

	//generate a random number and send it to the client
	// TODO: clean up reading responce from client and return number.
	srand((int)time(NULL));
	for (int i = 1; i <= n; i++)
	{
		int randumNumber = 1 + (rand() % max);

		sprintf_s(message, MSGSENDLENGTH, "your roll:%d\n\0", randumNumber);
		send(myThreadData->soc, message, (int)strlen(message), 0);
	}
	

	closesocket(myThreadData->soc); // This versions receives only one message per socket, closes it, and waits to accept a new one.
	free(pMyThreadData);
	_endthreadex(0);
	return 0;
}

int main(int argc, char *argv[])
{
	// In traditional C, all variables must be declaring in a code block before any executable code. In C++, they can (and should) be declared right before their first use.
	WSADATA wsa;
	SOCKET s_new;
	int c;
	int connections = 0;
	HANDLE  hThreadArray[MAX_THREADS];

	unsigned int port;
	sscanf_s(argv[1], "%u", &port);

// CE-480 No need to change anything from here....
	// Before using Winsock calls on Windows, the Winsock library needs to be initialized...     
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Winsock error Code : %d", WSAGetLastError());
		return ERROR_WINSOCK_INIT_FAILURE;
	}

	/* The socket() call takes three arguments. The first is the network protocol "Address Family", hence the AF_prefix.
	The two most common are AF_INET for IPv4 and AF_INET6 for IPv6. The next asks for the port type, which is usually a
	TCP port with SOCK_STREAM, or a UDP port with SOCK_DGRAM. The third parameter is the specific protocol, such as ICMP,
	IGMP, or for the purposes of this chat program, TCP, which uses the constant IPPROTO_TCP. */

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
		return ERROR_WINSOCK_SOCKET_CREATE_FAILURE;
	}
//  ...to here

	// The socket now exists...but it isn't configured with an IP address or port number yet.
	
	{   // This block sets up the IP address that the server will listen on (in case is has multiple IP addresses) and the port
		//InetPton(AF_INET, ipv4addr, &socketServer.sin_addr.s_addr); // The variable ip4addr is a character array that has an IPv4 address in it. The code defines it above to
																	// to 127.0.0.1, so it will only talk to itself. The  InetPton function converts this text string to a 32-bti number and stores
																	// it in the socket socketServer.
		//or allow the system to select one
		socketServer.sin_addr.s_addr = INADDR_ANY;					// NOTE!!! when you use INADDR_ANY, the host will print 0.0.0.0 as the IP Address, but it is listening on all IP's.
		socketServer.sin_family = AF_INET;							// Must agree with the socket Address Family type

		//CE-480 - Change the port number that the server is using by altering the following line.
		socketServer.sin_port = htons(port);						// htons() converts the host endianness to network endianness
																	// This should always be used when transmitting integers
																	// ntohs() converts the opposite way for receiving integers.
	}

	if (bind(s, (struct sockaddr *) &socketServer, sizeof(socketServer)) == SOCKET_ERROR)
	{
		printf("Could not bind socket");
		return ERROR_WINSOCK_SOCKET_BIND_FAILURE;
	}
		// converts between the IP address as a number to a printable string in ipv4addr

	{ // This block uses some common functions to print some feedback. Nothing affecting the socket happens here.
		InetNtop(AF_INET, &(socketServer.sin_addr), (PSTR)ipv4addr, INET_ADDRSTRLEN);      // The InetNtop converts the 32-bit IP address to a printable string in dotted decimal notation.
		printf("Listening on IP address %s port %d...\n", ipv4addr, ntohs(socketServer.sin_port));  // Note that teh ntohs() function is always used to convert the socket number 
																									// in network-endianness to the computer's endianness before printing it.
	}
	listen(s, 5);	// tells socket to start listening for clients, 5 is the arbitrarily chosen number of backlogged connections allowed
					// This only needs to be done once on the server's original socket.
	
	while (TRUE) {    // Normally a server runs "forever", but this demo will exit after 5 connections.
		
		if (connections > 5)
		{
			WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);
			break;
		}

		// the server will keep accepting new connectiions the "listening" socket. 
		c = sizeof(struct sockaddr_in);
		s_new = accept(s, (struct sockaddr *)&socketClient, &c); // This call will block until a client attempts to connect. At that point,
																 // a new socket is created (stored in s_new) that is a complete socket between this server
																 // and the client. The orginal socket s still existed to accept new calls. Information about the connection, 
																 // such as the client's IP and port number, is stored to socketClient. 
		thread_data *workerData = malloc(sizeof(thread_data));
		
		workerData->soc = s_new;
		workerData->connections = connections;
		workerData->test = 1337;

		//hThreadArray[connections] = _beginthread(workerThread, 0, workerData);
		hThreadArray[connections] = (HANDLE) _beginthreadex(NULL, 0, workerThread, workerData, 0, NULL);
		connections++;

		
	}   


	// then close the program
	printf("Out of \'threads\'.... closing down.\n");
	Sleep(5000);
	closesocket(s);
	WSACleanup();
	exit(0);


}