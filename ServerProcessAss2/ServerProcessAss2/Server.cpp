#include "stdafx.h"
#include "Server.h"


// get the current host name
string getHostname()
{
	char hostname[1024];
	DWORD size = 1024;
	GetComputerName(hostname,  &size);
	return hostname;
}


// server constructor
Server::Server(void)
{
	srand(time(NULL) + 25);
	freeLogFile("log.txt");
	print( "Starting on host " << getHostname());
	////////////SELF
	m_listeningPort = 5000;

	//////////////ROUTER
	m_routerListeningPort = 7000;
	m_routerHostName = "localhost";
	m_requestNumber = 0;

	m_handshackeIsDone = false;
	bindToListeningPort();
	initializeRemoteSocAdd();
	m_sendBuffer = new char[maxDataSize + sizeof(MessageHeader)];
}



// destructor - free-up resources
Server::~Server(void)
{
	delete [] m_sendBuffer;
}


// delete the log file if exists
void Server::freeLogFile(string log){
	ofstream logFile;
	if(fileExists(log)){
		remove(log.c_str());
	}
}


bool Server::bindToListeningPort(){
	bool returnValue = true;

	int error = WSAStartup (0x0202, &m_wdata);   // Starts socket library
	if (error != 0)
	{
		std::cout << "wsastartup failed with error: " << error << std::endl;
		return false; //For some reason we couldn't start Winsock
	}


	initializeLocalSocAdd();
	m_listeningSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Creates a socket

	if (m_listeningSocket == INVALID_SOCKET)
	{
		print("failed to create a socket, error: " << WSAGetLastError() << endl);
		return false; //Don't continue if socket is not created
	}

	// bides socket to a given port/address
	if (bind(m_listeningSocket, (LPSOCKADDR)&m_localSocAddr, sizeof(m_localSocAddr)) == SOCKET_ERROR)
	{
		//error accures if we try to bind to the same socket more than once
		print("error binding socket: " << WSAGetLastError() << endl);
		return false;
	}

	return returnValue;
}


// low level send function 
int msgnum = 0;
bool Server::sendUDPPacket(MessageHeader* message)
{
	msgnum++;
	message->msgnumber = msgnum;

	int messageSize = message->msgSize + sizeof(MessageHeader);
	int result = sendto(m_listeningSocket, (char*)message, messageSize, 0, (struct sockaddr *)&m_remoteSocAddr, sizeof(m_remoteSocAddr));

	// check if error occurred
	if(result == SOCKET_ERROR)
	{
		stringstream serr;
		serr << "Error sending "<< WSAGetLastError()<<endl;
		throw serr.str();
	}

	sentPackets++;
	sentBytes += MsgSize(message);

	return true;
}


// initialize socket data and get prepared for udp connection
bool Server::initializeRemoteSocAdd(){
	bool returnValue = true;

	m_remoteSocAddr.sin_family = AF_INET;
	m_remoteSocAddr.sin_port  = htons(m_routerListeningPort);

	hostent * pHostEntry = gethostbyname(m_routerHostName.c_str()); // returns server IP based on server host name 
	if(pHostEntry == NULL)//if thre is no such hoste 
	{
		std::cout << "Error host not found : "<< WSAGetLastError() << std::endl;
		return false; 
	}
	m_remoteSocAddr.sin_addr = *(struct in_addr *) pHostEntry->h_addr;


	return returnValue;


}

// initialize socket data and get prepared for udp connection
bool Server::initializeLocalSocAdd()
{

	m_localSocAddr.sin_family = AF_INET;      // Address family
	m_localSocAddr.sin_port = htons (m_listeningPort);   // Assign port to this socket
	m_localSocAddr.sin_addr.s_addr = htonl (INADDR_ANY);  

	return true;
}

// low level udp receive function
MessageHeader* Server::receiveUDPPacket(){
	MessageHeader* result = NULL;
	int size = sizeof(m_remoteSocAddr);


	int returnValue = recvfrom(m_listeningSocket, m_recBuff, MAXPACKETSIZE, 0, (SOCKADDR *) & m_remoteSocAddr, &size);
	if(returnValue == SOCKET_ERROR){
		stringstream serr;
		serr << "Receive error "<< WSAGetLastError()<<endl;
		throw serr.str();
	}
	else if (returnValue != 0) // if received data
	{
		result = (MessageHeader *)m_recBuff;

		recvPackets++;
		recvBytes += MsgSize(result);
	}

	return result;
}


// wait for incomming data during a timeout
MessageHeader* Server::runSelectMethod(){
	MessageHeader* result = NULL;
	fd_set readfds;
	struct timeval *tp=new timeval();

	int RetVal, fromlen, wait_count;
	DWORD count1, count2;

	count1=0; 
	count2=0;
	wait_count=0;
	tp->tv_sec=0;//TIMEOUT;
	tp->tv_usec=TIMEOUT*1000;
	try{
		FD_ZERO(&readfds);
		FD_SET(m_listeningSocket,&readfds);

		fromlen=sizeof(m_remoteSocAddr);
		//check for incoming packets.
		if((RetVal=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR){	
			throw "Timer Out from client side";
		}
		else if(RetVal>0){ // if there is actual data, then retrieve it
			if(FD_ISSET(m_listeningSocket, &readfds))	//incoming packet from peer host 1
			{
				result = receiveUDPPacket();
				if(result == NULL)		
					throw " Get buffer error!";

			}
		}

	}catch(string ss){
		cout << "Error occurred: " << ss << endl;
	}

	return result;

}


// handshaking function
// 2 packets of 3 way handshaking is sent/received 
void Server::handShaking()
{
	m_localGeneratedNumber = generateRandomNum();
	m_localSequence = getLastSignificantBit(m_localGeneratedNumber);
	cout<<endl;
	cout<<"server generated : "<<m_localGeneratedNumber<<endl;
	cout<<"server sequence number is : "<<m_localSequence<<endl;

	MessageHeader* message = (MessageHeader*)m_sendBuffer;
	MessageHeader* messageReceived = (MessageHeader*)m_recBuff;


	message->msgType = MessageType::HANDSHAKE;
	message->sequenceNumber =m_localGeneratedNumber;  //sends its generated sequence number
	message->isLastChunck = 1;
	message->ACK = messageReceived->sequenceNumber;// piggybackes acknowledgment for last received message from client
	message->msgSize = 0;

	sendUDPPacket(message);
	m_handshackeIsDone = true;
}


// extract the sequence number out of randomly generated number
int Server::getLastSignificantBit(int number)
{
	return number % 2;
}


// generate a random number
int Server::generateRandomNum(){
	return rand()%253 + 2;
}


// message loop function for incomming network messages
void Server::processIncommingMessages(){

	do{
		try
		{ 
			//wait for incomming message
			MessageHeader* header = receiveUDPPacket();
			if(header != NULL)
			{
				switch(header->msgType)
				{
				case MessageType::GET:
					{
						//if incomming message contains piggybacked acknowledgment for the last 3 wey hanshack message , then accept it, otherwise ignor
						if(m_handshackeIsDone && header->ACK == m_localGeneratedNumber && header->sequenceNumber == m_remoteSequence )
						{
							string fileName = header->data;
							// check if the file exists and only then send the file to client
							if(fileExists(FILEPATH + fileName)){
								sendRequestedFile(fileName);
							}
							else{
								//if file does not exist send an error message
								string wrongFile = header->data;
								string messageBody = wrongFile + " : File does not exist";
								sendErrorMessage(messageBody);
							}
							updateSequenceNumber(m_remoteSequence); //update client sequence number
							//update request number when task is done
							m_handshackeIsDone = false;
						}
						break;
					}
				case MessageType::HANDSHAKE:
					{
						m_remotGeneratedNumber = header->sequenceNumber;// number generated by client is assigned to a sequence number
						m_remoteSequence = getLastSignificantBit(m_remotGeneratedNumber);
						cout<<"client generated : "<<m_remotGeneratedNumber<<endl;
						cout<<"client sequence number "<<m_remoteSequence<<endl;
						cout<<header->sequenceNumber<<endl;
						cout<<endl;
						cout<<"handshake"<<endl;
						handShaking();
						break;
					}

				case MessageType::PUT:
					{
						if(m_handshackeIsDone && header->ACK == m_localGeneratedNumber && header->sequenceNumber == m_remoteSequence )
						{
							// send an ACK message 
							sendACK("Server is ready to transfer file");
							receiveFile(header->data);
							m_handshackeIsDone = false;
						}
						break;
					}
				case MessageType::LIST:
					{
						if(m_handshackeIsDone && header->ACK == m_localGeneratedNumber && header->sequenceNumber == m_remoteSequence )
						{
							listFilesInDirectory();	//lists files available on the server
							m_handshackeIsDone = false;
						}
						break; 

					}
				default:
					{
						if(m_lastSendPacket != NULL)
							resendLastPacket();
						break;
					}
				} //switch
			}//if

		} //try
		catch(const char * err)
		{
			cout << err << endl;
			string ss;
			m_handshackeIsDone = false;
		}
		cout<<"client sequence " <<m_remoteSequence<<endl;
		cout<<"server sequence "<<m_localSequence<<endl;

	}// do while
	while(true);

}

// download the file from client
void Server:: receiveFile(std::string fileName)
{
	resetStats(); // reset statistics

	int dataBytesReceived = 0;

	// calculate the file path, where to store the icomming data
	std::string filePath = FILEPATH + fileName;
	bool isLastChunck = false;//last chank or not
	size_t SizeOfFileChunck;
	std::ofstream file;
	string str;
	int currentCunck = 0;
	//open a file for binary write
	file.open(filePath, std::ios::out | std::ios::binary);
	try{
		do{
			cout << "enter waitForData" <<endl;
			MessageHeader* header = waitForData(); //accept a chanck of the file and reads the header
			cout << "exit waitForData" <<endl;
			SizeOfFileChunck = header->msgSize;//shows how many bites should be written
			isLastChunck = header->isLastChunck;//is it the last chanck or not
			//if file is oppen without errors
			if(file.is_open()){
				//write received chanck
				file.write(header->data, SizeOfFileChunck);
				str = "Chank" + intToStr(currentCunck) + "is transmited";
				// send acknowledgement
				sendACK(str);
				// update statistics
				currentCunck++;
				dataBytesReceived += header->msgSize;
			}
			if(header->isLastChunck)
			{
				// when the last chunk is received
				// wait in a loop and resend the last acknowledgement
				// this could happen if the client did not received the last acknowledgement
				int repeatForLastPacket = 0;
				while(repeatForLastPacket <= RETRY)
				{
					if(runSelectMethod() != NULL)
						resendLastPacket();
					repeatForLastPacket++;
				}
				isLastChunck = true;
			}

		}while(!isLastChunck); //while it is not the last chunck
	}catch(const char* ee){
		if(file.is_open()){
			file.close();
			remove(filePath.c_str());
			throw ee;
		}
	}
	// close the file
	file.close();
	
	print("download of file "<<fileName<<" is completed"<<endl);

	// print statistics
	print("number of DATA packets received: " << currentCunck);
	print("number of DATA bytes received: " << dataBytesReceived);
	print("number of packets sent: " << sentPackets);
	print("number of bytes sent: " << sentBytes);
	print("total number of packets received: " << recvPackets);
	print("total number of bytes received: " << recvBytes);
}


// check if the current file exists
bool Server::fileExists(string fileName)
{
	string filePath = fileName;
	bool returnValue = true;
	//This will get the file attributes bitlist of the file
	DWORD fileAtt = GetFileAttributesA(filePath.c_str());

	//If an error occurred it will equal to INVALID_FILE_ATTRIBUTES
	if(fileAtt == INVALID_FILE_ATTRIBUTES)
		returnValue = false;
	return returnValue;

}


// upload the requested file to client side
void Server::sendRequestedFile(std::string fileName)
{
	resetStats();
	std::string filePath = FILEPATH	+ fileName;
	int nuberOfBits = maxDataSize;// maxmum size for a chank

	int dataBytesSent = 0;
	size_t size = 0;
	
	ifstream file (filePath, ios::in|ios::binary|ios::ate); // opens a file for binarry reading
	if (file.is_open())
	{
		cout<<endl;
		
		//if file is oppen
		file.seekg(0,ios::end);	//moves coursor at the end of a file
		size = file.tellg();//gets the size of the file
		file.seekg(0,ios::beg);//moves cursor t the beginning of the file, to start reading it
		size_t currentSize = size;//
		int current = 0;//the number of current sended packet
		int isLast = 0; //this variable shows whether the sending packet is the last one or not, if it is equal 0 => is not the last

		MessageHeader * header = (MessageHeader*)m_sendBuffer; //maps message header into a buffer


		while(currentSize != 0) 
		{
			//while there is something to read in the file
			if(currentSize > nuberOfBits)
			{
				file.read(header->data, nuberOfBits ); //read file by the size of nuberOfBits
				currentSize = currentSize - nuberOfBits;//reduce by the bits already read
			}
			else
			{
				//if what is left is smaller than we wxpect
				//indicate that that is the last chank
				nuberOfBits = currentSize;
				file.read(header->data, nuberOfBits );
				currentSize = 0; 
				isLast = 1;//indicates last chanck
			}

			int sendSize = nuberOfBits + sizeof(MessageHeader); //parameter that shows the size of the message we have to send

			//builds message header	
			header->msgType = MessageType::DATA;
			header->msgSize = nuberOfBits;
			header->isLastChunck = isLast;
			header->sequenceNumber = m_localSequence;
			header->ACK = 3;
			if(current == 0) 
				header->ACK  = m_remoteSequence; //piggybacks sequence just received request message

			//keep last sent packet
			updateLastSendPachet(header);

			print("sent data packet " << header->sequenceNumber);
			if(!sendUDPPacket(header)){
				file.close();
				cout<<endl;
				//retuns an error when connection is died
				throw "Socket error on the client side";
			}
			else{

				MessageHeader* confirmation =  waitForAcknowledgement();
				//updateSequenceNumber(m_localSequence);
				if(confirmation != NULL){
					print("received ACK for packet " << header->sequenceNumber);
				}
				else{
					print("file processing is failed"<<endl);
					break;
				}
			}

			//update statistics
			current++;
			dataBytesSent += header->msgSize;
		}

		file.close();

		print("upload of file " << fileName << " is completed "<<endl);

		// print statistics
		print("number of DATA packets sent: " << current);
		print("number of DATA bytes sent: " << dataBytesSent);
		print("number of packets sent: " << sentPackets);
		print("number of bytes sent: " << sentBytes);
		print("total number of packets received: " << recvPackets);
		print("total number of bytes received: " << recvBytes);
	
	}
	else{
		//sendErrorMessage(fileName + " : File can not oppen");
	}
	resetStats();
}

// updates the sequence number
void Server::updateSequenceNumber(int& sequence){
	if(sequence == 1) sequence = 0;
	else sequence = 1;
}


// send the acknowledgment
void Server::sendACK(string messageBody){
	std::string message  = messageBody;

	int sendSize = message.length() + sizeof(MessageHeader) + 1; //+1 for "/0" to indicate end of string
	char* buffer = new char[sendSize];

	MessageHeader * header = (MessageHeader*)buffer;

	header->msgType = MessageType::ACK;
	header->msgSize = message.length() + 1;
	header->isLastChunck = true;// this is the last reply
	header->sequenceNumber = m_remoteSequence;
	header->ACK = 3;//nothing is piggybacked
	memcpy(header->data, message.c_str() , message.length() + 1);
	m_lastSendPacket = header;//update last sent packet
	updateSequenceNumber(m_remoteSequence);//update sender/client sequence number
	if(!sendUDPPacket(header)){
		cout<<endl;
		//retuns an error when connection is died
		throw "Socket error on the client side";
	}
}


// resend last sent packet
void Server::resendLastPacket(){

	sendUDPPacket(m_lastSendPacket);
}


// waits for acknowledgement packet
MessageHeader*  Server::waitForAcknowledgement(){

	MessageHeader* ACKmessage;
	int count = 0;
	while(count <= RETRY)
	{
		ACKmessage = runSelectMethod();
		if(ACKmessage != NULL && ACKmessage->msgType == MessageType::ACK &&  
			ACKmessage->sequenceNumber == m_localSequence)
		{
			// acknowledgement is received
			updateSequenceNumber(m_localSequence);			
			break;
		}
		else{
			// resend last packet
			cout<< "resend packet: "<< count << endl;
			resendLastPacket();
			count++;
		}
	}

	return ACKmessage;
}


// wait for incomming data packet
MessageHeader* Server::waitForData(){

	MessageHeader* ACKmessage;
	int count = 0;
	while(count < RETRY)
	{
		ACKmessage = runSelectMethod();
		if(ACKmessage != NULL && ACKmessage->msgType == MessageType::DATA &&  ACKmessage->sequenceNumber == m_remoteSequence){
			// data is received can return from function
			break;
		}
		else
		{
			// resend last packet
			resendLastPacket();
			count++;
			// continue to wait
		}
	}

	if(count >= RETRY)
		throw "Connection Error :";
	return ACKmessage;

}

void Server::updateLastSendPachet(MessageHeader* lastSentPacket){
	m_lastSendPacket = lastSentPacket;
}

void Server::writeIntoLogFile(string log){
	ofstream logFile;
	logFile.open("log.txt", ios::out | ios::app);
	if(logFile.is_open())
	{
		logFile<<log<<endl;
		logFile.close();
	}

}

void Server::outputMessage(string message){
	writeIntoLogFile(message);
	cout<<message<<endl;
}

string Server::intToStr(int intValue){

	stringstream strValue;
	strValue<<intValue;
	return strValue.str();
}

// wait for handshake
MessageHeader*  Server::waitForHandShack(){
	string str;
	MessageHeader* ACKmessage;
	int count = 0;
	while(count <= RETRY)
	{
		print("waits for handshack "<<m_lastSendPacket->sequenceNumber);
		ACKmessage = runSelectMethod();
		if(ACKmessage != NULL && ACKmessage->msgType == MessageType::HANDSHAKE &&  ACKmessage->sequenceNumber == m_localGeneratedNumber){
			updateSequenceNumber(m_localSequence);
			updateSequenceNumber(m_remoteSequence);
			break;
		}
		else
		{
			resendLastPacket();
			count++;
		}
		if(count == RETRY)
			throw "Connection Error : Client does not receive ack";
	}

	return ACKmessage;
}


// notify about error to the client
void Server::sendErrorMessage(string messageBody){

	char* buffer = new char[maxDataSize];
	MessageHeader* message = (MessageHeader*)buffer;
	MessageHeader* messageReceived = (MessageHeader*)m_recBuff;
	message->msgType = MessageType::ERR;
	message->sequenceNumber = messageReceived->sequenceNumber; //acknowledges sequenvce number it received from a client
	strcpy(message->data, messageBody.c_str());//attaches its own sequence number 
	message->msgSize = messageBody.length() + 1;
	message->ACK = 3;
	sendUDPPacket(message);
	updateLastSendPachet(message); 

}


// reset local state
void Server::resetState(){
	m_localSequence = getLastSignificantBit(m_localGeneratedNumber);
	m_remoteSequence = getLastSignificantBit(m_remotGeneratedNumber);
}





//this method is done based on the internet 
//lists all the files that are in server's file directory
void Server::listFilesInDirectory(){

	HANDLE hFile; // Handle to file
	string filePath = FILEPATH;
	WIN32_FIND_DATA FileInformation; // File information
	filePath = filePath + "*.*";
	//find the first file in the given path
	hFile = FindFirstFile(filePath.c_str(), &FileInformation);
	
	stringstream str;

	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{//if file exists add into a file list
			if((FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				str << "$" << FileInformation.cFileName << "\t" << ((FileInformation.nFileSizeHigh * (MAXDWORD+1)) + FileInformation.nFileSizeLow) / 1024 << " kb (" <<
				((FileInformation.nFileSizeHigh * (MAXDWORD+1)) + FileInformation.nFileSizeLow) << " bytes)";
				//cout<< FileInformation.cFileName<< std::endl;
			}
			//while there is a file find the next file
		}while(::FindNextFile(hFile, &FileInformation) == TRUE);

		string files = str.str();
		if(files.length() == 0)
			sendErrorMessage("No files availabled for download");
		else
			sendFileList(files);
	}
}


// send files list to client, available for download
void Server::sendFileList(string list){
	size_t currentSize = list.length();//
	int current = 0;//the number of current sended packet
	int isLast = 0; //this variable shows whether the sending packet is the last one or not, if it is equal 0 => is not the last
	MessageHeader * header = (MessageHeader*)m_sendBuffer; //maps message header into a buffer
	bool isFirstChanck = true;
	int nuberOfBits = maxDataSize - 1;
	int start = 0;
	while(currentSize != 0) 
	{
		//while there is something to read in the file
		if(currentSize > nuberOfBits)
		{
			strcpy(header->data, list.substr(0, nuberOfBits).c_str());
			list = list.substr(nuberOfBits);
			//file.read(header->data, nuberOfBits ); //read file by the size of nuberOfBits
			currentSize = currentSize - nuberOfBits;//reduce by the bits already read
		}
		else
		{
			//if what is left is smaller than we wxpect
			//indicate that that is the last chank
			nuberOfBits = currentSize;
			strcpy(header->data, list.c_str());

			//file.read(header->data, nuberOfBits );
			currentSize = 0; 
			isLast = 1;//indicates last chanck
		}

		int sendSize = nuberOfBits + sizeof(MessageHeader) +1; //parameter that shows the size of the message we have to send

		//builds message header	
		header->msgType = MessageType::DATA;
		header->msgSize = nuberOfBits +1;
		header->isLastChunck = isLast;
		header->sequenceNumber = m_localSequence;
		if(isFirstChanck){
			header->ACK  = m_remoteSequence;
			updateSequenceNumber(m_remoteSequence);
			isFirstChanck = false;
		}
		else{
			header->ACK = 3;
		}


		//piggybacks sequence just received request message

		//keep last sent packet
		updateLastSendPachet(header);

		sendUDPPacket(header);
		MessageHeader* confirmation =  waitForAcknowledgement();

		if(confirmation != NULL){
//			cout<<"chank : " <<	current<<" is send" <<endl;
		}
		else{
			cout<< "file processing is failed"<<endl;
			break;
		}

		current++;
	}

}
