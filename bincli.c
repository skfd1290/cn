/* TCP Calculator Client */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct sockaddr_in serv_addr;
int skfd, r, w;
unsigned short serv_port = 25025;
char serv_ip[] = "127.0.0.1";
char buff[128];

int main(){
    int choice;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nTCP CALCULATOR CLIENT\n");

    if((skfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\nCLIENT ERROR: Cannot create socket.\n"); exit(1);
    }
    if(connect(skfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nCLIENT ERROR: Cannot connect.\n"); close(skfd); exit(1);
    }

    printf("\nConnected to server.\n");

    while(1){
        printf("\n===== MENU =====\n1. Decimal to Binary\n2. Binary to Decimal\n3. Solve Expression\n4. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);
        getchar();

        if(choice == 4){
            strcpy(buff, "exit");
            write(skfd, buff, strlen(buff));
            printf("Client exiting.\n");
            break;
        }

        if(choice == 1){
            int num;
            printf("Enter decimal number: ");
            scanf("%d", &num);
            getchar();
            sprintf(buff, "1 %d", num);
        }
        else if(choice == 2){
            printf("Enter binary number: ");
            scanf("%s", buff);
            getchar();
            char temp[128];
            snprintf(temp, sizeof(temp), "2 %s", buff);
            strcpy(buff, temp);
        }
        else if(choice == 3){
            printf("Enter expression: ");
            fgets(buff, sizeof(buff), stdin);
            char temp[128];
            snprintf(temp, sizeof(temp), "3 %s", buff);
            strcpy(buff, temp);
        }
        else{
            printf("Invalid choice. Try again.\n");
            continue;
        }

        w = write(skfd, buff, strlen(buff));
        if(w < 0){ printf("\nCLIENT ERROR: Write failed.\n"); break; }

        r = read(skfd, buff, sizeof(buff));
        if(r < 0){ printf("\nCLIENT ERROR: Read failed.\n"); break; }
        if(r == 0){ printf("\nServer disconnected.\n"); break; }

        buff[r] = '\0';
        printf("RESULT: %s\n", buff);
    }

    close(skfd);
    return 0;
}

