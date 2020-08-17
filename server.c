#include <sys/socket.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h> //strlen
#include <pthread.h> //for threading
#include <stdbool.h>

const size_t MAX_BUF_SIZE = 2048;
const size_t MAX_MSG_SIZE = 2000;

#define panic(...) { fprintf(stderr, __VA_ARGS__); fflush(stderr); exit(1); }

//mutex for writing to file
pthread_mutex_t lock;

const size_t clients_amount = 5;
pthread_t clients[] = {0, 0, 0, 0, 0};

void *connection_handler(void *params);

int main(int argc, char *argv[]) {
    if (argc < 2) panic("Usage: %s [port]\n", argv[0]);

    int port =  atoi(argv[1]);

    int params[] = {0, 0};
    int server_fd, client_fd;
    int err;
    struct sockaddr_in server, client;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) panic("Could not create socket\n");

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    err = bind(server_fd, (struct sockaddr *) &server, sizeof(server));
    if (err < 0) panic("Could not bind socket\n");

    err = listen(server_fd, 128);
    if (err < 0) panic("Could not listen on socket\n");

    printf("Server is listening on %d\n", port);

    while (1) {
        socklen_t client_len = sizeof(client);
        client_fd = accept(server_fd, (struct sockaddr *) &client, &client_len);

        if (client_fd < 0) panic("Could not establish new connection\n");

        puts("[Connection accepted]");
        for (int i = 0; i < clients_amount; ++i) {
            if (clients[i] == 0) {
                params[0] = client_fd;
                params[1] = i;

                pthread_attr_t attr; /* thread attrs */

                pthread_attr_init(&attr);

                if (pthread_create(&clients[i], &attr, connection_handler, (void *) params) > 0) {
                    panic("thread");
                }
                break;
            }
        }

    }

    return 0;
}



bool message_is_get(const char * msg){
    return msg[0] == 'g' && msg[1] == 'e' && msg[2] == 't';
}


//This will handle connection for each client
void *connection_handler(void *params) {
    int sockfd = ((int *) params)[0];
    int client_num = ((int *) params)[1];

    // read size
    ssize_t read_size = 0;
    char write_buf[MAX_BUF_SIZE] = {0};
    char client_message[MAX_MSG_SIZE] =
            "Your connection is established. Now you can send a mesage.\n";

    send(sockfd, client_message, strlen(client_message), SO_NOSIGPIPE); // CHANGE TO MSG_NOSIGNAL ON LINUX

    //Receive a message from client
    while (1) {
        read_size = recv(sockfd, client_message, MAX_BUF_SIZE, SO_NOSIGPIPE);

        if (!read_size) break; // done reading
        if (read_size < 0) panic("Client read failed\n");

        client_message[read_size] = '\0';

        //send(sockfd, client_message, read_size, SO_NOSIGPIPE);

        if(message_is_get(client_message)){
            int get_num = atoi(&client_message[4]);

            pthread_mutex_lock(&lock);
            FILE *fp = fopen("messages", "r");

            char send_msg[MAX_MSG_SIZE];

            while(fgets(send_msg, MAX_MSG_SIZE + 5, fp) != NULL){
                if((send_msg[0] - '0') == get_num)
                    send(sockfd, send_msg, strlen(send_msg), SO_NOSIGPIPE); // CHANGE TO MSG_NOSIGNAL ON LINUX
            }

            fclose(fp);
            pthread_mutex_unlock(&lock);

        } else {
            sprintf(write_buf, "%d - %s", client_num, client_message);

            pthread_mutex_lock(&lock);

            FILE *fp = fopen("messages", "a");

            fprintf(fp,"%s",write_buf);

            fclose(fp);
            pthread_mutex_unlock(&lock);
        }



        memset(client_message, 0, MAX_MSG_SIZE);

    }


    puts("[Client disconnected]");
    fflush(stdout);


    clients[client_num] = 0;
    return 0;
}
