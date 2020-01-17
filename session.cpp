#include"session.h"


Session::Session(int client_socket, std::map<std::string, CacheRecord *> * cache){

	this->client_socket = client_socket;
	remote_socket = -1;


	this->cache = cache;
	state = SessionState::RECEIVE_CLIENT_REQUEST;
	cache_read_position = 0;
	cache_write_position = 0;
	response_answer_code = 0;

	sending_to_client = false;


	buffer_write_position = 0;
	buffer_read_position = 0;
	request_length = 0;
	

	url = nullptr;
	host = nullptr;
	protocol = nullptr;
}


int Session::connect_to_host(char* hostname, int port) {
    int s;
    struct hostent* server_host;
    struct sockaddr_in servaddr;

    //printf("get by name! %s\n", hostname);

    int i;
    for (i = 0; i < 5; i++) {
    	server_host = gethostbyname(hostname);
        if (NULL != server_host) {
            break;
	   }
    }

    if (server_host == NULL) {
        fprintf(stderr, "Wrong hostname. Can't resolve it\n");
        return -1;
    }
    //printf("Got!\n");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    memcpy(&servaddr.sin_addr.s_addr, server_host->h_addr_list[0], sizeof(struct in_addr));
    servaddr.sin_port = htons(port);

    //printf("Socket & connect!\n");

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (-1  == connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
        fprintf(stderr, "Can't connect to server\n");
        return -1;
    }

    return s;
}




int Session::replace_field(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return -1;
    str.replace(start_pos, from.length(), to);
    return 0;
}








// refactor
int Session::handle_client_request(int request_length){




	int res = HttpParser::parse_client_request(
	this->buffer, request_length, &this->url, &this->protocol, &this->host);


	if(res < 0){
		return -1;
	}




	// GET запрос http1.1

	//!
	char buf[256] = "";


	
	strcat(buf, this->host);
	strcat(buf, this->url);

	printf("Key is: ++%s++\n", buf);

	std::string string_key(buf);



	std::map<std::string, CacheRecord *>::iterator it = this->cache->find(string_key);






	sending_to_client = true;
	// if is cache finished
	if(this->cache->end() != it && it->second->is_full()){
		this->state = SessionState::USE_CACHE;
		buffer_write_position = 0;
		return 0;
	}




	int port = 80;
	int remote_socket = connect_to_host(this->host, port);



	if(remote_socket < 0){
		return -1;
	}




	this->state = SessionState::SEND_REQUEST;
	this->remote_socket = remote_socket;



/*
	// восстанваливаем запрос
	this->buffer[request_length] = '\r';


	std::string string_buffer(this->buffer);


	// dangerous if data has Proxy-Connection, keep-alive


	replace_field(string_buffer, "Proxy-Connection", "Connection");


	replace_field(string_buffer, "keep-alive", "close");

	const char * b_copy = string_buffer.c_str();

	strcpy(this->buffer, b_copy);

	this->request_length = strlen(this->buffer);

	//printf("Len1: %d\n", this->request_length);


	buffer_write_position = 0;
	this->request_length = this->request_length + strlen("\r\n\r\n");
	this->buffer[this->request_length] = '\0';*/










	//size_t REQUEST_LEN = 256;

	char * request = buf;

	sprintf(request,
	"GET %s HTTP/1.1\r\n"
	"Host: %s\r\n"
	"Accept: */*\r\n"
	"Content-Type: application/x-www-form-urlencoded\r\n"
	"Connection: close\r\n\r\n", this->url, this->host);

	this->request_length = strlen(request);


	bcopy(request, this->buffer, this->request_length);


	this->buffer[this->request_length] = '\0';



	    /*char* pattern = "GET %s %s\r\n";
    sprintf(changed_request, pattern, page, version);*/



	//printf("Len2: %d\n",this->request_length);

	//printf("%s\n", "-------------");
	//printf("+++%s+++\n", this->buffer);

	return 0;
}





// advanced: check for buffer overflow
int Session::read_client_request(){

	int read_count = read(this->client_socket, this->buffer + this->buffer_write_position, IO_BUFFER_SIZE);


	if(read_count < 0){
		perror("read");
		return -1;
	}

	if(read_count > 0){
		this->buffer_write_position+=read_count;
	}


	char * sub_position = strstr(this->buffer, "\r\n\r\n");

	if(NULL != sub_position){
		printf("%s\n", "Reached end of request");

		int request_length = sub_position - this->buffer;

		this->buffer[request_length] = '\0';
		printf("%s\n", this->buffer);


		return handle_client_request(request_length);
	}

	if(0 == read_count){
		return -1;
	}

	return 0;
}



// +
int Session::send_request(){
	int to_write = this->request_length - this->buffer_read_position;


	int write_count = write(this->remote_socket, this->buffer + this->buffer_read_position, to_write);

	this->buffer_read_position+=write_count;

	if(write_count < 0){
		perror("write");
		return -1;
	}

	if(this->buffer_read_position == this->request_length){
		buffer_read_position = 0;
		buffer_write_position = 0;
		this->state = SessionState::MANAGE_RESPONSE;
	}
	printf("Wrote to server: %d\n", write_count);	
	return 0;
}













int Session::manage_response(int poll_read_ready, int poll_write_ready){



	char buf[256] = "";
	strcat(buf, this->host);
	strcat(buf, this->url);

	std::string string_key(buf);



	std::map<std::string, CacheRecord *>::iterator cache_it = this->cache->find(string_key);

	
	if(this->cache->end() == cache_it){


		CacheRecord * record = new CacheRecord();

		printf("%s\n", "create record");

		this->cache->insert(std::pair<std::string, CacheRecord *>(string_key, record)); 

		cache_it = this->cache->find(string_key);
	}




	CacheRecord	* cache_record = cache_it->second;

	if(poll_read_ready){

		int read_count = read(this->remote_socket, this->buffer, IO_BUFFER_SIZE);
		

		//write(1, this->buffer, read_count);



		if(0 == read_count){
		
			this->remote_socket = -1;
			this->sending_to_client	= false;
			char  r_end[5] = "\r\n\r\n";


			int cache_size = cache_record->get_size();

			if(cache_size < 4){
				// send bad to client
			}


			char * found = strstr(r_end, cache_record->get_data() - strlen(r_end));
			if(nullptr == found){
				// send bad to client
			}


			
			// end of response

			printf("%s\n", "Response ready");

			close(remote_socket);
			this->remote_socket = -1;

		

		}else if(read_count > 0){
			// debug
			/*char keep_ = this->buffer[read_count];
			this->buffer[read_count] = '\0';
			printf("%s\n", this->buffer);
			this->buffer[read_count] = keep_;*/
		}else{

			// ошибка чтения из сокета сервера
			perror("read");
			return -1;
		}


		





		int cache_size = cache_record->get_size();

		int res = 0;
		



		if(this->cache_write_position + read_count < cache_size){
			// не нужно писать в кэш, ждем пока данные придут с нужного смещения
			this->cache_write_position+=read_count;
		}else{
			if(0 == read_count){
				cache_it->second->finish();
				printf("%s\n", "Cache finish");
			}else{
				int to_add = this->cache_write_position + read_count - cache_size;


				
				
				// добавляем данные в кэш
				this->cache_write_position+=read_count;



				//write(1, this->buffer + (read_count - to_add), to_add);


				res = cache_it->second->add_data(this->buffer + (read_count - to_add), to_add);
				

				if(res < 0){
					// just exit from function --> delete current session
					return -1;
				}
			}


		}
	}



	if(poll_write_ready){

		char * data = cache_it->second->get_data();
		int size = cache_it->second->get_size();

		int to_write = size - this->cache_read_position;
		assert(to_write >= 0);


		if(0 == to_write && cache_it->second->is_full()){
			close(this->client_socket);
			this->client_socket = -1;
			printf("%s\n", "close connection");

			//delete session: ok
			return -1;
		}

		if(0 != to_write){

			if(to_write > IO_BUFFER_SIZE){
				to_write = IO_BUFFER_SIZE;
			}

			int write_count = write(this->client_socket, data + this->cache_read_position, to_write);

			write(1, data + this->cache_read_position, to_write);


			if(write_count < 0){

				// возможно отсоединился клиент

				close(this->client_socket);
				this->client_socket	= -1;
				perror("write");
				// do not delete session, докачиваем данные

				return 0;
			}

			this->cache_read_position+=write_count;
			

		}
	}

	return 0;
}



/*void Session::set_remote_socket(int socket){
	remote_socket = socket;
}*/


/*

	SessionState state;
	int response_answer_code;
	char * url;

	int client_socket;
	int remote_socket;
	char buffer[SESSION_BUFFER_SIZE];
	int cache_read_position;
	int cache_write_position;


*/