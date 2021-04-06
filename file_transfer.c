// File transfer by TCP / UDP
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<sys/stat.h>
#include<fcntl.h>

struct Ubuf{          //
	char buf[1024];   // Buffer used by UDP
	uint8_t bye;      //
};

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	time_t now;
	int sock, sock0, portno;
	socklen_t clilen;
	unsigned char buf[256];
	struct sockaddr_in server_addr, client_addr;
	struct hostent *server;
	struct Ubuf ubuf;
	ubuf.bye = 0;


	size_t s = sizeof(struct Ubuf);
	if(!strcmp("tcp",argv[1]))
	{
		if(!strcmp("send",argv[2]))
		{
			int request = 0,n = 0, f_len = 0,ret = 0;
			size_t f_size = 0;
			clock_t start,finish;
			double duration;

			sock = socket(AF_INET,SOCK_STREAM,0); // SOCKET_STREAM for TCP
			if(sock < 0) 
				error("Error opening socket");

			bzero((char *) &server_addr,sizeof(server_addr));                       //
			portno = atoi(argv[4]);                                                 //
			server_addr.sin_family = AF_INET;                                       //
			server_addr.sin_addr.s_addr = INADDR_ANY;                               //    TCP sender arguments setting
			server_addr.sin_port = htons(portno);                                   //
			if(bind(sock,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0) //
				error("Error on binding");                                          //
			listen(sock,5); // passive listen client

			clilen = sizeof(client_addr);
			sock0 = accept(sock, (struct sockaddr *) &client_addr, &clilen); // sock0 is client socket
			if(sock0 < 0)
				error("Error on accept");

			n = read(sock0, &request,sizeof(int)); // receive request from client 
			if(n < 0) error("error reading from socket");

			//start to handle the request
			if(request)
			{
				printf("transfer start...\n");
				f_len = strlen(argv[5]) + 1; // file name length

				n = write(sock0, &f_len, sizeof(int));                    //
				if(n < 0) error("error writing file length to client\n"); // send file length and name to client
				n = write(sock0, argv[5], f_len * sizeof(char));          //
				if(n < 0) error("error wtiting file to client\n");        //
			

				int f = open(argv[5],O_RDONLY);        // file open
				if(f < 0) error("file open error\n");  // 
				struct stat st;
				fstat(f,&st);
				f_size = st.st_size; // file file size

				if( write(sock0,&f_size,sizeof(size_t)) < 0 )     // send file size to client
					error("write file length to client error\n");

				size_t buftime = 0, logper25 = f_size * 0.25 / sizeof(buf); // variables for calculate packet log 
				int log = 0;                                                //
				int count = 0;                                              //
				
				start = clock(); // start calculate transfer time
				time(&now);
				printf("0%%   %s",ctime(&now));
				while((n = read(f,buf,sizeof(buf))) > 0)          // read original file
				{                                                 //
					write(sock0,buf,n);                           // send to clinet
					buftime ++;                                   //
					if(buftime == logper25 && log != 100)         // calculate log per 25%
					{                                             //
						buftime = 0;                              //
						time(&now);                               //
						printf("%d%%   %s",log+=25,ctime(&now));   //
					}
					
				}
				finish = clock(); // calculate transfer time end
				duration = (double)(finish - start) / CLOCKS_PER_SEC; // total trans time /ms
				printf("file size : %zu MB\n",f_size / 1000000);
				printf("Total trans time: %fms\n",duration * 1000);
				printf("File transfer successful\n");
			}
			else
				printf("No clients request file\n");
	
			close(sock0);//TCP close
			close(sock);//close socket listen
		}
		else if(!strcmp("recv",argv[2]))
		{
			int request = 1, ret = 0, n = 0,f_len = 0; 
			char f_name[256];
			size_t f_size = 0, r_size = 0;
			sock = socket(AF_INET,SOCK_STREAM,0); // SOCKET_STREAM for TCP
			if(sock < 0) 
				error("Error opening socket");
		
			server = gethostbyname(argv[3]);
			portno = atoi(argv[4]);
			if(server == NULL){
				fprintf(stderr,"ERROR, no such host\n");
				exit(0);
			}
					

			bzero((char *) &server_addr,sizeof(server_addr));                                        //
			server_addr.sin_family = AF_INET;                                                        //
			bcopy((char *)server->h_addr, (char *) &server_addr.sin_addr.s_addr,server->h_length);   //  TCP sender arguments setting
			server_addr.sin_port = htons(portno);                                                    //
			if(connect(sock,(struct sockaddr*) &server_addr,sizeof(server_addr)) < 0)                //
				error("error connecting\n");

			memset(buf,0,sizeof(buf));
			n = write(sock, &request, sizeof(int));// request the file
			n = read(sock, &f_len, sizeof(int)); // send file length to server
			n = read(sock, f_name, f_len * sizeof(char)); // send file name to server 
			char ch[20] = "recv_by_tcp_";                  //
			char newname[1024];                     // 
			memset(newname,'\0',sizeof(newname));   //  create new file name for receive file
			strcat(newname,ch);                     //
			strcat(newname,f_name);                 //

			int fr = open(newname,O_WRONLY | O_CREAT|O_TRUNC,0644); // open a new file for receive file
			n = read(sock, &f_size,sizeof(size_t)); // receive file size from server
			printf("receiving...\n");
			while((n = read(sock, buf, sizeof(buf))) > 0) // receive packet from server
			{
				ret = write(fr,buf,n); // write packet to fr
				if(ret < 1)
				{
					perror("write");
					break;
				}
			}
			close(fr); // close receive file
			printf("received\n");
			close(sock); // close socket 
		}
	}
	/* UDP */
	else if(!strcmp("udp",argv[1]))
	{
		if(!strcmp("send",argv[2]))
		{
			int request = 0,f_len=0,n = 0,ret = 0;
			size_t f_size = 0;
			clock_t start,finish;
			double duration;

			sock = socket(AF_INET,SOCK_DGRAM,0);// SOCK_DGRAM for UDP
			if(sock < 0) error("Error opening socket");

			bzero((char *) &server_addr,sizeof(server_addr));                          //
			portno = atoi(argv[4]);                                                    //
			server_addr.sin_family = AF_INET;                                          //  UDP sender arguments setting
			server_addr.sin_addr.s_addr = INADDR_ANY;                                  //
			server_addr.sin_port = htons(portno);                                      //
			if(bind(sock,(struct sockaddr *) &server_addr,sizeof(server_addr)) < 0)    //
				error("Error on binding");                                             //
			clilen = sizeof(client_addr);                                              //
			recvfrom(sock, &request,sizeof(int),0,(struct sockaddr *)&client_addr,&clilen); // receive request from client
            // start handle request from client
			if(request)
			{
				printf("start trans...\n");
				f_len = strlen(argv[5]) + 1; // f_len -> file name length
				sendto(sock, &f_len , sizeof(int), 0, (struct sockaddr*)&client_addr, clilen); // send file length to client
				sendto(sock, argv[5] , f_len * sizeof(char), 0, (struct sockaddr*)&client_addr, clilen); // send file name to client

				int f = open(argv[5],O_RDONLY);        //  open original file
				if(f < 0) error("file open error\n");  //
				struct stat st;
				fstat(f,&st);
				f_size = st.st_size; // original  file size

				sendto(sock ,&f_size,sizeof(size_t), 0, (struct sockaddr*)&client_addr, clilen); // send file size to client
				
				size_t buftime = 0, logper25 = f_size * 0.25 / sizeof(ubuf.buf);  // 
				int log = 0;                                                 //  variables for calculate log
				int count = 0;                                               //
				
				start = clock(); // start calculate transfer time
				time(&now);
				printf("0%%  %s",ctime(&now));
				while((n = read(f,ubuf.buf,sizeof(ubuf.buf))) > 0)                               // read original file
				{                                                                                // 
					sendto(sock, &ubuf, sizeof(ubuf), 0, (struct sockaddr*)&client_addr,clilen); // send packet to client
					buftime ++;
					if(buftime == logper25 && log != 100)         // calculate log per 25%
					{
						buftime = 0;                          
						time(&now);
						printf("%d%%  %s",log+=25,ctime(&now));
					}	
				}
				finish = clock(); // calculate transfer end
				duration = (double)(finish - start) / CLOCKS_PER_SEC; // file transfer time /ms
				memset(ubuf.buf,0,sizeof(ubuf.buf));
				ubuf.bye = 1; // setting udp trans end flag
				sendto(sock, &ubuf, n, 0, (struct sockaddr*)&client_addr,clilen); // send end flag to client
				printf("file size : %zu MB\n",f_size / 1000000);
				printf("Total trans time: %fms\n",duration * 1000);
				printf("File transfer successful\n");
			}
			else 
				printf("No client request file\n");
	
			close(sock); // close sock

		}
		else if(!strcmp("recv",argv[2]))
		{
			int request = 1, f_len = 0, n = 0, ret = 0;
			char f_name[256];
			size_t r_size = 0, f_size = 0;
			sock = socket(AF_INET, SOCK_DGRAM,0); // SOCK_DGRAM for UDP
			if(sock < 0) error("Error opening socket\n");
	
			server = gethostbyname(argv[3]);
			portno = atoi(argv[4]);
			if(server == NULL){
				fprintf(stderr,"ERROR, no such host\n");
				exit(0);
			}
			bzero((char *) &server_addr,sizeof(server_addr));                                       //
			server_addr.sin_family = AF_INET;                                                       // UDP reveiver arguments setting
			bcopy((char *)server->h_addr, (char *) &server_addr.sin_addr.s_addr,server->h_length);  // 
			server_addr.sin_port = htons(portno);                                                   //
			sendto(sock,&request, sizeof(int), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));//send request to server

			recvfrom(sock, &f_len, sizeof(int),0, NULL, NULL);//receive file len 
			recvfrom(sock, f_name, f_len * sizeof(char), 0, NULL, NULL);//receive file name
			recvfrom(sock, &f_size, sizeof(size_t), 0, NULL, NULL);//receive file size

			char ch[20] = "recv_by_udp_";          //
			char newname[512];                     //
			memset(newname,'\0',sizeof(newname));  //   create new file name for receive file
			strcat(newname,ch);                    //
			strcat(newname,f_name);                //
			int fr = open(newname,O_WRONLY | O_CREAT | O_TRUNC,0644); // fr -> receive file name
			printf("receiving...\n");
			while( (n = recvfrom(sock, &ubuf, sizeof(ubuf) ,0,NULL,NULL)) > 0 ) // receive packet from server
			{	
				if(ubuf.bye) break;     // if receive end flag , break
				ret = write(fr,&ubuf,n);// write packet data to fr

			}
			struct stat st;
			fstat(fr,&st);
			r_size = st.st_size; // receive file size
			double loss = 0.0;                                 // calculate packet loss
			loss = (double)(f_size - r_size) / (double)f_size; // print packet loss
			close(fr); // close receive file 
			printf("recived\n");
			printf("packet loss rate: %f\n",loss);
			close(sock); // close socket
		}
	}
	return 0;
}
