

#include"proxy.h"


#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include<string.h>


#define MAX_CONNECTIONS_COUNT 1024


int bind_to_port(int port) {
    struct sockaddr_in servaddr;                                    

    memset(&servaddr, 0, sizeof(servaddr));                         
    servaddr.sin_family = AF_INET;                                  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);                   
    servaddr.sin_port = htons((uint16_t)port);               

    int listener = socket(AF_INET, SOCK_STREAM, 0);                        

    if(listener < 0){
        perror("socket");
        return -1;
    }
    int res = bind(listener, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (res < 0) {   
        fprintf(stderr, "Can't bind to port %d!\n", port);
        return -1;
    }
    return listener;
}


int main(int argc, char** argv){


    if (2 != argc) {
        fprintf(stderr, "Usage: %s <bind port>\n", argv[0]);
        return EXIT_FAILURE;
    }


    signal(SIGPIPE, SIG_IGN);



    char * next = NULL;
    int radix = 10;
    int port_to_listen = strtol(argv[1], &next, radix);
        
    if((next == argv[1]) || (*next != '\0')){
        printf("Invalid argument: bind port.\n");
        return EXIT_FAILURE;
    }


    int listener = bind_to_port(port_to_listen);

    if(listener < 0){
        return EXIT_FAILURE;
    }


    if(fcntl(listener, F_SETFL, O_NONBLOCK) == -1){
        perror("fcntl");
        return -1;
    }

    int res = listen(listener, MAX_CONNECTIONS_COUNT);

    if(res < 0){
    	fprintf(stderr, "Port listen failed...\n");
    	return EXIT_FAILURE;
    }

    Proxy proxy(listener);

    proxy.start();



    return EXIT_SUCCESS;
}
