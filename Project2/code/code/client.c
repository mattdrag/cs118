// Client.c //

#include <time.h>
#include <sys/time.h>

#include <string.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <sys/socket.h>
#include <limits.h>
#define BUFLEN 2048
#define MSGS 1	/* number of messages to send */

#define N 5120

#include "Packet.h"


#define DEBUGMODE 0 //if == 0 no extra printf`s, if == 1 some system printf`s,
#define SHOWOUTPUT 1 //if == 0 no output about file receiving and sending, if 1 normal output

#define SHOWCHECKSUMS 0 //if = 1 show checksums, if = 0 no checksums





//Packet finds if received packet seqNum is in previous window. Returns 1 if it is and 0 if its not
int packetInPreviousWindow(unsigned short seqNum,int rcvBase)
{

	if(rcvBase - N > 0)
	{
		if(seqNum >= rcvBase - N && seqNum < rcvBase)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		int tempVar = 30720 - rcvBase - N;
		if(seqNum >= tempVar || seqNum < rcvBase)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
}


unsigned short checkSumCalculator(struct Packet packet,int dataLength)
{
	unsigned short retVal=0;
	retVal += (packet.seqNum > 0 && packet.ackNum > SHRT_MAX - packet.seqNum) ? (packet.seqNum + packet.ackNum + 1) : (packet.seqNum + packet.ackNum);
	retVal += (retVal > 0 && (unsigned short) packet.ack > SHRT_MAX - retVal) ? ((unsigned short) packet.ack + 1) : ((unsigned short) packet.ack);
	retVal += (retVal > 0 && (unsigned short) packet.syn > SHRT_MAX - retVal) ? ((unsigned short) packet.syn + 1) : ((unsigned short) packet.syn);
	retVal += (retVal > 0 && (unsigned short) packet.fin > SHRT_MAX - retVal) ? ((unsigned short) packet.fin + 1) : ((unsigned short) packet.fin);
	retVal += (retVal > 0 && (unsigned short) packet.rst > SHRT_MAX - retVal) ? ((unsigned short) packet.rst + 1) : ((unsigned short) packet.rst);
	retVal += (retVal > 0 && packet.length > SHRT_MAX - retVal) ? (packet.length + 1) : (packet.length);
	int i=0;
	for(i=0; i < dataLength; i++)
	{
		retVal += (retVal > 0 && (unsigned short) packet.data[i] > SHRT_MAX - retVal) ? ((unsigned short) packet.data[i] + 1) : ((unsigned short) packet.data[i]);
	}
	
	return ~retVal;
}

unsigned short checkSumCalculator_datalessPacket(struct DatalessPacket packet)
{
	unsigned short retVal=0;

	retVal += (packet.seqNum > 0 && packet.ackNum > SHRT_MAX - packet.seqNum) ? (packet.seqNum + packet.ackNum + 1) : (packet.seqNum + packet.ackNum);
	retVal += (retVal > 0 && (unsigned short) packet.ack > SHRT_MAX - retVal) ? ((unsigned short) packet.ack + 1) : ((unsigned short) packet.ack);
	retVal += (retVal > 0 && (unsigned short) packet.syn > SHRT_MAX - retVal) ? ((unsigned short) packet.syn + 1) : ((unsigned short) packet.syn);
	retVal += (retVal > 0 && (unsigned short) packet.fin > SHRT_MAX - retVal) ? ((unsigned short) packet.fin + 1) : ((unsigned short) packet.fin);
	retVal += (retVal > 0 && (unsigned short) packet.rst > SHRT_MAX - retVal) ? ((unsigned short) packet.rst + 1) : ((unsigned short) packet.rst);
	retVal += (retVal > 0 && packet.length > SHRT_MAX - retVal) ? (packet.length + 1) : (packet.length);

	return ~retVal;
}


int rcvBasePlusN(rcvBase)
{
	if(rcvBase + N - 30720 > 0)
	{
		return rcvBase + N - 30720;
	}
	else
	{
		return rcvBase + N;
	}
}


int seqNumInWindow(seqNum, rcvBase)
{
	if(rcvBase < rcvBasePlusN(rcvBase))
	{
		if(seqNum >= rcvBase && seqNum < rcvBasePlusN(rcvBase))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		if(seqNum >= rcvBase || seqNum < rcvBasePlusN(rcvBase))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
	
}




int findNextPacketWithCorrectSeqNum(struct Packet rcvWindPackets[N], int packetsInBuffer[N], int rcvBase)
{
	int retVal = -1;
	
	int i;
	for(i=0; i < N; i++)
	{
		if(packetsInBuffer[i] == 1)
		{
			if(rcvWindPackets[i].seqNum == rcvBase)
			{
				return i;
			}
		}
	}
	
	return retVal;
}

//returns -1 if buffer is full, otherwise returns number corresponding to buffer position that is not occupied
int findPlaceInBuffer(int packetsInBuffer[N])
{
	int retVal = -1;
	
	int i;
	for(i=0; i < N; i++)
	{
		if(packetsInBuffer[i] == 0)
		{
			return i;
		}
	}
	
	return retVal;
}


//returns 1 if buffer is empty, or 0 if it is not
int isBufferEmpty(int packetsInBuffer[N])
{
	int retVal = 1;
	int i;
	for(i=0; i < N; i++)
	{
		if(packetsInBuffer[i] == 1)
		{
			retVal = 0;
		}
	}
	
	return retVal;
}



//returns 0 if the packet is a new packet and 1 if we already got it
int checkIfWeGotThisPacket(struct Packet tempPacketIn,struct Packet rcvWindPackets[N], int packetsInBuffer[N])
{
	int retVal = 0;
	int i;
	for(i=0; i < N; i++)
	{
		if(packetsInBuffer[i] == 1)
		{
			if(tempPacketIn.seqNum == rcvWindPackets[i].seqNum)
			{
				retVal = 1;
			}
		}
	}
	
	return retVal;
}


struct DatalessPacket copyPacketToDataless(struct Packet dataPacket)
{
	struct DatalessPacket returnPacket;
	
	returnPacket.seqNum = dataPacket.seqNum;
	returnPacket.ackNum = dataPacket.ackNum; 
	returnPacket.ack = dataPacket.ack;
	returnPacket.syn = dataPacket.syn;
	returnPacket.fin = dataPacket.fin;
	returnPacket.rst = dataPacket.rst;
	returnPacket.checksum = dataPacket.checksum;
	returnPacket.length = 12;
	
	return returnPacket;
}


int main(int argc, char *argv[])
{
	
	int check =0; //this variable checks if file transfer is complete
	int firstSegment =0; //bool indicating if we received the very first packet
	int numberOfPacketsReceived = 0; //keeps count or received packets
	unsigned short tempChecksum;
	char *fileName;
	
	struct Packet tempPacketIn;
	memset((char*) &tempPacketIn, 0, sizeof(tempPacketIn));
	
	struct Packet rcvWindPackets[N];
	int packetsInBuffer[N];
	int z;
	for(z = 0; z < N; z++)
	{
		packetsInBuffer[z] = 0;
	}
	
	int rcvBase = 0;
	
	struct timeval synAckTimer;
	struct timeval timeNow;
	
	
	
	
	////ACK packet
	struct DatalessPacket ackPacket;
	ackPacket.seqNum = 0; 
	ackPacket.ackNum = 0;
	ackPacket.ack = '1'; 
	ackPacket.syn = '0'; 
	ackPacket.fin = '0'; 
	ackPacket.rst = '0'; 
	ackPacket.length = 12;
	ackPacket.checksum = checkSumCalculator_datalessPacket(ackPacket);
	
	
	////FIN packet
	struct DatalessPacket finPacket;
	finPacket.seqNum = 0; 
	finPacket.ackNum = 0;
	finPacket.ack = '0'; 
	finPacket.syn = '0'; 
	finPacket.fin = '1'; 
	finPacket.rst = '0'; 
	finPacket.length = 12;
	finPacket.checksum = ackPacket.checksum = checkSumCalculator_datalessPacket(finPacket);
	
	
	
	
	//////////////////////////////////////
	
	
	struct sockaddr_in myaddr, remaddr;
	int fd, i, slen=sizeof(remaddr);
	int recvlen;		/* # bytes in acknowledgement message */
	char *server;
	int serverSocket;
	
	
	//Getting the arguments from command line
	if (argc < 4) 
	{
       fprintf(stderr,"usage %s <ip_addr> <port> <filename>\n", argv[0]);
       exit(0);
    }
	server = argv[1];
	serverSocket = atoi(argv[2]);
	fileName = (char *)malloc(sizeof(char) * strlen(argv[3]));
	memcpy(fileName,argv[3],sizeof(char) * strlen(argv[3]));
	
	
	

	/* create a socket */
	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	
	
	/* bind it to all local addresses and pick any port number */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(serverSocket);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		printf("inet_aton() failed\n");
		exit(1);
	}

	/* now let's send the messages */
	
	struct Packet packetOutFileRequest;
	packetOutFileRequest.seqNum = 0; 
	packetOutFileRequest.ackNum = 0;//tempPacketIn.seqNum; ???CHECK???
	packetOutFileRequest.ack = '0'; 
	packetOutFileRequest.syn = '1'; 
	packetOutFileRequest.fin = '0'; 
	packetOutFileRequest.rst= '0'; 
	packetOutFileRequest.length = 12 + strlen(fileName);
	memcpy(packetOutFileRequest.data, fileName,8*strlen(fileName));
	packetOutFileRequest.checksum = checkSumCalculator(packetOutFileRequest, strlen(fileName));
	
	printf("File Name is %s\n", packetOutFileRequest.data);
	
	
	//Establishing the connection
	gettimeofday(&timeNow, NULL);
	synAckTimer.tv_sec = timeNow.tv_sec;
	synAckTimer.tv_usec = timeNow.tv_usec;
	
	
	int fileNameSent = 0;
	int firstTimeTransmission = 0;
	while(fileNameSent == 0)
	{
		gettimeofday(&timeNow, NULL);
		if( (double) ((timeNow.tv_sec * 1000) + (timeNow.tv_usec / 1000)) - ((synAckTimer.tv_sec * 1000) + (synAckTimer.tv_usec / 1000)) > 500 || firstTimeTransmission == 0)
		{
			
			if(firstTimeTransmission == 0)
			{
				if(SHOWOUTPUT == 1) printf("Sending packet SYN\n");
			}
			else
			{
				if(SHOWOUTPUT == 1) printf("Sending packet SYN Retransmission\n");
			}
			
			if (sendto(fd, &packetOutFileRequest, sizeof(packetOutFileRequest), 0, (struct sockaddr *)&remaddr, slen)==-1) 
			{
				perror("sendto");
				exit(1);
			}
			firstTimeTransmission = 1;
			gettimeofday(&timeNow, NULL);
			synAckTimer.tv_sec = timeNow.tv_sec;
			synAckTimer.tv_usec = timeNow.tv_usec;
		}
		
		
		struct timeval read_timeout;
		read_timeout.tv_sec = 0;
		read_timeout.tv_usec = 1000;
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
		recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &slen);
		if (recvlen >= 0) 
		{
			tempChecksum = checkSumCalculator(tempPacketIn, ((tempPacketIn.length - 12)/4));

			if(tempChecksum == tempPacketIn.checksum)
			{
				if (SHOWCHECKSUMS == 1) printf("Got correct checksum when sending the file name\n");
				if(tempPacketIn.syn == '1' && tempPacketIn.ack == '1' && !strcmp(tempPacketIn.data, "file Exists"))
				{
					if(SHOWOUTPUT == 1) printf("\nReceiving packet %d \n", tempPacketIn.seqNum);
					fileNameSent = 1;
					rcvBase = rcvBase + tempPacketIn.length ;
					
				}
				else if (!strcmp(tempPacketIn.data, "File Does Not Exist"))
				{
					printf("ERROR: File does not exist on server!, aborting the program\n");
					return 0;
				}
			}
			else
			{
				if (SHOWCHECKSUMS == 1) printf("Checksum fields did not match. Given checksum 0x%x, our calculated checksum 0x%x\n", tempPacketIn.checksum, tempChecksum);
				
			}
			
		}
		
	}
	
	
	
	
	
	int firstTimeSendingFin = 0;
	//RECEIVE DATA FROM SERVER
	struct Packet *packet = malloc(sizeof(struct Packet));
	while(check == 0)
	{

		recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &slen);
		if (recvlen >= 0) 
		{

			if(tempPacketIn.length == 12)
			{
				tempChecksum = checkSumCalculator_datalessPacket(copyPacketToDataless(tempPacketIn));
			}
			else
			{
				tempChecksum = checkSumCalculator(tempPacketIn, tempPacketIn.length - 12);
			}
			if(tempChecksum == tempPacketIn.checksum)
			{
				if (SHOWCHECKSUMS == 1) printf("1 Got Correct checksum\n");
				if(tempPacketIn.fin == '1') //Got Fin message, trying to end the connection
				{
					if(SHOWOUTPUT == 1) printf("Receiving packet %d\n", tempPacketIn.seqNum);
					
					ackPacket.ackNum = tempPacketIn.seqNum;
					ackPacket.checksum = checkSumCalculator_datalessPacket(ackPacket);
					sendto(fd, &ackPacket, sizeof(ackPacket), 0, (struct sockaddr *)&remaddr, slen);
					if(SHOWOUTPUT == 1) printf("Sending packet %d\n", tempPacketIn.seqNum);
					
					struct timeval firstFinAckTime;
					gettimeofday(&timeNow, NULL);
					firstFinAckTime.tv_sec = timeNow.tv_sec;
					firstFinAckTime.tv_usec = timeNow.tv_usec;


					int firstFinAckReceived = 0;
					while(firstFinAckReceived == 0)
					{
						gettimeofday(&timeNow, NULL);
						if( (double) ((timeNow.tv_sec * 1000) + (timeNow.tv_usec / 1000)) - ((firstFinAckTime.tv_sec * 1000) + (firstFinAckTime.tv_usec / 1000)) > 500)
						{
							if(firstTimeSendingFin == 0)
							{
								if(SHOWOUTPUT == 1) printf("Sending packet %d FIN\n", tempPacketIn.seqNum);
							}
							else
							{
								if(SHOWOUTPUT == 1) printf("Sending packet %d FIN Retransmission\n", tempPacketIn.seqNum);
							}
							
							firstTimeSendingFin = 1;
							
							finPacket.ackNum = tempPacketIn.seqNum;
							finPacket.checksum = checkSumCalculator_datalessPacket(finPacket);
							if (sendto(fd, &finPacket, sizeof(finPacket), 0, (struct sockaddr *)&remaddr, slen) == -1) 
							{
								perror("sendto");
								exit(1);
							}
							gettimeofday(&timeNow, NULL);
							firstFinAckTime.tv_sec = timeNow.tv_sec;
							firstFinAckTime.tv_usec = timeNow.tv_usec;
						}
						
						
						struct timeval read_timeout;
						read_timeout.tv_sec = 0;
						read_timeout.tv_usec = 1000;
						setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
						recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &slen);
						if (recvlen >= 0) 
						{
							if(tempPacketIn.length == 12)
							{
								tempChecksum = checkSumCalculator_datalessPacket(copyPacketToDataless(tempPacketIn));
							}
							else
							{
								tempChecksum = checkSumCalculator(tempPacketIn, (tempPacketIn.length - 12)/4);
							}
							
							
							
							if(tempChecksum == tempPacketIn.checksum)
							{
								if (SHOWCHECKSUMS == 1) printf("2 Got correct checksum\n");
								
								if(tempPacketIn.ack == '1')
								{
									if(SHOWOUTPUT == 1) printf("Receiving packet %d\n", tempPacketIn.seqNum);
									firstFinAckReceived = 1;
								}
								else if(tempPacketIn.rst == '1')
								{
									if(SHOWOUTPUT == 1) printf("Receiving packet %d\n", tempPacketIn.seqNum);
									firstFinAckReceived = 1;
								}
								else if(tempPacketIn.fin == '1')//meaning server did not receive our ack for his fin
								{
									if(SHOWOUTPUT == 1) printf("Sending packet %d Retransmission\n", tempPacketIn.seqNum);
									sendto(fd, &ackPacket, sizeof(ackPacket), 0, (struct sockaddr *)&remaddr, slen);
								}
								
								
								
							}
							else
							{
								if (SHOWCHECKSUMS == 1) printf("1 Checksum fields did not match, expected x0%x got 0x%x\n",tempPacketIn.checksum, tempChecksum );
							}
						}
						
					}
								
					check = 1;
						
				}
				
				
				else
				{
					
					int positionInBuffer = findPlaceInBuffer(packetsInBuffer);
					if(positionInBuffer == -1) //buffer is full and we are dumping everything in the buffer
					{
						printf("Receive buffer is full, clearing the buffer\n");
						for(z=0; z < N; z++)
						{
							packetsInBuffer[z] = 0;
						}
					}
					else
					{
						if(seqNumInWindow(tempPacketIn.seqNum, rcvBase) == 1)
						{
							if(SHOWOUTPUT == 1) printf("Receiving packet %d\n",tempPacketIn.seqNum);
							
							////SEND ACK BACK TO SERVER
							ackPacket.ackNum = tempPacketIn.seqNum;
							ackPacket.checksum = checkSumCalculator_datalessPacket(ackPacket);

							if(SHOWOUTPUT == 1) printf("Sending packet %d\n",ackPacket.ackNum);
							sendto(fd, &ackPacket, sizeof(ackPacket), 0, (struct sockaddr *)&remaddr, slen);
							
							if(checkIfWeGotThisPacket(tempPacketIn, rcvWindPackets, packetsInBuffer) == 0) //This is a new packet 
							{
								if(tempPacketIn.seqNum == rcvBase) //This is the expected next packet
								{
									
									if(firstSegment == 0) //allocate storage in permanent storage if not the first packet(We already alocated space for first above)
									{
										firstSegment = 1;
									}
									else
									{
										packet = (struct Packet *)realloc(packet, (numberOfPacketsReceived+1) * sizeof(struct Packet));
									}
									packet[numberOfPacketsReceived] = tempPacketIn;
									
									numberOfPacketsReceived +=1;
									if(rcvBase + tempPacketIn.length - 30720 > 0)
									{
										rcvBase = rcvBase + tempPacketIn.length - 30720;
									}
									else
									{
										rcvBase = rcvBase + tempPacketIn.length ;

									}
									
									if(DEBUGMODE == 1) printf("--Next packet with seqNum %d received. Moving rcvBase to %d\n",tempPacketIn.seqNum, rcvBase);
									
									int nextPacketToInsertIntoMemory = findNextPacketWithCorrectSeqNum(rcvWindPackets, packetsInBuffer, rcvBase);
									while(nextPacketToInsertIntoMemory != -1) //trying to move packets from rcvBuffer to permanent memory
									{
										if(firstSegment == 0) //allocate storage in permanent storage if not the first packet(We already alocated space for first above)
										{
											firstSegment = 1;
										}
										else
										{
											packet = (struct Packet *)realloc(packet, (numberOfPacketsReceived+1) * sizeof(struct Packet));
										}
										
										packet[numberOfPacketsReceived] = rcvWindPackets[nextPacketToInsertIntoMemory];
										
										numberOfPacketsReceived +=1;
										
										if(rcvBase + rcvWindPackets[nextPacketToInsertIntoMemory].length - 30720 > 0)
										{
											rcvBase = rcvBase + rcvWindPackets[nextPacketToInsertIntoMemory].length - 30720;
										}
										else
										{
											rcvBase = rcvBase + rcvWindPackets[nextPacketToInsertIntoMemory].length ;
										}
										
										packetsInBuffer[nextPacketToInsertIntoMemory] = 0;
										if(DEBUGMODE == 1) printf("\n**Next packet with seqNum %d from buffer window received. Moving rcvBase to %d\n",packet[numberOfPacketsReceived].seqNum, rcvBase);
										
										nextPacketToInsertIntoMemory = findNextPacketWithCorrectSeqNum(rcvWindPackets, packetsInBuffer, rcvBase);
									}
									
									
									
								}
								else
								{
									rcvWindPackets[positionInBuffer] = tempPacketIn;
									packetsInBuffer[positionInBuffer] = 1;
									if(DEBUGMODE == 1) printf("\n~~packet with seqNum %d received. Putting it into buffer\n",tempPacketIn.seqNum);
								}
								
								
							}
							
							if(isBufferEmpty(packetsInBuffer) == 1)
							{
								for(z=0; z < N; z++);
							}
							
						}
						else if(packetInPreviousWindow(tempPacketIn.seqNum, rcvBase) == 1) //packet is out of window but we still need to send ack
						{
							////SEND ACK BACK TO SERVER
							ackPacket.ackNum = tempPacketIn.seqNum;
							ackPacket.checksum = checkSumCalculator_datalessPacket(ackPacket);
							
							if(SHOWOUTPUT == 1) printf("Sending packet %d Retransmission\n",ackPacket.ackNum);
							sendto(fd, &ackPacket, sizeof(ackPacket), 0, (struct sockaddr *)&remaddr, slen);
						}
						
					}	
				}
			
			
			}
			else
			{
				if (SHOWCHECKSUMS == 1) printf("2 Checksum fields did not match, expected 0x%x got 0x%x\n",tempPacketIn.checksum, tempChecksum);
			}
		}
	}

	
	FILE *write_ptr;
	write_ptr = fopen("testOut.png","wb");
	
	
	
	
	i=0;
	int headerLength = sizeof(packet[i].seqNum) + sizeof(packet[i].ackNum) +
					 sizeof(packet[i].ack) + sizeof(packet[i].syn) + sizeof(packet[i].fin) +
					 sizeof(packet[i].rst) + sizeof(packet[i].checksum) + sizeof(packet[i].length);
	
	
	for(i=0; i < numberOfPacketsReceived; i++)
	{
			fwrite(packet[i].data,packet[i].length - headerLength,1,write_ptr);
	}
	fclose(write_ptr);
	
	printf("\nFile transfer is complete\n");
	close(fd);
	return 0;
}

