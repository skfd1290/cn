TCPcommandclient
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
unsigned short serv_port = 25035;
char serv_ip[] = "127.0.0.1";
char buff[256];

int main(){
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nTCP COMMAND CLIENT\n");

    if((skfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("\nCLIENT ERROR: Cannot create socket.\n"); exit(1);
    }

    if(connect(skfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        perror("CLIENT ERROR: Cannot connect");
        close(skfd); exit(1);
    }

    printf("\nConnected to server.\n");

    while(1){
        printf("\nEnter command (or exit): ");
        fgets(buff, sizeof(buff), stdin);

        w = write(skfd, buff, strlen(buff));
        if(w < 0){ printf("\nCLIENT ERROR: Write failed.\n"); break; }

        if(strncmp(buff, "exit", 4) == 0){
            printf("Client exiting.\n");
            break;
        }

        r = read(skfd, buff, sizeof(buff));
        if(r < 0){ printf("\nCLIENT ERROR: Read failed.\n"); break; }
        if(r == 0){ printf("\nServer disconnected.\n"); break; }

        buff[r] = '\0';

        /* FILE CASE */
        if(strncmp(buff, "FILE", 4) == 0){
            long filesize;
            sscanf(buff, "FILE %ld", &filesize);

            FILE *fp = fopen("received.txt", "w");
            if(fp == NULL){ printf("File error\n"); break; }

            printf("Receiving %ld bytes...\n", filesize);

            long received = 0;

            while(received < filesize){
                r = read(skfd, buff, sizeof(buff));
                if(r <= 0) break;

                fwrite(buff, 1, r, fp);
                received += r;
            }

            fclose(fp);
            printf("Saved to received.txt\n");
        }
        else{
            printf("OUTPUT:\n%s\n", buff);
        }
    }

    close(skfd);
    return 0;

TCPCommandServer
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
}

UDPBodmasClient
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>

struct sockaddr_in serv_addr;
socklen_t serv_len;

int main()
{
    int skfd, r, w;

    unsigned short serv_port = 25020;
    char serv_ip[] = "127.0.0.1";

    char rbuff[512];
    char sbuff[512];

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nUDP CALCULATOR CLIENT.\n");

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nCLIENT ERROR: Cannot create socket.\n");
        exit(1);
    }

    serv_len = sizeof(serv_addr);

    while(1)
    {
        printf("\nEnter expression or 'exit': ");
        if(fgets(sbuff, sizeof(sbuff), stdin) == NULL) break;

        w = sendto(skfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&serv_addr, serv_len);
        if(w < 0)
        {
            printf("\nCLIENT ERROR: Cannot send.\n");
            break;
        }

        r = recvfrom(skfd, rbuff, sizeof(rbuff)-1, 0, (struct sockaddr*)&serv_addr, &serv_len);
        if(r < 0)
        {
            printf("\nCLIENT ERROR: Cannot receive.\n");
            break;
        }

        rbuff[r] = '\0';

        if(strncmp(rbuff, "exit", 4) == 0)
        {
            printf("SERVER: exit\n");
            break;
        }

        printf("SERVER: %s", rbuff);

        r = recvfrom(skfd, rbuff, sizeof(rbuff)-1, 0, (struct sockaddr*)&serv_addr, &serv_len);
        if(r > 0)
        {
            rbuff[r] = '\0';
            if(strncmp(rbuff, "exit", 4) == 0)
            {
                printf("SERVER: exit\n");
                break;
            }
            printf("SERVER MSG: %s", rbuff);
        }

        if(strncmp(sbuff, "exit", 4) == 0) break;
    }

    close(skfd);
}
UDPBodmasServer
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<ctype.h>

struct sockaddr_in serv_addr, cli_addr, active_addr;
socklen_t cli_len;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char rbuff[512];
char sbuff[512];

int precedence(char op)
{
    if(op=='+' || op=='-') return 1;
    if(op=='*' || op=='/') return 2;
    return 0;
}

int applyOp(int a, int b, char op, int *ok)
{
    if(op=='+') return a+b;
    if(op=='-') return a-b;
    if(op=='*') return a*b;
    if(op=='/')
    {
        if(b==0){ *ok=0; return 0; }
        return a/b;
    }
    *ok=0;
    return 0;
}

int infixToPostfix(const char *infix, char *postfix)
{
    char ops[512];
    int top = -1;
    int k = 0;
    int i = 0;

    while(infix[i] != '\0')
    {
        if(isspace((unsigned char)infix[i])) { i++; continue; }

        if(isdigit((unsigned char)infix[i]))
        {
            while(isdigit((unsigned char)infix[i]))
            {
                postfix[k++] = infix[i++];
            }
            postfix[k++] = ' ';
            continue;
        }

        if(infix[i] == '(')
        {
            ops[++top] = infix[i++];
            continue;
        }

        if(infix[i] == ')')
        {
            while(top >= 0 && ops[top] != '(')
            {
                postfix[k++] = ops[top--];
                postfix[k++] = ' ';
            }
            if(top < 0) return 0;
            top--;
            i++;
            continue;
        }

        if(infix[i]=='+' || infix[i]=='-' || infix[i]=='*' || infix[i]=='/')
        {
            char op = infix[i++];
            while(top >= 0 && ops[top] != '(' && precedence(ops[top]) >= precedence(op))
            {
                postfix[k++] = ops[top--];
                postfix[k++] = ' ';
            }
            ops[++top] = op;
            continue;
        }

        return 0;
    }

    while(top >= 0)
    {
        if(ops[top] == '(') return 0;
        postfix[k++] = ops[top--];
        postfix[k++] = ' ';
    }

    postfix[k] = '\0';
    return 1;
}

int evalPostfix(const char *postfix, int *ok)
{
    int st[512];
    int top = -1;
    int i = 0;

    *ok = 1;

    while(postfix[i] != '\0')
    {
        while(isspace((unsigned char)postfix[i])) i++;
        if(postfix[i] == '\0') break;

        if(isdigit((unsigned char)postfix[i]))
        {
            int val = 0;
            while(isdigit((unsigned char)postfix[i]))
            {
                val = val*10 + (postfix[i]-'0');
                i++;
            }
            st[++top] = val;
            continue;
        }

        if(postfix[i]=='+' || postfix[i]=='-' || postfix[i]=='*' || postfix[i]=='/')
        {
            if(top < 1){ *ok = 0; return 0; }
            int b = st[top--];
            int a = st[top--];
            int res = applyOp(a, b, postfix[i], ok);
            if(!(*ok)) return 0;
            st[++top] = res;
            i++;
            continue;
        }

        *ok = 0;
        return 0;
    }

    if(top != 0){ *ok = 0; return 0; }
    return st[top];
}

int same_client(struct sockaddr_in *a, struct sockaddr_in *b)
{
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

int main()
{
    int sockfd;
    int active = 0;

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nUDP CALCULATOR SERVER.\n");

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nSERVER ERROR: Cannot create socket.\n");
        exit(1);
    }

    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nSERVER ERROR: Cannot bind.\n");
        close(sockfd);
        exit(1);
    }

    cli_len = sizeof(cli_addr);

    while(1)
    {
        printf("\nSERVER: Waiting for client...\n");
        r = recvfrom(sockfd, rbuff, sizeof(rbuff)-1, 0, (struct sockaddr*)&cli_addr, &cli_len);
        if(r < 0) continue;

        rbuff[r] = '\0';

        if(!active)
        {
            active_addr = cli_addr;
            active = 1;
            printf("\nSERVER: Active client %s:%d\n", inet_ntoa(active_addr.sin_addr), ntohs(active_addr.sin_port));
        }

        if(!same_client(&cli_addr, &active_addr))
        {
            strcpy(sbuff, "Server busy\n");
            sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&cli_addr, cli_len);
            continue;
        }

        if(strncmp(rbuff, "exit", 4) == 0)
        {
            strcpy(sbuff, "exit");
            sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));
            active = 0;
            continue;
        }

        char postfix[512];
        int ok;
        int result;

        if(!infixToPostfix(rbuff, postfix))
        {
            strcpy(sbuff, "Error: invalid expression\n");
        }
        else
        {
            result = evalPostfix(postfix, &ok);
            if(!ok) strcpy(sbuff, "Error: evaluation failed\n");
            else sprintf(sbuff, "Result = %d\n", result);
        }

        sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));

        printf("SERVER: Type message to send (or exit): ");
        fflush(stdout);
        if(fgets(sbuff, sizeof(sbuff), stdin) == NULL) continue;

        if(strncmp(sbuff, "exit", 4) == 0)
        {
            strcpy(sbuff, "exit");
            sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));
            active = 0;
            continue;
        }

        sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));
    }
}

UDPCalculatorClient
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>

int main()
{
    struct sockaddr_in serv_addr;
    socklen_t serv_len;

    int skfd, r, w;

    unsigned short serv_port = 25020;
    char serv_ip[] = "127.0.0.1";

    char rbuff[256];
    char sbuff[256];

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nUDP CALCULATOR CLIENT.\n");

    if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nCLIENT ERROR: Cannot create socket.\n");
        exit(1);
    }

    serv_len = sizeof(serv_addr);

    while(1)
    {
        printf("\nEnter expression (a + b) or 'exit': ");
        if(fgets(sbuff, sizeof(sbuff), stdin) == NULL) break;

        w = sendto(skfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&serv_addr, serv_len);
        if(w < 0)
        {
            printf("\nCLIENT ERROR: Cannot send.\n");
            break;
        }

        r = recvfrom(skfd, rbuff, sizeof(rbuff)-1, 0, (struct sockaddr*)&serv_addr, &serv_len);
        if(r < 0)
        {
            printf("\nCLIENT ERROR: Cannot receive.\n");
            break;
        }

        rbuff[r] = '\0';

        if(strncmp(rbuff, "exit", 4) == 0)
        {
            printf("SERVER: exit\n");
            break;
        }

        printf("SERVER: %s", rbuff);

        r = recvfrom(skfd, rbuff, sizeof(rbuff)-1, 0, (struct sockaddr*)&serv_addr, &serv_len);
        if(r > 0)
        {
            rbuff[r] = '\0';
            if(strncmp(rbuff, "exit", 4) == 0)
            {
                printf("SERVER: exit\n");
                break;
            }
            printf("SERVER MSG: %s", rbuff);
        }

        if(strncmp(sbuff, "exit", 4) == 0) break;
    }

    close(skfd);
}
UDPclaculatorServer
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

struct sockaddr_in serv_addr, cli_addr, active_addr;
socklen_t cli_len;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char rbuff[256];
char sbuff[256];

int same_client(struct sockaddr_in *a, struct sockaddr_in *b)
{
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

int main()
{
    int sockfd, r, w;
    int active = 0;

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nUDP CALCULATOR SERVER.\n");

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nSERVER ERROR: Cannot create socket.\n");
        exit(1);
    }

    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nSERVER ERROR: Cannot bind.\n");
        close(sockfd);
        exit(1);
    }

    cli_len = sizeof(cli_addr);

    while(1)
    {
        printf("\nSERVER: Waiting for client...\n");
        r = recvfrom(sockfd, rbuff, sizeof(rbuff)-1, 0, (struct sockaddr*)&cli_addr, &cli_len);
        if(r < 0) continue;

        rbuff[r] = '\0';

        if(!active)
        {
            active_addr = cli_addr;
            active = 1;
            printf("\nSERVER: Active client %s:%d\n", inet_ntoa(active_addr.sin_addr), ntohs(active_addr.sin_port));
        }

        if(!same_client(&cli_addr, &active_addr))
        {
            strcpy(sbuff, "Server busy\n");
            sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&cli_addr, cli_len);
            continue;
        }

        if(strncmp(rbuff, "exit", 4) == 0)
        {
            strcpy(sbuff, "exit");
            sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));
            active = 0;
            continue;
        }

        int a, b, result;
        char op;

        if(sscanf(rbuff, "%d %c %d", &a, &op, &b) == 3)
        {
            int ok = 1;

            switch(op)
            {
                case '+': result = a + b; break;
                case '-': result = a - b; break;
                case '*': result = a * b; break;
                case '/':
                    if(b != 0) result = a / b;
                    else ok = 0;
                    break;
                default:
                    ok = 0;
            }

            if(ok) sprintf(sbuff, "Result = %d\n", result);
            else strcpy(sbuff, "Error\n");
        }
        else
        {
            strcpy(sbuff, "Error\n");
        }

        sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));

        printf("SERVER: Type message to send (or exit): ");
        fflush(stdout);

        if(fgets(sbuff, sizeof(sbuff), stdin) == NULL) continue;

        if(strncmp(sbuff, "exit", 4) == 0)
        {
            strcpy(sbuff, "exit");
            sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));
            active = 0;
            continue;
        }

        sendto(sockfd, sbuff, strlen(sbuff), 0, (struct sockaddr*)&active_addr, sizeof(active_addr));
    }
}

UDPChatClient
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int sockfd, r;
    struct sockaddr_in serv_addr;
    socklen_t serv_len;
    unsigned short serv_port = 25020;
    char serv_ip[] = "127.0.0.1";
    char buff[128];

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    serv_len = sizeof(serv_addr);

    printf("UDP CHAT CLIENT STARTED\n");

    while (1) {
        printf("Client: ");
        if (fgets(buff, sizeof(buff), stdin) == NULL) continue;

        sendto(sockfd, buff, strlen(buff), 0,
               (struct sockaddr*)&serv_addr, serv_len);

        if (strncmp(buff, "exit", 4) == 0) {
            printf("Client exiting...\n");
            break;
        }

        r = recvfrom(sockfd, buff, sizeof(buff) - 1, 0, NULL, NULL);
        if (r < 0) { perror("recvfrom"); break; }

        buff[r] = '\0';
        printf("Server: %s", buff);

        if (strncmp(buff, "exit", 4) == 0) {
            printf("Server ended chat. Client disconnecting...\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}
UDPChatServer
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int sockfd, r;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t cli_len;
    unsigned short serv_port = 25020;
    char serv_ip[] = "127.0.0.1";
    char buff[128];

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); exit(1); }

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("UDP CHAT SERVER STARTED\n");

    while (1) {
        printf("\nWaiting for client message...\n");

        cli_len = sizeof(cli_addr);
        r = recvfrom(sockfd, buff, sizeof(buff) - 1, 0,
                     (struct sockaddr*)&cli_addr, &cli_len);

        if (r < 0) { perror("recvfrom"); continue; }

        buff[r] = '\0';

        printf("Client (%s:%d): %s",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buff);

        if (strncmp(buff, "exit", 4) == 0) {
            printf("Client exited. Waiting for new client...\n");
            continue;
        }

        while (1) {
            printf("Server: ");
            if (fgets(buff, sizeof(buff), stdin) == NULL) continue;

            sendto(sockfd, buff, strlen(buff), 0,
                   (struct sockaddr*)&cli_addr, cli_len);

            if (strncmp(buff, "exit", 4) == 0) {
                printf("Client disconnected. Waiting for new client...\n");
                break;
            }

            cli_len = sizeof(cli_addr);
            r = recvfrom(sockfd, buff, sizeof(buff) - 1, 0,
                         (struct sockaddr*)&cli_addr, &cli_len);

            if (r < 0) { perror("recvfrom"); break; }

            buff[r] = '\0';

            printf("Client (%s:%d): %s",
                   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), buff);

            if (strncmp(buff, "exit", 4) == 0) {
                printf("Client exited. Waiting for new client...\n");
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}
UDPCommandClient
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int send_ack(int sockfd, struct sockaddr_in *serv, int seq){
    char abuf[64];
    socklen_t slen = sizeof(*serv);
    int n = snprintf(abuf, sizeof(abuf), "ACK %d", seq);
    if(n <= 0) return -1;
    return sendto(sockfd, abuf, n, 0, (struct sockaddr*)serv, slen);
}

static int recv_exact_text(int sockfd, struct sockaddr_in *serv, long size){
    long got = 0;
    int expected = 0;

    while(got < size){
        char pkt[1400];
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);

        int r = recvfrom(sockfd, pkt, sizeof(pkt), 0, (struct sockaddr*)&from, &flen);
        if(r <= 0) continue;

        if(from.sin_addr.s_addr != serv->sin_addr.s_addr || from.sin_port != serv->sin_port) continue;

        int seq = -1, len = -1;
        char *nl = memchr(pkt, '\n', r);
        if(!nl) continue;

        int hdrlen = (int)(nl - pkt + 1);
        if(sscanf(pkt, "DATA %d %d\n", &seq, &len) != 2) continue;
        if(hdrlen + len > r) continue;
        if(len < 0) continue;

        if(seq == expected){
            fwrite(pkt + hdrlen, 1, len, stdout);
            fflush(stdout);
            got += len;
            send_ack(sockfd, serv, seq);
            expected++;
        }else{
            send_ack(sockfd, serv, expected - 1);
        }
    }
    return 0;
}

int main(){
    int sockfd;
    struct sockaddr_in serv_addr;
    unsigned short serv_port = 25035;
    char serv_ip[] = "127.0.0.1";

    char buff[256];

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){ printf("CLIENT ERROR: Cannot create socket.\n"); exit(1); }

    printf("\nUDP COMMAND CLIENT\n");

    while(1){
        printf("\nEnter command: ");
        if(fgets(buff, sizeof(buff), stdin) == NULL) break;

        sendto(sockfd, buff, strlen(buff), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

        if(strncmp(buff, "exit", 4) == 0){
            printf("CLIENT: Disconnected.\n");
            break;
        }

        char header[128];
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);

        int r = recvfrom(sockfd, header, sizeof(header) - 1, 0, (struct sockaddr*)&from, &flen);
        if(r <= 0) continue;

        header[r] = '\0';

        if(from.sin_addr.s_addr != serv_addr.sin_addr.s_addr || from.sin_port != serv_addr.sin_port) continue;

        if(r == 4 && strncmp(header, "exit", 4) == 0){
            printf("\nSERVER: exit\nCLIENT: Disconnected.\n");
            break;
        }

        if(strncmp(header, "BUSY", 4) == 0){
            printf("SERVER: BUSY\n");
            continue;
        }

        long size = -1;
        if(sscanf(header, "TEXT %ld", &size) != 1 || size < 0){
            printf("SERVER: Invalid response\n");
            continue;
        }

        printf("\nSERVER OUTPUT:\n");
        recv_exact_text(sockfd, &serv_addr, size);
        printf("\n");
    }

    close(sockfd);
    return 0;
}
UDPCommandServer
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>

static int addr_eq(struct sockaddr_in *a, struct sockaddr_in *b){
    return a->sin_family == b->sin_family && a->sin_port == b->sin_port && a->sin_addr.s_addr == b->sin_addr.s_addr;
}

static int wait_ack(int sockfd, struct sockaddr_in *cli, int seq){
    fd_set rfds;
    struct timeval tv;
    char abuf[128];
    struct sockaddr_in from;
    socklen_t flen = sizeof(from);

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int rv = select(sockfd + 1, &rfds, NULL, NULL, &tv);
    if(rv <= 0) return 0;

    int r = recvfrom(sockfd, abuf, sizeof(abuf) - 1, 0, (struct sockaddr*)&from, &flen);
    if(r <= 0) return 0;
    abuf[r] = '\0';

    if(!addr_eq(&from, cli)) return 0;

    int aseq = -1;
    if(sscanf(abuf, "ACK %d", &aseq) == 1 && aseq == seq) return 1;
    return 0;
}

static int send_data_stopwait(int sockfd, struct sockaddr_in *cli, const char *data, long size){
    const int PAY = 900;
    char pkt[1200];
    long off = 0;
    int seq = 0;
    socklen_t clen = sizeof(*cli);

    while(off < size){
        int chunk = (int)((size - off) > PAY ? PAY : (size - off));
        int hdr = snprintf(pkt, sizeof(pkt), "DATA %d %d\n", seq, chunk);
        if(hdr <= 0 || hdr + chunk > (int)sizeof(pkt)) return -1;
        memcpy(pkt + hdr, data + off, chunk);

        int tries = 0;
        while(tries < 10){
            sendto(sockfd, pkt, hdr + chunk, 0, (struct sockaddr*)cli, clen);
            if(wait_ack(sockfd, cli, seq)) break;
            tries++;
        }
        if(tries >= 10) return -1;

        off += chunk;
        seq++;
    }
    return 0;
}

int main(){
    struct sockaddr_in serv_addr, cli_addr, active_cli;
    int sockfd;
    socklen_t cli_len;
    unsigned short serv_port = 25035;
    char serv_ip[] = "127.0.0.1";

    char buff[256];
    char response[65536];

    int has_client = 0;

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, &serv_addr.sin_addr);

    printf("\nUDP COMMAND SERVER\n");

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0){ printf("\nSERVER ERROR: Cannot create socket.\n"); exit(1); }

    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        printf("\nSERVER ERROR: Cannot bind.\n");
        close(sockfd);
        exit(1);
    }

    while(1){
        if(!has_client) printf("\nSERVER: Waiting for client...\n");

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(0, &readfds);

        int maxfd = sockfd > 0 ? sockfd : 0;
        if(select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) continue;

        if(FD_ISSET(0, &readfds)){
            if(fgets(buff, sizeof(buff), stdin) == NULL) continue;
            if(strncmp(buff, "exit", 4) == 0){
                if(has_client){
                    sendto(sockfd, "exit", 4, 0, (struct sockaddr*)&active_cli, sizeof(active_cli));
                    printf("SERVER: Client disconnected.\n");
                    has_client = 0;
                }
                continue;
            }
            printf("SERVER: No client. Type commands after a client connects.\n");
        }

        if(FD_ISSET(sockfd, &readfds)){
            cli_len = sizeof(cli_addr);
            int r = recvfrom(sockfd, buff, sizeof(buff) - 1, 0, (struct sockaddr*)&cli_addr, &cli_len);
            if(r <= 0) continue;
            buff[r] = '\0';

            if(!has_client){
                active_cli = cli_addr;
                has_client = 1;
                printf("\nConnected to %s:%d\n", inet_ntoa(active_cli.sin_addr), ntohs(active_cli.sin_port));
            }else{
                if(!addr_eq(&cli_addr, &active_cli)){
                    sendto(sockfd, "BUSY", 4, 0, (struct sockaddr*)&cli_addr, cli_len);
                    continue;
                }
            }

            if(strncmp(buff, "exit", 4) == 0){
                printf("Client exited.\n");
                has_client = 0;
                continue;
            }

            printf("CLIENT CMD: %s", buff);

            if(strncmp(buff, "cd", 2) == 0){
                char path[256];
                if(sscanf(buff, "cd %255[^\n]", path) != 1){
                    strcpy(response, "Invalid format");
                }else{
                    if(chdir(path) == 0) strcpy(response, "Directory changed");
                    else strcpy(response, "Failed to change directory");
                }

                char header[64];
                long sz = (long)strlen(response);
                snprintf(header, sizeof(header), "TEXT %ld", sz);
                sendto(sockfd, header, strlen(header), 0, (struct sockaddr*)&active_cli, sizeof(active_cli));
                send_data_stopwait(sockfd, &active_cli, response, sz);
                continue;
            }

            FILE *fp = popen(buff, "r");
            if(fp == NULL){
                strcpy(response, "Error executing command");
            }else{
                response[0] = '\0';
                char temp[512];
                size_t used = 0;
                while(fgets(temp, sizeof(temp), fp) != NULL){
                    size_t t = strlen(temp);
                    if(used + t + 1 >= sizeof(response)) break;
                    memcpy(response + used, temp, t);
                    used += t;
                    response[used] = '\0';
                }
                pclose(fp);
            }

            long outsz = (long)strlen(response);
            if(outsz == 0){
                strcpy(response, "\n");
                outsz = 1;
            }

            char header[64];
            snprintf(header, sizeof(header), "TEXT %ld", outsz);
            sendto(sockfd, header, strlen(header), 0, (struct sockaddr*)&active_cli, sizeof(active_cli));

            if(send_data_stopwait(sockfd, &active_cli, response, outsz) != 0){
                printf("SERVER ERROR: Transfer failed.\n");
                has_client = 0;
            }
        }
    }

    close(sockfd);
    return 0;
}

UDPEchoClient
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct sockaddr_in serv_addr;

int skfd, r, w, serv_addr_len;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char rbuff[128];
char sbuff[128] = "===good morning===";

int main()
{
    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, (&serv_addr.sin_addr));

    printf("\nUDP ECHO CLIENT.\n");

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nCLIENT ERROR: Cannot create socket.\n");
        exit(1);
    }

    serv_addr_len = sizeof(serv_addr);

    w = sendto(skfd, sbuff, 128, 0, (struct sockaddr *)&serv_addr, serv_addr_len);
    if (w < 0)
    {
        printf("\nCLIENT ERROR: Cannot send message to the server.\n");
        close(skfd);
        exit(1);
    }
    printf("\nCLIENT: Message sent to echo server.\n");

    r = recvfrom(skfd, rbuff, 128, 0, NULL, NULL);
    if (r < 0)
        printf("\nCLIENT ERROR: Cannot receive message from server.\n");
    else
    {
        rbuff[r] = '\0';
        printf("\nCLIENT: Message from echo server: %s\n", rbuff);
    }

    close(skfd);
    return 0;
}
UDPEchoServer
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct sockaddr_in serv_addr, cli_addr;

int skfd, r, w, cli_addr_len;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char buff[128];

int main()
{
    bzero(&serv_addr, sizeof(serv_addr));
    bzero(&cli_addr, sizeof(cli_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);
    inet_aton(serv_ip, (&serv_addr.sin_addr));

    printf("\nUDP ECHO SERVER.\n");

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nSERVER ERROR: Cannot create socket.\n");
        exit(1);
    }

    if ((bind(skfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("\nSERVER ERROR: Cannot bind.\n");
        close(skfd);
        exit(1);
    }

    cli_addr_len = sizeof(cli_addr);

    for (;;)
    {
        printf("\nSERVER: Waiting for messages... Press Ctrl + C to stop:\n");

        r = recvfrom(skfd, buff, 128, 0, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addr_len);
        if (r < 0)
        {
            printf("\nSERVER ERROR: Cannot receive message.\n");
            continue;
        }

        buff[r] = '\0';
        printf("\nSERVER: Received '%s' from %s:%d\n",
               buff, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

        w = sendto(skfd, buff, 128, 0, (struct sockaddr *)&cli_addr, cli_addr_len);
        if (w < 0)
            printf("\nSERVER ERROR: Cannot send echo.\n");
        else
            printf("\nSERVER: Echoed back to %s:%d\n",
                   inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
    }

    close(skfd);
    return 0;
}
UDPTimeServer
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>

struct sockaddr_in serv_addr, cli_addr, active_cli;

int skfd, r, w;
socklen_t cli_addr_len;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char buff[128];
char cmd[128];

int same_client(struct sockaddr_in *a, struct sockaddr_in *b)
{
    return a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port;
}

int main()
{
    int have_client = 0;

    bzero(&serv_addr, sizeof(serv_addr));
    bzero(&cli_addr, sizeof(cli_addr));
    bzero(&active_cli, sizeof(active_cli));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);

    if (inet_aton(serv_ip, &serv_addr.sin_addr) == 0)
    {
        printf("\nSERVER ERROR: Invalid IP.\n");
        exit(1);
    }

    printf("\nUDP TIME SERVER.\n");

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nSERVER ERROR: Cannot create socket.\n");
        exit(1);
    }

    if ((bind(skfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0)
    {
        printf("\nSERVER ERROR: Cannot bind.\n");
        close(skfd);
        exit(1);
    }

    cli_addr_len = sizeof(cli_addr);

    for (;;)
    {
        fd_set rfds;
        int maxfd;

        FD_ZERO(&rfds);
        FD_SET(skfd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);

        maxfd = (skfd > STDIN_FILENO) ? skfd : STDIN_FILENO;

        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) < 0)
        {
            printf("\nSERVER ERROR: select failed.\n");
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            if (fgets(cmd, sizeof(cmd), stdin) != NULL)
            {
                int n = strlen(cmd);
                if (n > 0 && cmd[n - 1] == '\n') cmd[n - 1] = '\0';

                if (strcmp(cmd, "exit") == 0)
                {
                    if (have_client)
                    {
                        sendto(skfd, "exit", 4, 0, (struct sockaddr *)&active_cli, sizeof(active_cli));
                        printf("\nSERVER: Sent exit to %s:%d\n",
                               inet_ntoa(active_cli.sin_addr), ntohs(active_cli.sin_port));
                        have_client = 0;
                    }
                    else
                    {
                        printf("\nSERVER: No active client.\n");
                    }
                }
            }
        }

        if (FD_ISSET(skfd, &rfds))
        {
            r = recvfrom(skfd, buff, 127, 0, (struct sockaddr *)&cli_addr, &cli_addr_len);
            if (r < 0)
            {
                printf("\nSERVER ERROR: Cannot receive message.\n");
                continue;
            }

            buff[r] = '\0';

            if (!have_client)
            {
                active_cli = cli_addr;
                have_client = 1;
                printf("\nSERVER: Active client is %s:%d\n",
                       inet_ntoa(active_cli.sin_addr), ntohs(active_cli.sin_port));
            }

            if (!same_client(&cli_addr, &active_cli))
            {
                continue;
            }

            if (strcmp(buff, "exit") == 0)
            {
                printf("\nSERVER: Client exit received from %s:%d\n",
                       inet_ntoa(active_cli.sin_addr), ntohs(active_cli.sin_port));
                have_client = 0;
                continue;
            }

            time_t t = time(NULL);
            char *ts = ctime(&t);
            if (!ts) ts = "time error\n";

            w = sendto(skfd, ts, strlen(ts), 0, (struct sockaddr *)&active_cli, sizeof(active_cli));
            if (w < 0)
                printf("\nSERVER ERROR: Cannot send time.\n");
            else
                printf("\nSERVER: Time sent to %s:%d\n",
                       inet_ntoa(active_cli.sin_addr), ntohs(active_cli.sin_port));
        }
    }

    close(skfd);
    return 0;
}
UDPTimeClient
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct sockaddr_in serv_addr;
socklen_t serv_len;

int skfd, r, w;

unsigned short serv_port = 25020;
char serv_ip[] = "127.0.0.1";

char sbuff[128] = "time";
char rbuff[128];

int main()
{
    bzero(&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);

    if (inet_aton(serv_ip, &serv_addr.sin_addr) == 0)
    {
        printf("\nCLIENT ERROR: Invalid server IP.\n");
        exit(1);
    }

    printf("\nUDP TIME CLIENT.\n");

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("\nCLIENT ERROR: Cannot create socket.\n");
        exit(1);
    }

    serv_len = sizeof(serv_addr);

    for (;;)
    {
        w = sendto(skfd, sbuff, strlen(sbuff), 0, (struct sockaddr *)&serv_addr, serv_len);
        if (w < 0)
        {
            printf("\nCLIENT ERROR: Cannot send request.\n");
            break;
        }

        r = recvfrom(skfd, rbuff, 127, 0, NULL, NULL);
        if (r < 0)
        {
            printf("\nCLIENT ERROR: Cannot receive.\n");
            break;
        }

        rbuff[r] = '\0';

        if (strcmp(rbuff, "exit") == 0)
        {
            printf("\nSERVER: exit received. Client disconnected.\n");
            break;
        }

        printf("\nSERVER TIME: %s", rbuff);
        sleep(2);
    }

    close(skfd);
    return 0;
}
