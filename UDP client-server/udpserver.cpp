#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>   
#include<unistd.h>  
#include<iostream>
#include<fstream>
#include<errno.h>
#include<sys/ioctl.h>
#include<chrono>
#include<sys/time.h>
using namespace std;


struct sockaddr_in server, client;


int receive_file(int socket);


int create_socket(){
	
	int socket_desc = socket(AF_INET , SOCK_DGRAM , 0);
  	if (socket_desc == -1){
  
	 	printf("Could not create socket");
	 	return 0;
  	}
  	
  	//Prepare the server sockaddr_in structure
  	server.sin_family = AF_INET;
  	
  	//server.sin_addr.s_addr = INADDR_ANY;
  	server.sin_addr.s_addr = inet_addr("10.0.2.5");
  	server.sin_port = htons( 8889 );

  	
  	//Bind 
 	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
 		puts("bind failed");
   		return 1;
 	}
	
	puts("bind done");
	return socket_desc;
		
}

 
// Function to check server status
void server_status(int socket){
	
	char buffer[] = "hello";
	char read_buffer[255];
	int stat , trials = 0;
	int client_len = sizeof(client);
	
	do{
		stat = recvfrom(socket , &read_buffer , 255,0,(struct sockaddr*)&client,(socklen_t*)&client_len);
			
	
	}while(stat < 0 and trials++ < 5);
	
	
	stat = 0 , trials = 0;
	do{
		
		stat = sendto(socket , &buffer , sizeof(buffer) ,0 ,(struct sockaddr*)&client,client_len);
		
	
	}while(stat < 0 and trials++ < 5);
	
}

//Function to check packet stored
void packet_status(int socket){
	char buffer[] = "Packet Stored";
	int client_len = sizeof(client);
	int stat = 0;
	do{
		
		stat = sendto(socket , &buffer ,sizeof(buffer) , 0 ,(struct sockaddr*)&client , client_len);	
		
	}while(stat < 0);
	
}

/// Function to connect with client
void connect_client(int socket_desc ){
	while(1){
		
		server_status(socket_desc);
		receive_file(socket_desc);
		
	}	
}




// Function for receiving file from the client
int receive_file(int socket){ 
	
	
	int buffersize = 0, recv_size = 0,size = 0, read_size, write_size, packet_index =1,stat;
	char from_ip[1024] = "";
	int client_len = sizeof(client);
	char output_file_array[10241], verify = '1';
	int no_of_tries = 0;
	FILE *file_ptr;
	char filename[255];
	
	// Find name of the file
	do{
		
		stat = recvfrom(socket , &filename , sizeof(filename) ,0 ,(struct sockaddr*)&client,(socklen_t*)&client_len);
	
	}while(stat < 0);
	
	if(strcmp(filename,"@reset@") == 0){
		printf("\n#####################################################\n");
		printf("Server Reset\n");
		close(socket);
		int socket_desc = create_socket();
		connect_client(socket_desc);
	
	}
	
	
	
	//Find the size of the file
	no_of_tries = 0;
	do{
		
		stat = recvfrom(socket , &size , sizeof(int) ,0 ,(struct sockaddr*)&client,(socklen_t*)&client_len);
		
		
		
	}while(stat<0 and no_of_tries++ < 5);
	//printing client ip and port
	inet_ntop(AF_INET , &client.sin_addr,from_ip,sizeof(from_ip));
  	printf("message received from  %s :: %d \n",from_ip , ntohs(client.sin_port));
	
	printf("Packet received.\n");
	printf("File name received:%s\n",filename);
	printf("Total file size: %i\n",size);
	printf(" \n");

	
	char buffer[] = "Got it";

	
	//Send our verification signal
	do{
		
		stat = sendto(socket , &buffer ,sizeof(buffer) , 0 ,(struct sockaddr*)&client , client_len);
		
		//printing client ip and port
		inet_ntop(AF_INET , &client.sin_addr,from_ip,sizeof(from_ip));
  		printf("sending to this address%s :: %d \n",from_ip , ntohs(client.sin_port));
  		
	
	
	}while(stat<0);

	printf("Reply sent\n");
	printf(" \n");
	
	file_ptr = fopen(filename, "w");

	if( file_ptr == NULL) {
		printf("Error has occurred. File could not be opened.\n");
		return -1; 
	}

	
	//Loop while we have not received the entire file yet
	int count = 1;
	int need_exit = 0;
	struct timeval timeout_for_select = {30,0};/// after this timeout server resets
	int actual_timeout = timeout_for_select.tv_sec - 10;
	fd_set fds;
	int buffer_fd, buffer_out;
	
	int flag;
	packet_status(socket);
	while(recv_size < size) {
		std::chrono::time_point<std::chrono::system_clock> start, end;

		start = std::chrono::system_clock::now();
		FD_ZERO(&fds);
		FD_SET(socket,&fds);
		//start = std::chrono::system_clock::now();
		
		buffer_fd = select(FD_SETSIZE,&fds,NULL,NULL,&timeout_for_select);
		end = std::chrono::system_clock::now();	
		std::chrono::duration<double> elapsed_seconds = end - start; 
		
		if(buffer_fd == 0){
			flag = 1;
				
			//printf("bad descriptor\n");
			
		}
		if (buffer_fd < 0){
		    printf("error: buffer read timeout expired.\n");// if client stops server waits for sometime and then resets
			printf("\n#####################################################\n");
			printf("Server Reset\n");
			close(socket);
			int socket_desc = create_socket();
			connect_client(socket_desc);
		}
		
		else{	
			// if chrono time difference >= actual_timeout ---> reset server
			if(flag == 1 and elapsed_seconds.count() >= actual_timeout){
				printf("\n#####################################################\n");
				printf("Server Reset\n");
				close(socket);
				int socket_desc = create_socket();
				connect_client(socket_desc);

			}
			else{
				do{
					
					read_size = recvfrom(socket , output_file_array ,sizeof(output_file_array) ,0 ,0 ,0);
					flag = 0;
					
				}while(read_size <0);
			}
		}   		
		
		    printf("Packet number received: %i\n",packet_index);
		    printf("Packet size: %i\n",read_size);


		    //Write the currently read data into our file
		    write_size = fwrite(output_file_array,1,read_size, file_ptr); 
		    printf("Written file size: %i\n",write_size); 

		    if(read_size !=write_size) {
	            printf("error in read write\n");    
	        }
			

	         //Increment the total number of bytes read
	        recv_size += read_size;
	        packet_index++;
	        printf("Total received file size: %i\n",recv_size);
	        printf(" \n");
	        printf(" \n");
		
	
			packet_status(socket);
			cout<<"Packet_status called :"<<count++<<"times"<<endl;	
		

	}
  	fclose(file_ptr);
  	printf("File name received:%s\n",filename);
  	printf("File successfully Received!\n");
  	
  	return 1;
}



int main(int argc , char *argv[]){
	
	
  	int socket_desc = create_socket();
  	
	connect_client(socket_desc);
	
	close(socket_desc);
	
	return 0;
}
