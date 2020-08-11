#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>   
#include<sys/ioctl.h>
#include<unistd.h>  
#include<iostream>
#include<fstream>
#include<errno.h>

using namespace std;

struct sockaddr_in server,client;

// check server status
int server_status(int socket){
	
	char buffer[] = "hello";
	char read_buffer[255];
	int stat , trials = 0;
	int server_size = sizeof(server);
	
	do{
		stat = sendto(socket , &buffer , sizeof(buffer) ,0 ,(struct sockaddr*)&server,server_size);	
		
	}while(stat < 0 and trials++ < 5);
	
	sleep(2);
	stat = 0 , trials = 0;
	do{
	
		stat = recvfrom(socket , &read_buffer , 255,MSG_DONTWAIT,(struct sockaddr*)&server,(socklen_t*)&server_size);
		
	}while(stat < 0 and trials++ < 5);
	
	if(stat < 0)
		return 0;
	else
		return 1;
}
	
	

int packet_status(int socket){
	char buffer[255];
	int server_size = sizeof(server);
	int stat = 0;
	struct timeval timeout = {20,0};/// after this timeout client stops

	fd_set fds;
	int buffer_fd, buffer_out;
	FD_ZERO(&fds);
	FD_SET(socket,&fds);
	buffer_fd = select(FD_SETSIZE,&fds,NULL,NULL,&timeout);
	if (buffer_fd <= 0){
		printf("error: Server Not Responding !!.\n");
		printf("Please try again after some time.\n");
		close(socket);
		exit(0);
		
	}
	else{
		do{
			stat = recvfrom(socket , &buffer , 255,0,(struct sockaddr*)&server,(socklen_t*)&server_size);
		
		}while(stat < 0);
	
		return 1;
	}
}

// Function for sending file to the server
int send_file(int socket , char filename[]){
	
	
	FILE *file_ptr;
	int size, read_size, stat, packet_index , count = 0;
	char send_buffer[10240],read_buffer[256];
	
	int server_size = sizeof(server);
		
	packet_index = 1;
	
	
	if(file_ptr = fopen(filename, "r")){
		printf("Getting File Size\n");
		fseek(file_ptr, 0, SEEK_END);  
		size = ftell(file_ptr);			// tells the size of the file
		fseek(file_ptr, 0, SEEK_SET);
		printf("Total File size: %i\n",size);

		//sending filename
		printf("Sending File name : %s\n",filename);
		char newfilename[255];
		strcpy(newfilename , filename);
		sendto(socket , &newfilename , sizeof(newfilename) , 0 ,(struct sockaddr*)&server,server_size); 
	}
	else{	
		char terminate[]="@reset@";
		printf("Error Opening File\n");
		sendto(socket , &terminate , sizeof(terminate) , 0 ,(struct sockaddr*)&server,server_size);	
		cout<<"Termination request sent\n"<<endl;
		sleep(3);
		return 0;	
	}
	
	
	
	
	// sending file size
	printf("Sending File Size\n");
	sendto(socket , (void*) &size , sizeof(int) , 0 ,(struct sockaddr*)&server,server_size);

	//Send File as Byte Array
	printf("Sending File as Byte Array\n");

	
	/***** First reply from server ******/
	
	//Read while we get errors that are due to signals.
	do{ 
		stat = recvfrom(socket , &read_buffer , 255,0,(struct sockaddr*)&server,(socklen_t*)&server_size);
	    
	    
	
	}while (stat < 0);
	
	printf("Received data in socket\n");
	printf("Socket data: %s\n", read_buffer);
	
	
	while(!feof(file_ptr)) {
		if(packet_status(socket)){
	    //Read from the file into our send_buffer
	    read_size = fread(send_buffer, 1, sizeof(send_buffer)-1, file_ptr);
		
	   //Send data through our socket 
	   //usleep(3000);
	   
			do{
		   		 
		   		stat = sendto(socket , send_buffer , read_size , 0 ,(struct sockaddr*)&server,server_size);
			}while (stat < 0);
			
			printf("Packet Number: %i\n",packet_index);
			printf("Packet Size Sent: %i\n",read_size);     
			printf(" \n\n");
			

			packet_index++;  
		
		
	   		//Zero out our send buffer
	   		bzero(send_buffer, sizeof(send_buffer));
	   }
	   cout<<"Packet_status called : "<<++count<<"times"<<endl;
	}
	printf("Complete File Sent\n");
}



int main(int argc , char *argv[]){
	
	if(argc == 4){
		int socket_desc , server_size = 0;
		//struct sockaddr_in server,client;
		char *parray;

		
		//Create socket
		socket_desc = socket(AF_INET , SOCK_DGRAM , 0);

		if (socket_desc == -1) {
			printf("Could not create socket");
		}

		//Prepare server address structure
		memset(&server,0,sizeof(server));
		char *ip = argv[1];
		char *port = argv[2];
		
		printf("\n %s\n",ip);
		server.sin_addr.s_addr = inet_addr(ip);
		server.sin_family = AF_INET;
		//server.sin_port = htons( 8889 );
		server.sin_port = htons( stoi(port) );
		
		// prepare client address structure
		memset(&client,0,sizeof(client));
		client.sin_addr.s_addr = inet_addr("10.0.2.5");
		client.sin_family = AF_INET;
		client.sin_port = htons(9000);
		
		
		// bind for client side
		if( bind(socket_desc,(struct sockaddr *)&client , sizeof(client)) < 0){
	 
	   		puts("bind failed");
	   		return 1;
	 	}
		puts("bind done");
		char filename[255] = "";
		strcpy(filename , argv[3]);
		
		
		if(server_status(socket_desc)){
			printf("connection established\n");
			send_file(socket_desc,filename );
		}
		else
			printf("\nServer Unreachable!!\n");
		close(socket_desc);
		
		fflush(stdout);
	}
	else 
		printf("\nCommand line arguments mismatch!! Restart the program with correct arguments");
	return 0;
}
