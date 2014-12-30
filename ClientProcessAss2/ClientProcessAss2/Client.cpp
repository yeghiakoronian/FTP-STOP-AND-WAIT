#include "stdafx.h"
#include "Client.h"

// get current host name
string getHostname()
{
	char hostname[1024];
	DWORD size = 1024;
	GetComputerName(hostname,  &size);
	return hostname;
}




// constructor
Client::Client(void)
{
	srand(time(NULL) + 15);
	freeLogFile("log.txt");
	print( "Starting on host " << getHostname());
	const char * a = getHostname().c_str();

	////////////SELF
	m_listeningPort = 5001;

	//////////////ROUTER
	m_routerListeningPort = 7001;

	m_routerHostName = "localhost";
	
	m_acknowledgementState = false;
	bindToListeningPort();
	initializeLocalSocAdd();
	initializeRemoteSocAdd();

}





Client::~Client(void)
{
}


// prepare socket data
bool Client::bindToListeningPort(){
	bool returnValue = true;

	int error = WSAStartup (0x0202, &m_wdata);   // Starts socket library
	if (error != 0)
	{
		print( "WSAStartup failed with error: " << error);
		return false; //For some reason we couldn't start Winsock
	}


	initializeLocalSocAdd();


	m_listeningSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Creates a socket

	if (m_listeningSocket == INVALID_SOCKET)
	{
		print( "failed to listen on port: " << WSAGetLastError());
		return false; //Don't continue if socket is not created
	}

	// bides socket to a given port/address
	if (bind(m_listeningSocket, (LPSOCKADDR)&m_localSocAddr, sizeof(m_localSocAddr)) == SOCKET_ERROR)
	{
		//error accures if we try to bind to the same socket more than once
		print("error binding socket: " << WSAGetLastError());
		return false;
	}


	return returnValue;
}


// low level send udp message function
int msgnum = 0;
MessageHeader* Client::sendUDPPacket(MessageHeader* message){
	string str;
	msgnum++;
	message->msgnumber = msgnum;

#ifdef PRINT_SEND_RCV
	print("SEND: " << ToStr(message));
#endif

	int messageSize = message->msgSize + sizeof(MessageHeader);
	int result = sendto(m_listeningSocket, (char*)message, messageSize, 0, (struct sockaddr *)&m_remoteSocAddr, sizeof(m_remoteSocAddr));

	if(result == SOCKET_ERROR)
		print("Error sending: " << WSAGetLastError()); 

	// update statistics
	sentPackets++;
	sentBytes += MsgSize(message);

	return message;
}


// initialize remote socket data
bool Client::initializeRemoteSocAdd(){
	bool returnValue = true;

	m_remoteSocAddr.sin_family = AF_INET; 
	m_remoteSocAddr.sin_port  = htons(m_routerListeningPort);

	hostent * pHostEntry = gethostbyname(m_routerHostName.c_str()); // returns server IP based on server host name 
	if(pHostEntry == NULL)//if thre is no such hoste 
	{
		print("Error host not found : "<< WSAGetLastError());
		return false; 
	}
	m_remoteSocAddr.sin_addr = *(struct in_addr *) pHostEntry->h_addr;


	return returnValue;


}


// low level receive udp packet fuction
MessageHeader* Client::receiveUDPPacket(){

	MessageHeader* result = NULL;
	int size = sizeof(m_remoteSocAddr);

	int returnValue = recvfrom(m_listeningSocket, m_recBuff, MAXPACKETSIZE, 0, (SOCKADDR *) & m_remoteSocAddr, &size);
	if(returnValue == SOCKET_ERROR){
		print("receive error: " << WSAGetLastError());		
	}
	else if(returnValue > 0) // check if really received data
	{
		result = (MessageHeader*) m_recBuff;
#ifdef PRINT_SEND_RCV
		cout << "RCV: " <<ToStr(result) << endl;
#endif
		
		// update statistics
		recvPackets++;
		recvBytes += MsgSize(result);
	}

	return result;
}


// initialize socket data
bool Client::initializeLocalSocAdd(){

	m_localSocAddr.sin_family = AF_INET;      // Address family
	m_localSocAddr.sin_port = htons (m_listeningPort);   // Assign port to this socket
	m_localSocAddr.sin_addr.s_addr = htonl (INADDR_ANY);


	return true;
}


// perform receive on timeout
MessageHeader* Client::runSelectMethod(){
	MessageHeader* message = NULL;
	fd_set readfds;
	struct timeval *tp=new timeval;

	int RetVal, fromlen, recvlen, wait_count;
	DWORD CurrentTime, count1, count2;

	count1=0; 

	wait_count=0;
	tp->tv_sec=0;//TIMEOUT;
	tp->tv_usec=TIMEOUT*1000;
	try{
		FD_ZERO(&readfds);
		FD_SET(m_listeningSocket,&readfds);
		//	FD_SET(Sock2,&readfds);
		fromlen=sizeof(m_remoteSocAddr);
		//check for incoming packets.
		if((RetVal=select(1,&readfds,NULL,NULL,tp))==SOCKET_ERROR){	
			throw "Timer Out from client side";
		}
		else if(RetVal>0){
			if(FD_ISSET(m_listeningSocket, &readfds))	//incoming packet from peer host 1
			{
				message = receiveUDPPacket();
				//message = (MessageHeader*)m_recBuff;
				if(message == NULL)		
					throw " Get buffer error!";

			}
		}

	}catch(string ss){
		print("Error occurred: " << ss);
	}
	return message;

}


// do the handshaking 
// in a loop and retry if failed
// note there is no resend for handshake messages
void Client::handShaking()
{
	while(runSelectMethod() != NULL)
	{
		cout << ".";
	}
	cout<<endl;

	bool doHandShaking = true;
	char* buffer = new char[maxDataSize];
	MessageHeader* message = (MessageHeader*)buffer;
	MessageHeader* serverReturnMessage = NULL;

	while(doHandShaking)
	{
		print("-------====start handshake====-------");
		m_localGeneratedNumber= generateRandomNum();
		m_localSequenceNumber = getLastSignificantBit(m_localGeneratedNumber);

		
		print("client generated : "<<m_localGeneratedNumber);
		print("client sequence number is : "<<m_localSequenceNumber);


		message->msgType = MessageType::HANDSHAKE;
		message->sequenceNumber = m_localGeneratedNumber;
		message->msgSize = 0;
		message->isLastChunck = 1;
		message->ACK = 3;
		sendUDPPacket(message);
		serverReturnMessage = runSelectMethod();

		if(serverReturnMessage != NULL && serverReturnMessage->msgType == MessageType::HANDSHAKE &&  serverReturnMessage->ACK == m_localGeneratedNumber)
		{
			m_remotGeneratedNumber = serverReturnMessage->sequenceNumber;
			m_remoteSequenceNumber = getLastSignificantBit(m_remotGeneratedNumber);
			doHandShaking = false;
			// handshaking is done
		}
	}
}


// generate random number
int Client::generateRandomNum(){
	return rand() % 253 + 2;
	
}


// extract sequence number
int Client::getLastSignificantBit(int number)
{
	return number % 2;
}


// spilt the given string into vector of strings delimiting by delimitter
void Client::split(string inputValue, vector<string>* result) {
	std::stringstream stream(inputValue);
	std::string item;
	//reads from a stream, writes into item, $ indicates end of line(separeter)
	while (std::getline(stream, item, '$')) {
		//ads item into a given vector
		result->push_back(item);
	}

}



// get the files in local directory 
void Client::listFilesInDirectory(){

	HANDLE hFile; // Handle to file
	std::string filePath = FILEPATH;
	WIN32_FIND_DATA FileInformation; // File information
	filePath = filePath + "*.*";
	// find all files
	hFile = FindFirstFile(filePath.c_str(), &FileInformation);

	int fileCount = 0;
	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			// print files found
			if((FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				print(FileInformation.cFileName<< "\t" << ((FileInformation.nFileSizeHigh * (MAXDWORD+1)) + FileInformation.nFileSizeLow) / 1024 << " kb (" <<
				((FileInformation.nFileSizeHigh * (MAXDWORD+1)) + FileInformation.nFileSizeLow) << " bytes)");
				fileCount++;
			}
		}while(::FindNextFile(hFile, &FileInformation) == TRUE);
	}
	if(fileCount == 0)
		print("No files available"<<endl);
}


// process messages function
int Client::processClientMessages(){

	// print the list of available commands
	print("available commands: ");
	print("get : dowloade file");
	print("put : uploads file");
	print("list : returns available file list fron the server");
	print("quit : stop the process"<<endl<<endl);

	char* buffer = new char[maxDataSize];
	MessageHeader* message = (MessageHeader*)buffer;

	while(true){
		try{

			print("waiting for a command..."<< std::endl);

			string command;
			command =  readCommand();
			if(command == "get") // received 'get' command
			{
				print("Process Get method."<< endl);
				
				print("Available files on the server ");
				// print the available server files names list
				getFilesList();


				// read the file name to download
				print(endl << "input file name");

				string fileName = readCommand();

				print(fileName << " is requeted from server "<< endl);

				bool retryConnectionRequest = true;
				while(retryConnectionRequest){
					handShaking(); //proess handshack

					// set the message header to send get command to server
					message->msgType = MessageType::GET;
					message->msgSize = fileName.length() + 1; // doesn't matter just in case
					message->isLastChunck = true;
					message->sequenceNumber = m_localSequenceNumber;
					message->ACK = m_remotGeneratedNumber; // iggybackes the last acknowledgment for 3 way handshack
					// set the file name to download
					strcpy(message->data, fileName.c_str());

					print("3rd handshack is piggybacked with GET request message "<<"sdf"<<1223);
					sendUDPPacket(message);

					updateLastSendPacket(message);
					if(getFile(fileName))
						retryConnectionRequest = false;

				}//while
				updateSequenceNumber(m_localSequenceNumber); 
			}
			else if(command == "put") // received 'set' command
			{
				// print out the list of available files to upload

				print("List of available files ");
				listFilesInDirectory();
				print("");

				// get the file name to upload
				print("input file name");
				string fileName = readCommand();
				print("local file " << fileName << " upload is started");

				// check if the file exists
				if(fileExists(FILEPATH + fileName))
				{
					bool retryConnectionRequest = true;
					while(retryConnectionRequest){
						handShaking();
						// send the set message to server
						message->msgType = MessageType::PUT; // command is set
						message->msgSize = fileName.length() + 1; // exact size to read
						message->isLastChunck = true; // doesn't matter just in case
						message->sequenceNumber = m_localSequenceNumber;
						message->ACK = m_remotGeneratedNumber; // if this value is equal to 3 => there is no acknowledgement piggybacked
						strcpy(message->data, fileName.c_str()); // prepare the message to send
						//send the message
						if(sendUDPPacket(message))
						{
							updateLastSendPacket(message);

							// upload the file
							if(setFile(fileName))
								retryConnectionRequest = false;
						}
					}
				}
				else 
				{
					
					print("file does not exist");
				}
			}

			else if(command == "list")
			{
				getFilesList();
			}
			else if(command == "quit")
			{
				return 0;
			}
			else
			{
				cout<<endl;
				cout<<"Wrong command!"<<endl;
			}
		}
		catch(const char* error)
		{
			print(error);
		}
		catch(OperationError* error)
		{
			print("Error occurred - " << error->Message);
		}


		print("client sequence " << m_localSequenceNumber);
		print("server sequence " << m_remoteSequenceNumber);
	}


	string str;
	cout<<"quit process";
	cin>>str;//just avoid to closing a process

	return 0;

}

void Client::getFilesList()
{
	char* buffer = new char[maxDataSize];
	MessageHeader* message = (MessageHeader*)buffer;

	bool retryConnectionRequest = true;
	while(retryConnectionRequest)
	{
		handShaking();

		// request the files list from server

		// set the header
		message->msgType = MessageType::LIST;
		message->msgSize = 0; // doesn't matter just in case
		message->isLastChunck = true;
		message->sequenceNumber = m_localSequenceNumber;
		message->ACK = m_remotGeneratedNumber;
		updateLastSendPacket(message);
		//send the command to server
		print("connection established"<<endl);
		
		sendUDPPacket(message);
				
		m_acknowledgementState = false;
		bool t = true;
		MessageHeader* fileList;
		string list = getFileList();
		parsServerFileList(list);

		if(list.length() != 0)
		{
			updateSequenceNumber(m_localSequenceNumber);
			retryConnectionRequest = false;
		}
		else
		{
			print("--==connection failed==--");
		}
	}

	delete [] buffer;
}


// read input from console
string Client::readCommand()
{
	string data;
	getline(cin,data);
	std::transform(data.begin(), data.end(), data.begin(), ::tolower);

	return data;
}


void Client::updateSequenceNumber(int& sequence){
	if(sequence == 1) sequence = 0;
	else sequence = 1;
}


// send acknowledgement
void Client::sendACK(string messageBody){
	std::string message  = messageBody;

	int sendSize = message.length() + sizeof(MessageHeader) + 1; //+1 for "/0" to indicate end of string
	char* buffer = new char[sendSize];

	MessageHeader * header = (MessageHeader*)buffer;

	header->msgType = MessageType::ACK;
	header->msgSize = message.length() + 1;
	header->isLastChunck = true;// this is the last reply
	header->sequenceNumber = m_remoteSequenceNumber;
	header->ACK = 3; // nothing is piggybacked
	memcpy(header->data, message.c_str() , message.length() + 1);

	updateLastSendPacket(header);
	updateSequenceNumber(m_remoteSequenceNumber);
	if(!sendUDPPacket(header)){
		//retuns an error when connection is died
		throw "Socket error on the client side";
	}

}

void Client::resendLastPacket()
{
	sendUDPPacket(m_lastSendPacket);
}

MessageHeader*  Client::waitForAcknowledgement(){
	string str;
	MessageHeader* ACKmessage;
	int count = 0;
	while(count <= RETRY)
	{
		ACKmessage = runSelectMethod();
		if(ACKmessage != NULL && ACKmessage->msgType == MessageType::ACK &&  ACKmessage->sequenceNumber == m_localSequenceNumber){
			updateSequenceNumber(m_localSequenceNumber);
			// acknowledgement received
			break;
		}
		else{
			// resend last packet
			// retry
			print(str);
			resendLastPacket();
			count++;
		}
		if(count >= RETRY)
			throw "Connection Error : Client did not receive ack";
	}

	return ACKmessage;
}


bool Client::getFile(string fileName)
{
	resetStats();
	print("file download started: " << fileName << endl);

	m_acknowledgementState = false;
	std::string filePath = FILEPATH + fileName; // compute the local file path to write
	bool isLastChunck = false;
	string str;
	// receive request result message
	MessageHeader * header = NULL;
	std::ofstream file;
	// open the file in binary mode to write the incomming data
	file.open(filePath, std::ios::out | std::ios::binary);
	// check if the file is successfully opened
	if(!file.is_open())
	{
		// terminate the command in case of error
		throw new OperationError("file can not be written: "+ filePath);
	}

	///// stats
	// enumerate the chunks
	int currentChunk = 0;
	int dataBytesReceived = 0;
	///// stats
	// receive the file data from server in a loop and write it into the file
	try{
		do{ 
			header = waitForData(currentChunk);
			if(header == NULL)
				return false;
			if( header->msgType == MessageType::DATA)
			{
				file.write(header->data, header->msgSize);
				
				currentChunk++;	
				dataBytesReceived += header->msgSize;

				print("received data packet " << header->sequenceNumber);
				sendACK("chank is received"); //send acknowledjment first
				print("send acknowledgment for packet " << header->sequenceNumber); 

				if(header->isLastChunck)
				{
					int repeatForLastPacket = 0;
					while(repeatForLastPacket <= RETRY)
					{
						if(runSelectMethod() != NULL)
							resendLastPacket();
						repeatForLastPacket++;
					}
				} 

			}

		}
		while(header!= NULL && !header->isLastChunck); // finish the loop if the last chunk is received
	}
	catch(const char* ee)
	{
		if(file.is_open()){
			file.close();
		}
		remove(filePath.c_str());
		print("Exception: " << ee << endl);
		return false;
	}
	print("download of file "<<fileName<<" is completed"<<endl);

	print("number of DATA packets received: " << currentChunk);
	print("number of DATA bytes received: " << dataBytesReceived);
	print("number of packets sent: " << sentPackets);
	print("number of bytes sent: " << sentBytes);
	print("total number of packets received: " << recvPackets);
	print("total number of bytes received: " << recvBytes);
	// close the file
	file.close();
	return true;	
	resetStats();
}


MessageHeader* Client::waitForData( int currentChanck){
	string str;
	MessageHeader* ACKmessage;


	int count = 0;
	while(count < RETRY)
	{
		//print("Waiting for data sequence : " << m_lastSendPacket->sequenceNumber);
		bool resend = false;
		ACKmessage = runSelectMethod();
		if(ACKmessage != NULL)
		{
			if(ACKmessage->msgType  == MessageType::ACK && m_acknowledgementState == false)
			{
				m_acknowledgementState = true;
				break;
			}
			else if(ACKmessage->msgType == MessageType::ERR) //&& ACKmessage->sequenceNumber == m_remoteSequenceNumber)
			{
				throw new OperationError(ACKmessage->data);
			}
			else if(ACKmessage->msgType == MessageType::DATA)
			{
				if((!m_acknowledgementState && ACKmessage->sequenceNumber == m_remoteSequenceNumber && ACKmessage->ACK == m_localSequenceNumber)
					||(m_acknowledgementState && ACKmessage->sequenceNumber == m_remoteSequenceNumber))
				{
					if(!m_acknowledgementState)
						m_acknowledgementState = true;

					break;
				}
				else
					resend = true;
			}
		}
		

		if(currentChanck !=0){
			if (resend || !m_acknowledgementState)
			{
				//print("Resend : sequence :" << m_lastSendPacket->sequenceNumber); 
				resendLastPacket();
			}
		}
		count++;
		if(count >= RETRY)
			return NULL;
	}

	return ACKmessage;
}

void Client::updateLastSendPacket(MessageHeader* lastSentPacket){
	m_lastSendPacket = lastSentPacket;
}



void Client::outputMessage(string message){

	writeIntoLogFile(message);
	cout<<message<<endl;
}

string Client::intToStr(int intValue){

	stringstream strValue;
	strValue<<intValue;
	return strValue.str();
}



void Client::writeIntoLogFile(string log){
	ofstream logFile ;
	logFile.open("log.txt", ios::out | ios::app);
	if(logFile.is_open())
	{
		logFile<<log<<endl;
		logFile.close();
	}

}

void Client::freeLogFile(string log){
	ofstream logFile;
	if(fileExists(log)){
		remove(log.c_str());
	}
}


bool Client::fileExists(string fileName)
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

bool Client::setFile(string fileName)
{
	resetStats();
	bool result = false;
	// compute local file name
	std::string filePath = FILEPATH	+ fileName;
	int nuberOfBytes = maxDataSize;

	size_t size = 0;
	char* byteStream = 0;
	int dataBytesSent = 0;

	// open the file
	print("preparing file to send to server : " <<filePath <<endl);
	ifstream file (filePath, ios::in|ios::binary|ios::ate);
	// if the file is opened
	if (file.is_open())
	{
		//wait for a confirmation from a server side

		MessageHeader * header = waitForAcknowledgement();
		
		print(endl << "file transfer has started "<< endl);

		// comput the file size
		file.seekg(0,ios::end);
		size = file.tellg();
		file.seekg(0,ios::beg);
		size_t currentSize = size;
		int current = 0;
		int isLast = 0; //this variable shows whether the sending packet is the last one or not, if it is equal 0 => is not the last
		while(currentSize != 0) 
		{
			// get the current chunk to send to server
			// if there is more file data than the buffer can hold
			if(currentSize > nuberOfBytes){
				// then get file data equal to buffer size
				byteStream = new char[nuberOfBytes];
				file.read( byteStream, nuberOfBytes );
				currentSize = currentSize - nuberOfBytes;
			}
			else{
				// get the remaining data
				nuberOfBytes = currentSize;
				byteStream = new char[nuberOfBytes];
				file.read( byteStream, nuberOfBytes );
				currentSize = 0;
				// set the last chunk variable
				isLast = 1;
			}

			// get the total send size
			int sendSize = nuberOfBytes + sizeof(MessageHeader);
			// allocate buffer to hold the data and send using socket
			char* buffer = new char[sendSize];
			// map the buffer to message header
			MessageHeader * header = (MessageHeader*)buffer;

			// set the message header
			header->msgType = MessageType::DATA;
			header->msgSize = nuberOfBytes;
			header->isLastChunck = isLast;
			header->sequenceNumber = m_localSequenceNumber;
			header->ACK = 3; // if this value is equal to 3 => there is no acknowledgement piggybacked
			// copy the data to send to send buffer
			memcpy(header->data, byteStream, nuberOfBytes);

			m_lastSendPacket = header; ////update last sent packet
			print("sent data packet " << header->sequenceNumber);
			if(!sendUDPPacket(header)){
				file.close();
				
				//retuns an error when connection is died
				print(endl << "Socket error on the server side:"<<GetLastError()<<endl);
				break;
			}
			else{
				waitForAcknowledgement();
				print("received ACK for packet " << header->sequenceNumber);
			}

			dataBytesSent += header->msgSize;
			current++;

			// cleanup resources
			delete[] byteStream;
			delete[] buffer;

		}
		if(currentSize == 0)
		{
			result = true;
			print("transfer of file "<<fileName<<"  is completed"<<endl);
		}

		print("number of DATA packets sent: " << current);
		print("number of DATA bytes sent: " << dataBytesSent);
		print("number of packets sent: " << sentPackets);
		print("number of bytes sent: " << sentBytes);
		print("total number of packets received: " << recvPackets);
		print("total number of bytes received: " << recvBytes);

		resetStats();

	}
	else
	{
		throw new OperationError("file doesn't exist");
	}



	return result;
}


void Client::resetState(){
	m_localSequenceNumber = getLastSignificantBit(m_localGeneratedNumber);
	m_remoteSequenceNumber = getLastSignificantBit(m_remotGeneratedNumber);
}



void Client::parsServerFileList(string list)
{
	// wait for a message containing file name list

	// parse and split the data into vector
	vector<string> fileItems;
	split(list, &fileItems);

	vector<string>::iterator it;
	// print the file names list
	for(it = fileItems.begin(); it != fileItems.end(); ++it){
		print(*it);

	}
}

string Client::getFileList(){
	int currentChunk = 0;
	MessageHeader* header = NULL;
	string list;
	bool t = true;
	// receive the file data from server in a loop and write it into the file
	try{
		do{ 
			header =  waitForData(currentChunk);
			if( header != NULL && header->msgType == MessageType::DATA)
			{
				//parsServerFileList(fileList);
				//updateSequenceNumber(m_remoteSequenceNumber);
				t = false;
				list = list + header->data;
				//cout<<currentChunk << " chank is downloaded " <<endl;
				currentChunk++;	
				//m_lastSendPacket = header;
				sendACK("chank is received"); //send acknowledjment first

				if(header->isLastChunck)
				{
					int repeatForLastPacket = 0;
					while(repeatForLastPacket <= RETRY)
					{
						if(runSelectMethod() != NULL)
							resendLastPacket();
						repeatForLastPacket++;
					}
				}
			}
		}
		while(header!= NULL && !header->isLastChunck); // finish the loop if the last chunk is received
	}catch(const char* ee){
		throw ee;
	}

	return list;
}


