/* TCP Calculator Client Program */

#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

struct sockaddr_in serv_addr;

int sockfd, r, w;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char buff[128];

int main()
{
    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, (&serv_addr.sin_addr));

    printf("\nTCP CALCULATOR CLIENT\n");

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nCLIENT ERROR: Cannot create socket.\n");
        exit(1);
    }

    if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nCLIENT ERROR: Cannot connect to server.\n");
        close(sockfd);
        exit(1);
    }

    printf("\nConnected to server.\n");

    while(1)
    {
        printf("\nEnter expression (a + b) or 'exit': ");
        fgets(buff, 128, stdin);

        if(strncmp(buff, "exit", 4) == 0)
        {
            printf("Disconnecting...\n");
            break;
        }

        /* send request */
        if((w = write(sockfd, buff, strlen(buff))) < 0)
        {
            printf("\nCLIENT ERROR: Cannot send.\n");
            break;
        }

        /* receive response */
        if((r = read(sockfd, buff, 128)) < 0)
        {
            printf("\nCLIENT ERROR: Cannot receive.\n");
            break;
        }
        else if(r == 0)
        {
            printf("\nServer disconnected.\n");
            break;
        }

        buff[r] = '\0';
        printf("SERVER RESPONSE: %s", buff);
    }

    close(sockfd);
}
