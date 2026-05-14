




/* TCP Command Execution Server */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

struct sockaddr_in serv_addr, cli_addr;
int listenfd, connfd, r, w, cli_len;
unsigned short serv_port = 25035;
char serv_ip[] = "127.0.0.1";
char buff[256];

int main(){
    char response[2048];

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nTCP COMMAND SERVER\n");

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\nSERVER ERROR: Cannot create socket.\n"); exit(1);
    }
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nSERVER ERROR: Cannot bind.\n"); close(listenfd); exit(1);
    }
    if(listen(listenfd, 5) < 0){
        printf("\nSERVER ERROR: Cannot listen.\n"); close(listenfd); exit(1);
    }

    cli_len = sizeof(cli_addr);

    while(1){
        printf("\nSERVER: Waiting for client...\n");

        if((connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_len)) < 0){
            printf("\nSERVER ERROR: Cannot accept client.\n"); continue;
        }

        printf("\nConnected to %s\n", inet_ntoa(cli_addr.sin_addr));

        while(1){
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(connfd, &readfds);
            FD_SET(0, &readfds);

            select(connfd + 1, &readfds, NULL, NULL, NULL);

            if(FD_ISSET(0, &readfds)){
                fgets(buff, sizeof(buff), stdin);
                if(strncmp(buff, "exit", 4) == 0){
                    printf("SERVER: Disconnecting client...\n");
                    break;
                }
            }

            if(FD_ISSET(connfd, &readfds)){
                r = read(connfd, buff, sizeof(buff));
                if(r < 0){ printf("\nSERVER ERROR: Read failed.\n"); break; }
                if(r == 0){ printf("\nClient disconnected.\n"); break; }

                buff[r] = '\0';
                printf("CLIENT CMD: %s", buff);

                if(strncmp(buff, "exit", 4) == 0){
                    printf("Client exited.\n"); break;
                }

                /* HANDLE cd */
                if(strncmp(buff, "cd", 2) == 0){
                    char path[128];
                    if(sscanf(buff, "cd %s", path) != 1){
                        strcpy(response, "Invalid format");
                    } else{
                        if(chdir(path) == 0)
                            strcpy(response, "Directory changed");
                        else
                            strcpy(response, "Failed to change directory");
                    }

                    printf("SERVER RESULT: %s\n", response);
                    write(connfd, response, strlen(response));
                    continue;
                }

                /* EXECUTE COMMAND */
                FILE *fp = popen(buff, "r");
                if(fp == NULL){
                    strcpy(response, "Error executing command");
                    write(connfd, response, strlen(response));
                    continue;
                }

                char temp[256];
                response[0] = '\0';

                while(fgets(temp, sizeof(temp), fp) != NULL){
                    if(strlen(response) + strlen(temp) < sizeof(response)-1)
                        strcat(response, temp);
                    else
                        break;
                }

                /* SMALL OUTPUT */
                if(feof(fp)){
                    printf("SERVER RESULT:\n%s\n", response);
                    write(connfd, response, strlen(response));
                }
                /* LARGE OUTPUT */
                else{
                    FILE *file = fopen("output.txt", "w");
                    if(file == NULL){
                        strcpy(response, "File error");
                        write(connfd, response, strlen(response));
                    } else{
                        fputs(response, file);

                        while(fgets(temp, sizeof(temp), fp) != NULL)
                            fputs(temp, file);

                        fclose(file);

                        FILE *f = fopen("output.txt", "r");
                        fseek(f, 0, SEEK_END);
                        long size = ftell(f);
                        rewind(f);

                        sprintf(response, "FILE %ld", size);
                        write(connfd, response, strlen(response));

                        int n;
                        while((n = fread(temp, 1, sizeof(temp), f)) > 0)
                            write(connfd, temp, n);

                        fclose(f);

                        printf("SERVER RESULT: Sent as file (%ld bytes)\n", size);
                    }
                }

                pclose(fp);
            }
        }

        close(connfd);
    }

    close(listenfd);
    return 0;
}

