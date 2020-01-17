

#include"proxy.h"


#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include<string.h>


#define MAX_CONNECTIONS_COUNT 100


int bind_to_port(int port) {
    struct sockaddr_in servaddr;                                    

    memset(&servaddr, 0, sizeof(servaddr));                         
    servaddr.sin_family = AF_INET;                                  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);                   
    servaddr.sin_port = htons((uint16_t)port);               

    int listener = socket(AF_INET, SOCK_STREAM, 0);                        
    int res = bind(listener, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (res < 0) {   
        fprintf(stderr, "Can't bind to port %d!\n", port);
        return -1;
    }
    return listener;
}


int main(int argc, char** argv){


    if (2 > argc) {
        fprintf(stderr, "Usage: %s <bind port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    //signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);

    int port_to_listen = atoi(argv[1]);

    int listener = bind_to_port(port_to_listen);


    int res = listen(listener, MAX_CONNECTIONS_COUNT);

    if(res < 0){
    	fprintf(stderr, "Port listen failed...\n");
    	return EXIT_FAILURE;
    }

    Proxy proxy(listener);

    proxy.start();



    return EXIT_SUCCESS;
}