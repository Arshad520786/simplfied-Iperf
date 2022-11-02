#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>       /*  socket definitions */
#include <sys/types.h>        /*  socket types       */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>

double timediff(struct timespec *beg_t, struct timespec *end_t){
    if((beg_t->tv_nsec - end_t->tv_nsec) > 0)
        return  (end_t->tv_sec - beg_t->tv_sec - 1) + (1000000000.0 - beg_t->tv_nsec + end_t->tv_nsec)/1000000000.0;
    else 
        return  (end_t->tv_sec - beg_t->tv_sec) + (end_t->tv_nsec - beg_t->tv_nsec)/1000000000.0;
}

int main(int argc, char* argv[]){
    char *hostname;
    int serverport;
    int listenport;
    int transmit_time;
    int receivedbytes;              // store size of received data
    int sentbytes;              // store size of sent data
    int sockfd, new_fd;            // listen on sock_fd, new connection on new_fd
    struct sockaddr_in server_addr;  //server address
    struct sockaddr_in client_addr;  //server address
    struct timespec end_t, beg_t, current_t;

    // Parse option and arguments.
    enum {
    CLIENT_MODE = 1, 
    SERVER_MODE = 2, 
    INVALID_MODE = -1
    } mode = INVALID_MODE;


    if(strcmp(argv[1], "-c") == 0){
        mode = CLIENT_MODE;
    }
    else if (strcmp(argv[1], "-s") == 0){
        mode = SERVER_MODE;
    }
    else{
        printf("Error: missing or additional arguments\n");
        exit(1);
    }

    
    
    //Client Code section-----------------------------------------------------
    if(mode == CLIENT_MODE){
        // check argument count
        if(argc != 8){
            printf("Error: missing or additional arguments\n");
            exit(1);
        }
    
        //write hostname and port number and time
        if(strcmp(argv[2], "-h") == 0){
            hostname = argv[3];
        }
        else{
            printf("Error: missing or additional arguments\n");
            exit(1);
        }
        if(strcmp(argv[4], "-p") == 0){
            serverport = atoi(argv[5]);
            if(serverport < 1024 || serverport > 65535){
                printf("Error: port number must be in the range 1024 to 65535\n");
                exit(1);
            }
        }
        else{
            printf("Error: missing or additional arguments\n");
            exit(1);
        }
        if(strcmp(argv[6], "-t") == 0){
            transmit_time = atoi(argv[7]);
        }
        else{
            printf("Error: missing or additional arguments\n");
            exit(1);
        }

        // inital sendbuffer and timer and byte count
        char SendBuffer[1000];
        long totalbytes = 0;
        memset(&SendBuffer, '0', 1000); // fill sendbuffer with 0 to make a 1000 byte chunck
        // set connection
        server_addr.sin_family = AF_INET; // host byte order
        server_addr.sin_port = htons(serverport); // short, network byte order
        server_addr.sin_addr.s_addr = inet_addr(hostname);
        memset(&(server_addr.sin_zero), 0, 8); // zero the rest of the struct


        if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
                perror("socket");
                exit(1);
            }
        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
            perror("connect");
            exit(1);
        }

        //start clcok
        clock_gettime(CLOCK_MONOTONIC, &beg_t);
        //Repeatedly sending 1000 bytes chunck.
        while(clock_gettime(CLOCK_MONOTONIC, &current_t) == 0 && timediff(&beg_t,&current_t) < (double)transmit_time){
            if ((sentbytes = send(sockfd, SendBuffer, sizeof(SendBuffer), 0)) == -1) {
                perror("send");
                exit(1);
            }
            else{
                totalbytes += sentbytes;
                //printf("totalbyes : %d\n", totalbytes);
            }
        }
        // End clock
        clock_gettime(CLOCK_MONOTONIC, &end_t);
        // Send a 0 byte chunck to notify the server that transmission is over.
        if ((send(sockfd, "\0", 0, 0)) == -1) {
            perror("send");
            exit(1);
        }
        close(sockfd);
        double totaltime = timediff(&beg_t, &end_t);
        printf("Time window : %f secs\n", totaltime);
        printf("Sent : %f KB Rate : %f Mbps\n", (double)totalbytes/1000.0, (double)totalbytes*8/1000000.0/totaltime);
    }
    
    //Server Code section-----------------------------------------------------
    else if(mode == SERVER_MODE){
        // check argument count
        if(argc != 4){
            printf("Error: missing or additional arguments\n");
            exit(1);
        }
        //write port number
        if(strcmp(argv[2], "-p") == 0){
            listenport = atoi(argv[3]);
            if(listenport < 1024 || listenport > 65535){
                printf("Error: port number must be in the range 1024 to 65535\n");
                exit(1);
            }
            printf("hostname : %d\n", listenport);
        }
        else{
            printf("Error: missing or additional arguments\n");
            exit(1);
        }

        //bind socket and listen
        char ReceiveBuffer[1000];

        if ((sockfd = socket(AF_INET, SOCK_STREAM, 6)) == -1) {
            perror("socket");
            exit(1);
        }
        server_addr.sin_family = AF_INET; // host byte order
        server_addr.sin_port = htons(listenport); // short, network byte order
        server_addr.sin_addr.s_addr = INADDR_ANY;
        memset(&(server_addr.sin_zero), 0, 8); // zero the rest of the struct
        
        if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1){
            perror("bind");
            exit(1);
        }
        if (listen(sockfd, 5) == -1){
            perror("listen");
            exit(1);
        }
        // main accept() loop
        clock_t time = 0;
        clock_t startofclock;
        long totalbytes = 0;
        int sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size)) == -1){
            perror("accept");
        }
        // start of clock
        clock_gettime(CLOCK_MONOTONIC, &beg_t);
        //receive() loop
        while (1){ 
            if ((receivedbytes = recv(new_fd, ReceiveBuffer, 1000, 0)) == -1){
                perror("receive");
                exit(1);
            }
            else{
                totalbytes += receivedbytes;
                //printf("totalbyes : %d\n", totalbytes);

            }    
            if(receivedbytes == 0){
                // end of clock
                clock_gettime(CLOCK_MONOTONIC, &end_t);
                break;
            }
        }
        double totaltime = timediff(&beg_t, &end_t);    
        close(new_fd);
        printf("Time window : %f secs\n", totaltime);
        printf("Sent : %f KB Rate : %f Mbps\n", (double)totalbytes/1000.0, (double)totalbytes*8/1000000.0/totaltime);
    }
    return 0;
}