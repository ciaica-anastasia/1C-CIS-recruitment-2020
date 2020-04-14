#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MESSAGE_FILE "./messages"

void *connection_handler(void *);
 

const size_t clients_amount = 5;
pthread_t clients[clients_amount];

int main(int argc , char *argv[])
{
    int sockfd = 0;
    int client_sock = 0;
    int fd_messages = 0;
    int params[] = { 0, 0, 0 }; // sockfd, fd_message, client_num
    
    struct sockaddr_in server;
    struct sockaddr_in client;
    
    size_t sockaddr_in_size = sizeof(struct sockaddr_in);
    
    sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (sockfd == -1)
    {
        perror("socket");
        return 0;
    }
    //puts("Socket created");
     
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9998);
     
    if(bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind");
        return 0;
    }
    //puts("bind done");
     
    listen(sockfd, clients_amount);
    fd_messages = open(MESSAGE_FILE, O_RDWR | O_APPEND | O_CREAT);
    
    //puts("Waiting for incoming connections...");
    
    while((client_sock = accept(sockfd, (struct sockaddr *)&client,
                                (socklen_t*)&sockaddr_in_size)))
    {
        puts("[Connection accepted]");
        
        for (int i = 0; i < clients_amount; ++i)
        {
            if (clients[i] == 0)
            {
                params[0] = sockfd;
                params[1] = fd_messages;
                params[2] = i;
                
                //printf("[in main] %d %d %d\n", sockfd, fd_messages, i);
                
                if(pthread_create(&clients[i], NULL,
                                  connection_handler, (void*)params) < 0)
                {
                    perror("thread");
                }
                break;
            }
        }
        
        //pthread_join(thread_id, NULL);
        //puts("Handler assigned");
    }
    
    close(fd_messages);
    
    if (client_sock < 0)
    {
        perror("accept");
        return 0;
    }
     
    return 0;
}
 

void *connection_handler(void *params)
{
    const size_t MAX_BUF_SIZE = 2048;
    const size_t MAX_MSG_SIZE = 2000;
    
    int sockfd = ((int*)params)[0];
    int fd_message = ((int*)params)[1];
    int client_num = ((int*)params)[2];
    
    ssize_t read_size = 0;
    char write_buf[MAX_BUF_SIZE] = { 0 };
    char client_message[MAX_MSG_SIZE] =
        "Your connection is established. Now you can send a mesage.\n";

    printf("[in thread] %d %d %d\n", sockfd, fd_message, client_num);
    
    write(sockfd, client_message, strlen(client_message));
    
    while((read_size = recv(sockfd, client_message, MAX_MSG_SIZE - 1, 0)) > 0)
    {
        client_message[read_size] = '\0';
        //write(sockfd, client_message, strlen((const char*)client_message));
        
        if (client_message[0] == ';')
        {
            
        }
        else
        {
            sprintf(write_buf, "%d - %s", client_num, client_message);
            write(fd_message, write_buf, strlen(write_buf));
        }
        memset(client_message, 0, MAX_MSG_SIZE);
    }
     
    if(read_size == 0)
    {
        puts("[Client disconnected]");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv");
    }
         
    clients[client_num] = 0;
    
    return 0;
}
