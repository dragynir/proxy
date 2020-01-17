#pragma once



#include"parser.h"
#include"cache.h"



#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<strings.h>
#include<string.h>


#include<errno.h>
#include<assert.h>
#include<map>




#define SESSION_BUFFER_SIZE 1024
#define IO_BUFFER_SIZE 1024



enum SessionState {
	RECEIVE_CLIENT_REQUEST,
	SEND_REQUEST,
	
	MANAGE_RESPONSE,
	USE_CACHE
};


class Session{
	// destory url
public:

	int client_socket;
	int remote_socket;

	Session(int client_socket, std::map<std::string, CacheRecord *> * cache);

	//void set_remote_socket(int socket);

	int getState(){return state;}
	void setState(SessionState state){this->state = state;}
	void setUrl(char * url){this->url = url;};
	void setProtocol(char * protocol){this->protocol = protocol;}

	char * getBuffer(){return buffer;}



	int read_client_request();
	int send_request();
	int manage_response(int poll_read_ready, int poll_write_ready);

	bool is_sending(){return sending_to_client;};
	//void set_is_sending(bool sending){sending_to_client = sending;};

	~Session();


private:

	int handle_client_request(int request_length);

	int connect_to_host(char* hostname, int port);

	int replace_field(std::string& str, const std::string& from, const std::string& to);


	SessionState state;
	int response_answer_code;
	int cache_read_position;
	int cache_write_position;
	std::map<std::string, CacheRecord *> * cache;
	char * url;
	char * protocol;
	char * host;

	bool sending_to_client;


	char buffer[SESSION_BUFFER_SIZE];
	int buffer_write_position;
	int buffer_read_position;
	int request_length;




};