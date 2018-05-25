#include <sys/types.h> 
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define BUFSIZE 2048
#define dataSize 1012
#define headerLength 1

#define N 5120

#include "Packet.h"

#include <limits.h>


#include <time.h>
#include <sys/time.h>

#define SHOWOUTPUT 1 //if = 1 show normal outputs, if = 0 no output
#define SHOWCHECKSUMS 0 //if = 1 show checksums, if = 0 no checksums


int findIndex(short ackNum, int fakeSendBase, struct Packet * packetOut,int numberOfFullPackets)
{
	int i=0;
	for(i = fakeSendBase; i < fakeSendBase + 5 && i < numberOfFullPackets + 1; i++) // + 5 or + N
	{
		if(packetOut[i].seqNum == ackNum)
		{
			return i;
		}
	}
	
	return -1;
}

int sendBasePlusN(sendBase)
{
	if(sendBase + N - 30720 > 0)
	{
		return sendBase + N - 30720;
	}
	else
	{
		return sendBase + N;
	}
}

//returns 1 if seqNum is in base, else returns 0
int seqNumInWindow(seqNum, sendBase)
{
	

	if(sendBase < sendBasePlusN(sendBase))
	{
		if(seqNum >= sendBase && seqNum < sendBasePlusN(sendBase))
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
		if(seqNum >= sendBase || seqNum < sendBasePlusN(sendBase))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	
	
}





unsigned short checkSumCalculator(struct Packet packet, int dataLength)
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

int checkSumCalculator_datalessPacket(struct DatalessPacket packet)
{
	unsigned short retVal=0; //return variable

	
	retVal += (packet.seqNum > 0 && packet.ackNum > SHRT_MAX - packet.seqNum) ? (packet.seqNum + packet.ackNum + 1) : (packet.seqNum + packet.ackNum);
	retVal += (retVal > 0 && (unsigned short) packet.ack > SHRT_MAX - retVal) ? ((unsigned short) packet.ack + 1) : ((unsigned short) packet.ack);
	retVal += (retVal > 0 && (unsigned short) packet.syn > SHRT_MAX - retVal) ? ((unsigned short) packet.syn + 1) : ((unsigned short) packet.syn);
	retVal += (retVal > 0 && (unsigned short) packet.fin > SHRT_MAX - retVal) ? ((unsigned short) packet.fin + 1) : ((unsigned short) packet.fin);
	retVal += (retVal > 0 && (unsigned short) packet.rst > SHRT_MAX - retVal) ? ((unsigned short) packet.rst + 1) : ((unsigned short) packet.rst);
	retVal += (retVal > 0 && packet.length > SHRT_MAX - retVal) ? (packet.length + 1) : (packet.length);
	
	
	
	
	
	
	
	
	
	return ~retVal;
}

int main(int argc, char **argv)
{
	//////My Variables//////
	int currentSeqNum=0;
	
	struct Packet tempPacketIn;
	memset((char*) &tempPacketIn, 0, sizeof(tempPacketIn));
	
	char *fileName;
	
	int sendBase = 0;
	int *ackList; //list of files that has been acked
	int *sentPacketsList; //used to see if the packet is sent at least once
	struct timeval *timerList; //keeps track of time the packet was sent
	
	long int timeoutValue = 500;
	
	int fakeSendBase = 0;
	

	struct timeval timeNow;
	

	
	
	struct DatalessPacket finPacket;
	finPacket.seqNum = 0; 
	finPacket.ackNum = 0;
	finPacket.ack = '0'; 
	finPacket.syn = '0'; 
	finPacket.fin = '1'; 
	finPacket.rst = '0'; 
	finPacket.length = 12;
	finPacket.checksum = checkSumCalculator_datalessPacket(finPacket);
	
	////ACK packet
	struct DatalessPacket ackPacket;
	ackPacket.seqNum = 0; 
	ackPacket.ackNum = 0;
	ackPacket.ack = '1'; 
	ackPacket.syn = '0'; 
	ackPacket.fin = '0'; 
	ackPacket.rst = '0'; 
	ackPacket.length = 0;
	ackPacket.checksum = checkSumCalculator_datalessPacket(ackPacket);
	
	////RST packet
	struct DatalessPacket rstPacket;
	rstPacket.seqNum = 0; 
	rstPacket.ackNum = 0;
	rstPacket.ack = '0'; 
	rstPacket.syn = '0'; 
	rstPacket.fin = '0'; 
	rstPacket.rst = '1'; 
	rstPacket.length = 0;
	rstPacket.checksum = checkSumCalculator_datalessPacket(rstPacket);
	
	

	//Ack and Syn packet
	struct Packet ackSynPacket;
	ackSynPacket.seqNum = 0; 
	ackSynPacket.ackNum = 0;
	ackSynPacket.ack = '1'; 
	ackSynPacket.syn = '1'; 
	ackSynPacket.fin = '0'; 
	ackSynPacket.rst = '0'; 
	strcpy(ackSynPacket.data,"file Exists");
	ackSynPacket.length = 12 + 11*4; //file Esists = 11 characters long
	ackSynPacket.checksum = checkSumCalculator(ackSynPacket, strlen("file Exists")); 
	///////////////////////
	
	
	
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	//int msgcnt = 0;			/* count # of messages we received */
	//unsigned char buf[BUFSIZE];	/* receive buffer */
	int portno;
	
	int i; //used in for loops
	
	

	
	if (argc < 2) 
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
    }
	
	portno = atoi(argv[1]);

	
	
	/* create a UDP socket */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	
	
	/* bind the socket to any valid IP address and a specific port */
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(portno);

	
	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) 
	{
		perror("bind failed");
		return 0;
	}
	
	
	
	int receivedFileName =0;
	while(receivedFileName == 0) //receiving fileName from client
	{
		currentSeqNum = 0;
		
		printf("\nwaiting on port %d\n", portno);
		recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) 
		{
			if(SHOWOUTPUT == 1) printf("received message: \"%s\" (%d bytes)\n", tempPacketIn.data, recvlen);
			
			fileName = (char *)malloc(sizeof(char) * (tempPacketIn.length - 12));
			memcpy(fileName,tempPacketIn.data,sizeof(char) * (tempPacketIn.length - 12)); 
			
			
			
			if( access( fileName, R_OK ) == -1 ) //If file does not exist
			{
				printf("Warining: File *%s* does not exist\n",fileName);
				free(fileName);
				
				
				strcpy(ackSynPacket.data,"File Does Not Exist");
				ackSynPacket.length = 12 + 4*19;
				ackSynPacket.checksum = checkSumCalculator(ackSynPacket,strlen ("File Does Not Exist"));
				if(SHOWOUTPUT == 1) printf("Sending packet %d 5120 SYN\n", 0); //TODO: (maybe change) hard coded 5120 since this packet never receives ack in our implementation
				if(SHOWCHECKSUMS == 1) printf("Checksum for packet %d is 0x%x\n",ackSynPacket.seqNum, ackSynPacket.checksum);
				currentSeqNum = ackSynPacket.length;
				if (sendto(fd, &ackSynPacket, sizeof(ackSynPacket), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
				{
					perror("sendto");
				}
				strcpy(ackSynPacket.data,"file Exists");
				ackSynPacket.length = 12 + 4*11;
				ackSynPacket.checksum = checkSumCalculator(ackSynPacket,strlen ("file Exists"));
			}
			else
			{
				receivedFileName = 1;
				sendBase = ackSynPacket.length;
				currentSeqNum = ackSynPacket.length;
				if(SHOWOUTPUT == 1) printf("Sending packet %d 5120 SYN\n", 0); //TODO: (maybe change)hard coded 5120 since this packet never receives ack in our implementatio
				if(SHOWCHECKSUMS == 1) printf("Checksum for packet %d is 0x%x\n",ackSynPacket.seqNum, ackSynPacket.checksum);
				if (sendto(fd, &ackSynPacket, sizeof(ackSynPacket), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
				{
					perror("sendto");
				}
			}
		}
	}
	
////READING FILE TO A BUFFER
	FILE *fileptr;
	char *buffer;
	long filelen;
	
	

	fileptr = fopen(fileName, "rb");  
	fseek(fileptr, 0, SEEK_END);          
	filelen = ftell(fileptr);             
	rewind(fileptr);                  

	buffer = (char *)malloc((filelen+1)*sizeof(char)); 
	fread(buffer, filelen, 1, fileptr);
	fclose(fileptr);

	long int numberOfFullPackets = (filelen - filelen % dataSize) / dataSize ;
	long int lastPacketSize = filelen % dataSize;
	//printf("File length is: %ld\n", filelen);
	//printf("Number of full packets is: %li\n", numberOfFullPackets);
	//printf("Last packet size is: %li\n", lastPacketSize);
	
	////////////////////Creating the packets///////////////////////////
	
	struct Packet packetOut[numberOfFullPackets + 1]; //+1 for last packet
	
	
	for(i=0; i<numberOfFullPackets; i++) //full packets
	{
		packetOut[i].seqNum = currentSeqNum;
		
		if(currentSeqNum + 1024 - 30720 > 0 )
		{
			currentSeqNum = 1024 + currentSeqNum - 30720;
		}
		else
		{
			currentSeqNum = currentSeqNum + 1024;
		}
		
		
		
		packetOut[i].ackNum = 0;
		packetOut[i].ack = '0';
		packetOut[i].syn = '0';
		packetOut[i].fin = '0';
		packetOut[i].rst = '0';
		packetOut[i].length = sizeof(packetOut[i].seqNum) + sizeof(packetOut[i].ackNum) + sizeof(packetOut[i].ack) + sizeof(packetOut[i].syn) +
							  sizeof(packetOut[i].fin) + sizeof(packetOut[i].rst) + sizeof(packetOut[i].length) +
							  sizeof(packetOut[i].checksum) + sizeof(char)*dataSize;
							
		
		
		
		int j=0;
		for(j=0; j < dataSize; j++)
		{
			packetOut[i].data[j] = buffer[j + dataSize*i];
		}
		packetOut[i].checksum =checkSumCalculator(packetOut[i], dataSize);
	}
	
	
	//LAST PACKET
	
	
	
	packetOut[i].seqNum = currentSeqNum;
	packetOut[i].ackNum = 0;
	packetOut[i].ack = '0';
	packetOut[i].syn = '0';
	packetOut[i].fin = '0';
	packetOut[i].rst = '0';
	packetOut[i].length = sizeof(packetOut[i].seqNum) + sizeof(packetOut[i].ackNum) + sizeof(packetOut[i].ack) + sizeof(packetOut[i].syn) +
							  sizeof(packetOut[i].fin) + sizeof(packetOut[i].rst) + sizeof(packetOut[i].length) +
							  sizeof(packetOut[i].checksum) + sizeof(char)*lastPacketSize;
							 
	int j=0;
	for(j=0; j < lastPacketSize; j++)
	{
		packetOut[i].data[j] = buffer[j + dataSize*numberOfFullPackets];
	}
	packetOut[i].checksum =checkSumCalculator(packetOut[i], lastPacketSize);
	
	
	free(buffer);

	timerList = (struct timeval *)malloc(sizeof(struct timeval) * (numberOfFullPackets + 1));
	ackList = (int *)malloc(sizeof(int) * (numberOfFullPackets + 1));
	for(i=0; i < (numberOfFullPackets + 1); i++)
	{
		ackList[i] = 0;
	}
	
	sentPacketsList = (int *)malloc(sizeof(int) * (numberOfFullPackets + 1));
	for(i=0; i < (numberOfFullPackets + 1); i++)
	{
		sentPacketsList[i] = 0;
	}
	
	
	
	for(i=0; i < (numberOfFullPackets + 1); i++)
	{
		timerList[i].tv_sec = 0;
		timerList[i].tv_usec = 0;
	}
	
	
	
	
	//Next 4 lines make recvfrom to timeout and not wait for data
	struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 1000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	
	for(i=0; i < (numberOfFullPackets + 1); i++)
	{
		printf("%d, %d\n",i,packetOut[i].seqNum);
	}
	

	//START SENDING DATA
	for (;;) 
	{
			
		
		for(i = fakeSendBase; i < fakeSendBase + 5 && i < numberOfFullPackets + 1; i++)
		{
			if(ackList[fakeSendBase] == 1)
			{
				int index = findIndex(packetOut[fakeSendBase].seqNum, fakeSendBase, packetOut, numberOfFullPackets);
				
				if(sendBase + packetOut[index].length - 30720 > 0)
				{
					sendBase = sendBase + packetOut[index].length - 30720;
				}
				else
				{
					sendBase = sendBase + packetOut[index].length;
				}
				fakeSendBase +=1;
			}
			
			int j;
			for(j = fakeSendBase; j < fakeSendBase + N && i < numberOfFullPackets + 1; j++)
			{
				gettimeofday(&timeNow, NULL);
				
				if(sentPacketsList[j] == 1 && ackList[j] == 0)
				{
					gettimeofday(&timeNow, NULL);
					
					if((double) ((timeNow.tv_sec * 1000) + (timeNow.tv_usec / 1000)) - ((timerList[j].tv_sec * 1000) + (timerList[j].tv_usec / 1000)) >= timeoutValue )
					{
						
						if (sendto(fd, &packetOut[j], sizeof(packetOut[j]), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
						{
							perror("sendto");
						}
						else
						{
							if(SHOWOUTPUT == 1) printf("Sending packet %d Retransmission\n",packetOut[j].seqNum);
							if(SHOWCHECKSUMS == 1) printf("Checksum for packet %d is 0x%x Retransmission\n",packetOut[j].seqNum, packetOut[j].checksum);

							gettimeofday(&timeNow, NULL);
							timerList[j].tv_sec = timeNow.tv_sec;
							timerList[j].tv_usec = timeNow.tv_usec; //set time of sending in miliseconds
						}		
					}
				}
			}
			
			
		
			if(seqNumInWindow(packetOut[i].seqNum, sendBase) == 1)
			{
				
				if(ackList[i] == 0 && sentPacketsList[i] == 0)
				{
					if (sendto(fd, &packetOut[i], sizeof(packetOut[i]), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
					{
						perror("sendto");
					}
					else
					{
						
						
						if(SHOWOUTPUT == 1) printf("Sending packet %d\n",packetOut[i].seqNum);
						if(SHOWCHECKSUMS == 1) printf("Checksum for packet %d is 0x%x\n",packetOut[i].seqNum, packetOut[i].checksum);
						sentPacketsList[i] = 1; //packet i has been sent at least once
						
						gettimeofday(&timeNow, NULL);
						timerList[i].tv_sec = timeNow.tv_sec;
						timerList[i].tv_usec = timeNow.tv_usec; //set time of sending in miliseconds
					}						
				}
			}
			
			
			recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &addrlen); //receive packets from server
			if (recvlen > 0) 
			{
				if(tempPacketIn.syn != '0') //Retransmit SYN packet in case if client did not receive
				{
					if(SHOWOUTPUT == 1) printf("Sending packet %d 5120 SYN Retransmit\n", 0);
					if(SHOWCHECKSUMS == 1) printf("Checksum for packet %d is 0x%x Retransmission\n",ackSynPacket.seqNum, ackSynPacket.checksum);
					if (sendto(fd, &ackSynPacket, sizeof(ackSynPacket), 0, (struct sockaddr *)&remaddr, addrlen) < 0) //resend ack for getting the file name
					{
						perror("sendto");
					}

				}
				else if(tempPacketIn.ack != '0') // got ack
				{
					//printf("\nGot Ack %d, %d\n",tempPacketIn.ackNum, sendBase);
					if(seqNumInWindow(packetOut[i].seqNum, sendBase) == 1)
					{
						//printf("\n111Got Ack %d, %d\n",tempPacketIn.ackNum, sendBase);
						int index = findIndex(tempPacketIn.ackNum, fakeSendBase, packetOut, numberOfFullPackets);
						if(index != -1)
						{
							if(SHOWOUTPUT == 1) printf("Receiving packet %d\n",tempPacketIn.ackNum);
							ackList[index] = 1;
							
							/*
							//Moving sendBase up
							if(sendBase + packetOut[index].length - 30720 > 0)
							{
								sendBase = sendBase + packetOut[index].length - 30720;
							}
							else
							{
								//printf("Moving sendBase from %d to %d\n",sendBase, sendBase + packetOut[index].length);
								sendBase = sendBase + packetOut[index].length;
							}
							fakeSendBase +=1;
							*/
							
							//Trying to move sendBase up
							
							
						}
						
					}
					
				}
			}	
		}
		
		
		
		if(fakeSendBase >= numberOfFullPackets + 1) //we are done with file transfer, terminating connection
		{
			int readyToTerminate = 0;
			struct timeval finTransmitTimer;
			gettimeofday(&timeNow, NULL);
			finTransmitTimer.tv_sec = timeNow.tv_sec;
			finTransmitTimer.tv_usec = timeNow.tv_usec;
			
			
			int firstFinSent = 0;
			while(readyToTerminate == 0)
			{
				finPacket.seqNum = packetOut[numberOfFullPackets].seqNum + lastPacketSize;
				finPacket.checksum = checkSumCalculator_datalessPacket(finPacket);
				
				gettimeofday(&timeNow, NULL);
				if( (double) ((timeNow.tv_sec * 1000) + (timeNow.tv_usec / 1000)) - ((finTransmitTimer.tv_sec * 1000) + (finTransmitTimer.tv_usec / 1000)) > 500)
				{
					if(firstFinSent == 0)
					{
						if(SHOWOUTPUT == 1) printf("\nSending packet %ld FIN\n",packetOut[numberOfFullPackets].seqNum + lastPacketSize);
					}
					else
					{
						if(SHOWOUTPUT == 1) printf("\nSending packet %ld FIN Retransmission\n",packetOut[numberOfFullPackets].seqNum + lastPacketSize);
					}
						
					if (sendto(fd, &finPacket, sizeof(finPacket), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
					{
						perror("sendto");
					}
					firstFinSent = 1;
					gettimeofday(&timeNow, NULL);
					finTransmitTimer.tv_sec = timeNow.tv_sec;
					finTransmitTimer.tv_usec = timeNow.tv_usec; 
				}
				
				struct timeval read_timeout;
				read_timeout.tv_sec = 0;
				read_timeout.tv_usec = 1000;
				setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);
				
				recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &addrlen);
				if (recvlen >= 0) 
				{
					if(tempPacketIn.ack == '1')
					{
						if(SHOWOUTPUT == 1) printf("\nReceiving packet %d\n",tempPacketIn.ackNum);
						readyToTerminate = 1;
					}
				}
			}
			
			int closingServerConnection = 0;
			int sentLastAck = 0;
			struct timeval finalTimer;
			while(closingServerConnection == 0)
			{
				if(sentLastAck == 1)
				{
					gettimeofday(&timeNow, NULL);
					if(timeNow.tv_sec - finalTimer.tv_sec > 3)
					{
						closingServerConnection = 1;
					}
				}
				
				recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &addrlen);
				if (recvlen >= 0 && sentLastAck != 1) 
				{
					if(tempPacketIn.fin == '1')
					{
						if(SHOWOUTPUT == 1) printf("\nReceiving packet %d\n",tempPacketIn.ackNum);
						
						ackPacket.seqNum = finPacket.seqNum + 12;
						ackPacket.checksum = checkSumCalculator_datalessPacket(ackPacket);
						
						if (sendto(fd, &ackPacket, sizeof(ackPacket), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
						{
							perror("sendto");
						}
						gettimeofday(&timeNow, NULL);
						finalTimer.tv_sec = timeNow.tv_sec;
						sentLastAck = 1;
						printf("\nStarting final timer\n");
					}
				}
			}
			
			
			
			
			
			
			printf("\nFile Transfer Complete, exiting the program\n");
			
			while(1)
			{
				recvlen = recvfrom(fd, &tempPacketIn, sizeof(tempPacketIn), 0, (struct sockaddr *)&remaddr, &addrlen);
				if (recvlen >= 0) 
				{
					if(tempPacketIn.fin == '1')
					{
						if (sendto(fd, &rstPacket, sizeof(rstPacket), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
						{
							perror("sendto");
						}
					}
				}
				printf("\n1\n");
			}
			return 0;
		}
	}
}


