/* TCP Calculator Server */

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
int listenfd, connfd, r, w, cli_addr_len;
unsigned short serv_port = 25025;
char serv_ip[] = "127.0.0.1";
char buff[128];

void decToBinary(int n, char *res){
    char temp[64]; int i = 0;
    if(n == 0){ strcpy(res, "0"); return; }
    while(n > 0){ temp[i++] = (n % 2) + '0'; n /= 2; }
    int j = 0; while(i > 0) res[j++] = temp[--i];
    res[j] = '\0';
}

int binToDecimal(char *bin){
    int result = 0;
    for(int i = 0; bin[i] != '\0'; i++){
        if(bin[i] != '0' && bin[i] != '1') return -1;
        result = result * 2 + (bin[i] - '0');
    }
    return result;
}

int main(){
    char response[128];
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nTCP CALCULATOR SERVER\n");

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\nSERVER ERROR: Cannot create socket.\n"); exit(1);
    }
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nSERVER ERROR: Cannot bind.\n"); close(listenfd); exit(1);
    }
    if(listen(listenfd, 5) < 0){
        printf("\nSERVER ERROR: Cannot listen.\n"); close(listenfd); exit(1);
    }

    cli_addr_len = sizeof(cli_addr);

    while(1){
        printf("\nSERVER: Waiting for client...\n");
        if((connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &cli_addr_len)) < 0){
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
                printf("CLIENT: %s", buff);

                if(strncmp(buff, "exit", 4) == 0){
                    printf("Client exited.\n"); break;
                }

                if(buff[0] == '1'){
                    int num;
                    if(sscanf(buff, "1 %d", &num) != 1) strcpy(response, "Invalid format");
                    else {printf("\n");decToBinary(num, response);}
                }
                else if(buff[0] == '2'){
                    char bin[100]; int val;
                    if(sscanf(buff, "2 %s", bin) != 1) strcpy(response, "Invalid format");
                    else{
                        val = binToDecimal(bin);
                        if(val == -1) strcpy(response, "Invalid binary number");
                        else {printf("\n");sprintf(response, "%d", val);}
                    }
                }
                else if(buff[0] == '3'){
                    char expr[100], command[200]; FILE *fp;
                    if(sscanf(buff, "3 %[^\n]", expr) != 1) strcpy(response, "Invalid format");
                    else{
                        sprintf(command, "echo \"%s\" | bc", expr);
                        fp = popen(command, "r");
                        if(fp == NULL) strcpy(response, "Error executing bc");
                        else{
                            if(fgets(response, sizeof(response), fp) == NULL) strcpy(response, "Invalid expression");
                            else response[strcspn(response, "\n")] = '\0';
                            pclose(fp);
                        }
                    }
                }
                else strcpy(response, "Invalid choice");

                printf("SERVER RESULT: %s\n", response);

                w = write(connfd, response, strlen(response));
                if(w < 0){ printf("\nSERVER ERROR: Write failed.\n"); break; }
            }
        }
        close(connfd);
    }
    close(listenfd);
    return 0;
}







