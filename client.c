#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

int main(int argc, char **argv)
{
    const size_t MAX_BUF_SIZE = 2048;
    int sockfd = 0;
    int portno = 0;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char message[MAX_BUF_SIZE] = { 0 };

    //Fill block of memory
    memset((void*)&serv_addr, 0, sizeof(serv_addr));

    if (argc < 3)
    {
        printf("usage: %s hostname port\n", argv[0]);
        return 0;
    }

    portno = atoi(argv[2]);

    //Create a socket point
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("socket");
        return 0;
    }

    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        printf("No such host\n");
        return 0;
    }

    serv_addr.sin_family = AF_INET;

    //Copy block of memory
    memcpy((char*)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    //Connect to the server
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        return 0;
    }

    recv(sockfd, message, MAX_BUF_SIZE - 1, 0);
    printf("%s\n", message);

    //Ask for a message from the user, this message will be read by server
    printf("Message: ");
    fgets(message, MAX_BUF_SIZE - 1, stdin);

    while (strcmp(message, "quit\n\0") != 0)
    {
        //Send message to the server
        if (write(sockfd, message, strlen(message)) < 0)
        {
            perror("write");
            return 0;
        }

        if (message[0] == ';')
        {
            //Read server response
            if (recv(sockfd, message, MAX_BUF_SIZE - 1, SO_NOSIGPIPE) < 0)
            {
                perror("read");
                return 0;
            }
            printf("%s\n", message);
        }

        printf("Message: ");

        memset(message, 0, MAX_BUF_SIZE);
        fgets(message, MAX_BUF_SIZE - 1, stdin);
    }

    close(sockfd);

    return 0;
}
