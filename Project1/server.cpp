#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h> 
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <cstring>
#include <fstream>

using namespace std;

void error(string msg)
{
    perror(msg.c_str());
    exit(1);
}


string parseRequest(char a[])
{
	string s(a);
	s = s.substr(s.find(" ")+1,s.find("\r\n"));
	s = s.substr(1,s.find(" ")-1);
	return s;
}

string getFileType(string fileName)
{
	return fileName.substr(fileName.find(".")+1,5);
}



int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, pid;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;


	if (argc < 2) 
	{
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);	//create socket
     	if (sockfd < 0) 
        error("ERROR opening socket");


	//fill in address info
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");

	listen(sockfd,5);	//5 simultaneous connection at most
     
	//accept connections
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

	if (newsockfd < 0) 
	error("ERROR on accept");

	int n;

	char buffer[1024] = {}; 

	

	//read client's message
	n = read(newsockfd,buffer,1023);
	if (n < 0) error("ERROR reading from socket");
	cout<< "----------------------------------------"<<endl<<buffer<<endl<<"----------------------------------------"<<endl;

	string fileName = parseRequest(buffer); //returns a string corresponding to requested file name.
	
	string fType = getFileType(fileName);
	
	string contentType;
	if (fType == "html"){
		contentType = "text/html";
	}
	else if (fType == "jpg" || fType == "jpeg" || fType == "png"){
		contentType = "image/" + fType;
	} 
	else if (fType == "gif"){
		contentType = "image/gif";
	} 

	string httpResponse = "HTTP/1.1 200 OK\r\nConnection: close\r\nDate: Tue, 09 Aug 2011 15:44:04 GMT\r\nSERVER: Apache/2.2.3 (CentOS)\r\nLast-Modified: Tue, 09 Aug 2011 15:11:03 GMT\r\nContent_length:500\r\nContent-Type: " + contentType += "\r\n\r\n";

	n = write(newsockfd,httpResponse.c_str(),httpResponse.length());
	if (n < 0) error("ERROR writing to socket");
	
	string data;
	int z;

	ifstream infile; 
	infile.open(fileName.c_str()); 
	if (infile.is_open()) 
	{
 		while (!infile.eof()) 
		{
			getline(infile, data);
			data.append("\n");
			n = write(newsockfd,data.c_str(),data.length());
			if (n < 0) error("ERROR writing to socket");
		}  
	}
	infile.close();


	close(newsockfd);//close connection 
	close(sockfd);

	return 0;
}