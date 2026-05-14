/* TCP Calculator Server Program */

#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

struct sockaddr_in serv_addr, cli_addr;

int listenfd, connfd, r, w, val, cli_addr_len;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char buff[128];

int main()
{
    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, (&serv_addr.sin_addr));

    printf("\nTCP CALCULATOR SERVER.\n");

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nSERVER ERROR: Cannot create socket.\n");
        exit(1);
    }

    if((bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("\nSERVER ERROR: Cannot bind.\n");
        close(listenfd);
        exit(1);
    }

    if((listen(listenfd, 5)) < 0)
    {
        printf("\nSERVER ERROR: Cannot listen.\n");
        close(listenfd);
        exit(1);
    }

    cli_addr_len = sizeof(cli_addr);

    for( ; ; )
    {
        printf("\nSERVER: Listening for clients...\n");

        if((connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_addr_len)) < 0)
        {
            printf("\nSERVER ERROR: Cannot accept client connections.\n");
            close(listenfd);
            exit(1);
        }

        printf("\nSERVER: Connection from client %s accepted.\n", inet_ntoa(cli_addr.sin_addr));

        while(1)
        {
            if((r = read(connfd, buff, 128)) < 0)
            {
                printf("\nSERVER ERROR: Cannot receive.\n");
            }
            else if(r == 0)
            {
                printf("\nClient disconnected.\n");
                break;
            }
            else
            {
                buff[r] = '\0';
                printf("\nCLIENT REQUEST: %s", buff);
            }

            /* 🔥 CALCULATOR LOGIC */
            int a, b, result;
            char op;

            if(sscanf(buff, "%d %c %d", &a, &op, &b) == 3)
            {
                switch(op)
                {
                    case '+': result = a + b; break;
                    case '-': result = a - b; break;
                    case '*': result = a * b; break;
                    case '/': 
                        if(b != 0)
                            result = a / b;
                        else
                        {
                            strcpy(buff, "Error: Division by zero\n");
                            write(connfd, buff, strlen(buff));
                            continue;
                        }
                        break;
                    default:
                        strcpy(buff, "Error: Invalid operator\n");
                        write(connfd, buff, strlen(buff));
                        continue;
                }

                sprintf(buff, "Result = %d\n", result);
            }
            else
            {
                strcpy(buff, "Error: Invalid format (use: a + b)\n");
            }

            if((w = write(connfd, buff, strlen(buff))) < 0)
            {
                printf("\nSERVER ERROR: Cannot send result.\n");
            }
            else
            {
                printf("SERVER: Result sent.\n");
            }
        }

        close(connfd);
    }
}
