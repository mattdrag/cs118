//Here is the structure of the packet

/*
	16 bit rows
	***************************
	*   Sequence Number       *
	*      Ack Number         *
	*   Ack    |    Syn       *
	*   Fin    |    rst       *
	*       Chekcsum          *
	*        Length           *
	*		  DATA            *
	***************************
*/




struct Packet
{
	unsigned short seqNum; //sequence number: 2 byte 
	unsigned short ackNum; //acknowledgement number: 2 byte
	char ack; //ack flag (Have to be 0 or 1): 1 byte
	char syn; //syn flag (Have to be 0 or 1): 1 byte
	char fin; //fin flag (Have to be 0 or 1): 1 byte
	char rst; //This will always be empty for now: It is used so the checksum calculation will be easier: 1 byte
	unsigned short checksum; //checksum field: 2 byte
	unsigned short length;
	
	//So in total, the header is 10 bytes long, so we can store 1024-12=1012 bytes of data
	
	char data[1012];
};

struct DatalessPacket
{ 
	unsigned short seqNum;
	unsigned short ackNum; 
	char ack;
	char syn;
	char fin;
	char rst;
	unsigned short checksum;
	unsigned short length;
};