#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>


#define PORT 3002
#define PORTLENGTH 6
#define BUFLEN 100
#define ADDYLENGTH 16
#define MAXCONTENT 100
#define CONTENTLENGTH 20
#define PEERLENGTH 20

//PDU Structure
typedef struct PDU1{
    char type;
    char data[BUFLEN];
} pdu;


void showMenu(void);
void initPDU(struct PDU1* sendData, struct PDU1* receivedData, int * registrationDone, char peerName[], char address[], char contentList[][CONTENTLENGTH], char port[], int * numContent);
void clearStructs(struct PDU1 * sendData, struct PDU1 * receivedData);
void registration(struct PDU1* sendData, struct PDU1* receivedData, int * registrationDone, int * socketDone, char peerName[], char address[], char contentList[][CONTENTLENGTH], char port[], int * numContent, ssize_t * recvLength, struct sockaddr_in* server_address, socklen_t * addrSize, int networkSock);
void deregistration(struct PDU1* sendData, struct PDU1* receivedData, int * registrationDone, char peerName[], char address[], char contentList[][CONTENTLENGTH], char port[], int * numContent, ssize_t * recvLength, struct sockaddr_in* server_address, socklen_t * addrSize, int networkSock);
void hostContent (char address[], char port[]);
int download(char filename[], char downlaodPort[], char contentAddy[]);



int main(int argc, char **argv){

    int registrationDone = 0, socketDone = 0;
    char peerName[PEERLENGTH];
    char contentList [MAXCONTENT][CONTENTLENGTH];
    char address [ADDYLENGTH];
    char port [PORTLENGTH];
    int numContent;
    pdu sendData, receivedData;
    char choice;
    int check;
    pid_t pid; //process for the hosting function
    initPDU(&sendData, &receivedData, &registrationDone, peerName, address, contentList, port, &numContent);
    socklen_t addrSize;
    char host [100];
    strcpy(host, argv[1]);
    struct hostent *server;
    //socket address
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server = gethostbyname (host);
    bcopy((char *)server -> h_addr, (char *) &server_address.sin_addr.s_addr, server -> h_length);
    addrSize = sizeof(server_address);
    ssize_t recvLength;
    //the udp socket
    int networkSock;
    networkSock = socket(PF_INET, SOCK_DGRAM, 0);
    if (networkSock < 0){
        printf("error creating socket");
        exit(1);
    }
    printf("Peer network started\n");
    showMenu();
    //main program loop
    while (1)
    {
        int i;
        //menu choice
        choice = getchar();
        //registration request
        if (choice == 'R'){
            registration(&sendData, &receivedData, &registrationDone, &socketDone, peerName, address, contentList, port, &numContent, &recvLength, (struct sockaddr_in *) &server_address, &addrSize, networkSock);
            if (socketDone == 1){
                socketDone = 0;
                //starting the process to host the content in the background
                //establishes a content server
                pid = fork();
                if (pid == 0){
                    hostContent(address, port);
                }
            }
            showMenu();
        }
        //download Request
        else if (choice == 'D'){
            int downloadStatus = 0;
            clearStructs(&sendData, &receivedData);
            check = 0;
            char filename[CONTENTLENGTH];
            int nameLength;
            printf("Enter the filename to downlaod:\n");
            scanf("%s", filename);
            nameLength = strlen(filename)+1;
            
            //send 'S' PDU to server to find  content
            sendData.type = 'S';
            strcpy(sendData.data, filename);
            sendto(networkSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &server_address, addrSize);
            recvLength = recvfrom(networkSock, (struct PDU1*) &receivedData, sizeof(receivedData), 0, (struct sockaddr*) &server_address, &addrSize);
            //respond with 'D' indicates found
            if (receivedData.type == 'D'){
                //getting the address of the content to download from
                char contentAddy [ADDYLENGTH];
                char downlaodPort [PORTLENGTH];
                int i = 0;
                int j = 0;
                for (i = 0 ; i < ADDYLENGTH ; i++){
                    if (receivedData.data[i] == '\0'){
                        break;
                    }
                    contentAddy[i] = receivedData.data[i];
                }
                contentAddy[i] = '\0';
                i++;
                for (i = i ; i < ADDYLENGTH + PORTLENGTH ; i++){
                    if (receivedData.data[i] == '\0'){
                        break;
                    }
                    downlaodPort[j] = receivedData.data[i];
                    j++;
                }
                downlaodPort[j] = '\0';
                downloadStatus = download(filename, downlaodPort, contentAddy);
                //user now becomes a content server for the file they downloaded
                if (downloadStatus == 1){
                        clearStructs(&sendData, &receivedData);
                        sendData.type = 'T';
                        sprintf(sendData.data, "Successful Download");
                        sendto(networkSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &server_address, addrSize);
                        strcpy(contentList[numContent], filename);
                        numContent++;
                    
                }
            }
            showMenu();
        }
        
        //hanles request to list online content
        else if (choice == 'O'){
            int i;
            char names [BUFLEN];
            clearStructs(&sendData, &receivedData);
            sendData.type = 'O'; //sending the 'O' type PDU
            sendto(networkSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (const struct sockaddr*) &server_address, addrSize);
            printf("Online Content:\n");
            //loop receiving PDUs until final
            while (receivedData.type != 'F')
            {
                recvLength = recvfrom(networkSock, (struct PDU1*) &receivedData, sizeof(receivedData), 0, (struct sockaddr*) &server_address, &addrSize);
                if (receivedData.type == 'F' && receivedData.data[0] == '\0')
                {
                    printf("no content online\n");
                    break;
                }
                memset(names, '\0', sizeof(names));
                //displaying the filenames
                for (i = 0 ; i < BUFLEN ; i++){
                    if (receivedData.data [i] == '\0'){
                        names [i] = '\n';
                        if (i == (BUFLEN - 1)){
                            printf("%s", names);
                            break;
                        }
                        if (receivedData.data[i + 1] == '\0'){
                            printf("%s", names);
                            break;
                        }
                    }
                    else{
                        names [i] = receivedData.data [i];
                    }
                }
            }
            showMenu();
        }
        //deregistration request
        else if (choice == 'T'){
            deregistration(&sendData, &receivedData, &registrationDone, peerName, address, contentList, port, &numContent, &recvLength, (struct sockaddr_in *) &server_address, &addrSize, networkSock);
            showMenu();
        }
        //quit request
        else if (choice == 'Q'){
            int status = 0;
            int i;
            clearStructs(&sendData, &receivedData);
            //Send a series of 'T' PDUs to deregister all content
            for (i = 0 ; i < numContent ; i++){
                clearStructs(&sendData, &receivedData);
                strcpy(sendData.data, contentList[i]);
                sendData.type = 'T';
                sendto(networkSock, (struct PDU1 *) &sendData, sizeof(sendData), 0, (struct sockaddr*) &server_address, addrSize);
                recvLength = recvfrom(networkSock, (struct PDU1*) &receivedData, sizeof(receivedData), 0, (struct sockaddr*) &server_address, &addrSize);
                //delete local content list
                if (receivedData.type == 'T'){
                    memset(contentList[i], '\0', sizeof(contentList[i]));
                    printf("%s\n", receivedData.data);
                }
                else{
                    printf("%s\n", receivedData.data);
                    status = 1;
                }
            }
            //If no error occured stop the hosting process because peer is no longer a content server
            if (status == 0){
                clearStructs(&sendData, &receivedData);
                recvLength = recvfrom(networkSock, (struct PDU1*) &receivedData, sizeof(receivedData), 0, (struct sockaddr*) &server_address, &addrSize);
                if (receivedData.type == 'T'){
                    kill (pid, SIGKILL);
                }
            }
            return 0; //quit the program
        }
    }
    
    return 0;
}

//displaying all the menu options
void showMenu()
{
    printf("Enter 'R' to register content\n");
    printf("Enter 'D' to download content\n");
    printf("Enter 'O' to see all avaialble content\n");
    printf("Enter 'T' to de-register content\n");
    printf("Enter 'Q' to quit\n");
}

//initialize PDU
void initPDU(struct PDU1* sendData, struct PDU1 * receivedData, int * registrationDone, char peerName[], char address[], char contentList[][CONTENTLENGTH], char port[], int * numContent){
    memset(sendData -> data, '\0', sizeof(sendData -> data));
    memset(receivedData -> data, '\0', sizeof(receivedData -> data));
    *registrationDone = 0;
    memset(peerName, '\0', sizeof(*peerName));
    memset(address, '\0', sizeof(*address));
    memset(port, '\0', sizeof(*port));
    int i;
    for (i = 0 ; i < MAXCONTENT ; i++)
    {
        memset(contentList[i], '\0', sizeof(*contentList[i]));
    }
    *numContent = 0;
}

//clear PDU
void clearStructs(struct PDU1 * sendData, struct PDU1 * receivedData){
    memset(sendData -> data, '\0', sizeof(sendData -> data));
    memset(receivedData -> data, '\0', sizeof(receivedData -> data));
}



//downlaoding content from a content server
int download(char filename[], char downlaodPort[], char contentAddy[]){
    pdu sendData, receivedData;
    int fileCheck = 0;
    int downloadConnection;
    int contentClient;
    int contentFile;
    //TCP socket for downloading
    struct hostent *contentServer;
    struct sockaddr_in contentServerAddy;
    memset(&contentServerAddy, 0, sizeof(contentServerAddy));
    contentServerAddy.sin_family = AF_INET;
    contentServerAddy.sin_port = htons(atoi(downlaodPort));
    contentServer = gethostbyname(contentAddy);
    bcopy((char *)contentServer -> h_addr, (char *) &contentServerAddy.sin_addr.s_addr, contentServer -> h_length);
    contentClient = socket(AF_INET, SOCK_STREAM, 0);
    if (contentClient < 0){
        printf("Error creating socket");
        exit(1);
    }
    downloadConnection = connect(contentClient, (struct sockaddr*) &contentServerAddy, sizeof(contentServerAddy));
    if (downloadConnection == -1){
        printf("error connecting to socket\n");
    }
    memset(sendData.data, '\0', sizeof(sendData.data));
    sendData.type = 'D';
    strcpy(sendData.data, filename);
    send(contentClient, (struct PDU1 *) &sendData, sizeof(sendData), 0);
    ssize_t numBytes;
    memset(receivedData.data, '\0', sizeof(receivedData.data));
    //creating the file to write to
    contentFile = open(filename, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
    while((numBytes = recv(contentClient, (struct PDU1 *) &receivedData, sizeof(receivedData), 0)) > 0){
        if (receivedData.type == 'C' || receivedData.type == 'F'){
            write(contentFile, receivedData.data, sizeof(receivedData.data));
        }
        else{
            printf("%s\n", receivedData.data);
            fileCheck = 1;
            break;
        }
    }
    //closing file and socket
    close(contentFile);
    close(contentClient);
    // the file check var checks for an error, so if no error=0, file received succesfully.
    if (fileCheck == 0){
        printf("\nfile received\n");
        return 1;
    }
    return 0;
}


//Registration function
void registration(struct PDU1* sendData, struct PDU1* receivedData, int * registrationDone, int * socketDone, char peerName[], char address[], char contentList[][CONTENTLENGTH], char port[], int * numContent, ssize_t * recvLength, struct sockaddr_in* server_address, socklen_t * addrSize, int networkSock){
    int peerSize, contentSize, portSize;
    char filename [CONTENTLENGTH];
    int i;
    clearStructs(sendData, receivedData);
    //If first time registration
    if (*registrationDone == 0){
        printf("Enter peer name:\n");
        scanf("%s", peerName);
        printf("Enter content filename:\n");
        scanf("%s", filename);
        strcpy(contentList[*numContent], filename);
        printf("Enter port number to host on:\n");
        scanf("%s", port);
        peerSize = strlen(peerName);
        contentSize = strlen(contentList[*numContent]);
        portSize = strlen(port);
        peerSize++;
        contentSize++;
        portSize++;
        memcpy(sendData -> data, peerName, peerSize);
        memcpy(sendData -> data + peerSize, contentList[*numContent], contentSize);
        memcpy(sendData -> data + peerSize + contentSize, port, portSize);
        *numContent = *numContent + 1;
        sendData -> type = 'R';
        sendto(networkSock, (struct PDU1 *) sendData, sizeof(*sendData), 0, (struct sockaddr*) server_address, *addrSize);
        *recvLength = recvfrom(networkSock, (struct PDU1*) receivedData, sizeof(*receivedData), 0, (struct sockaddr*) server_address, addrSize);
        printf("%s\n", receivedData -> data);
        //If error occured send 'E' type PDU
        if (receivedData -> type != 'E'){
            *registrationDone = 1;
            *socketDone = 1;
        }
        //no error occured setup the PDU with data
        else{
            printf("%s\n", receivedData -> data);
            memset(peerName, '\0', sizeof(*peerName));
            memset(port, '\0', sizeof(*port));
            *numContent = *numContent - 1;
            memset(contentList[*numContent], '\0', sizeof(*contentList[*numContent]));
        }
    }
    
}
//handles the hosting of files, process running in the background makes peers content servers.
void hostContent (char address[], char port[]){
    pdu sendInfo, resp;
    int fileSocket;
    //TCP socket for hosting
    struct sockaddr_in hostAddy;
    struct sockaddr_in clientAddy;
    char clientAddyString [ADDYLENGTH];
    socklen_t clientSize;
    memset(&hostAddy, 0, sizeof(hostAddy));
    memset(&clientAddy, 0, sizeof(clientAddy));
    hostAddy.sin_family = AF_INET;
    hostAddy.sin_port = htons(atoi(port));
    hostAddy.sin_addr.s_addr = htonl(INADDR_ANY);
    clientSize = sizeof(clientAddy);
    fileSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fileSocket < 0){
        printf("error making socket");
        exit(1);
    }
    int bindCheck;
    bindCheck = bind(fileSocket, (struct sockaddr*) &hostAddy, sizeof(hostAddy));
    if (bindCheck == -1){
        printf("Socket bind error\n");
        exit(1);
    }
    listen(fileSocket, 5);
    int clientSocket;
    while (1){
        clientSocket = accept(fileSocket, (struct sockaddr*)&clientAddy, &clientSize);
        memset(resp.data, '\0', sizeof(resp.data));
        recv(clientSocket, &resp, sizeof(resp), 0);
        inet_ntop(AF_INET, &clientAddy.sin_addr, (char *)clientAddyString, sizeof(clientAddyString));
        memset(sendInfo.data, '\0', sizeof(sendInfo.data));
        int file, numBytesRead;
        file = open(resp.data, O_RDONLY);
        //if the file exists send 100 bytes at time
        //last 100 bytes are sent with an 'F' type PDU
        if (file > 0){
            while ((numBytesRead = read(file, sendInfo.data, sizeof(sendInfo.data))) > 0){
                if (numBytesRead < 100){
                    sendInfo.type = 'F';
                }
                else{
                    sendInfo.type = 'C';
                }
                send (clientSocket, (struct PDU1 *)&sendInfo, sizeof(sendInfo), 0);
            }
            close (file);
        }
        else{
            //error send 'E' PDU
            sendInfo.type = 'E';
            strcpy(sendInfo.data, "error file not found\0");
            send (clientSocket, (struct PDU1 *)&sendInfo, sizeof(sendInfo), 0);
            close (file);
            printf("error file not found \n");
        }
        close(clientSocket);
        showMenu();
    }
}

void deregistration(struct PDU1* sendData, struct PDU1* receivedData, int * registrationDone, char peerName[], char address[], char contentList[][CONTENTLENGTH], char port[], int * numContent, ssize_t * recvLength, struct sockaddr_in* server_address, socklen_t * addrSize, int networkSock)
{
    if (registrationDone == 0){
        printf("error, not registered\n");
        return;
    }
    char filename[CONTENTLENGTH];
    int found = 0;
    int i;
    printf("Enter the name of the content you would like to de-register:\n");
    scanf("%s", filename);
    for (i = 0 ; i < MAXCONTENT ; i++){
        if (strcmp(filename, contentList[i]) == 0){
            found = 1;
            break;
        }
    }
    if (found == 0){
        printf("error, not the owner of the content\n");
        return;
    }
    else{
        //Send 'T' PDU to the server with the filename
        clearStructs(sendData, receivedData);
        sendData -> type = 'T';
        strcpy(sendData -> data, contentList[i]);
        sendto(networkSock, (struct PDU1 *) sendData, sizeof(*sendData), 0, (struct sockaddr*) server_address, *addrSize);
        *recvLength = recvfrom(networkSock, (struct PDU1*) receivedData, sizeof(*receivedData), 0, (struct sockaddr*) server_address, addrSize);
        if (receivedData -> type == 'A'){
            if (i == (MAXCONTENT - 1)){
                memset(contentList[i], '\0', sizeof(*contentList[i]));
                *numContent = *numContent - 1;
            }
            else{
                int j = i;
                for (j = i ; j < *numContent ; j++){
                    memset(contentList[j], '\0', sizeof(*contentList[j]));
                    strcpy(contentList[j], contentList[j + 1]);
                }
                *numContent = *numContent - 1;
                memset(contentList[*numContent], '\0', sizeof(*contentList[*numContent]));
            }
            printf("%s\n", receivedData -> data);
            //reset data
            if (*numContent == 0){
                initPDU(sendData, receivedData, registrationDone, peerName, address, contentList, port, numContent);
            }
        }
        else{
            printf("error\n");
        }
    }
}



