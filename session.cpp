#include"session.h"


Session::Session(int client_socket, std::map<std::string, CacheRecord *> * cache){

	this->client_socket = client_socket;
	this->remote_socket = -1;
	this->response_code = -1;


	this->cache = cache;
	this->cache_record = NULL;

	//state = SessionState::RECEIVE_CLIENT_REQUEST;
	state = RECEIVE_CLIENT_REQUEST;
	cache_read_position = 0;
	cache_write_position = 0;
	response_answer_code = 0;

	sending_to_client = false;


	buffer_write_position = 0;
	buffer_read_position = 0;
	request_length = 0;
	

	url = NULL;
	host = NULL;
	/*protocol = NULL;
	resource = NULL;*/
}



Session::~Session(){
	delete this->url;
	delete this->host;
	if(this->cache_record->is_local()){
		delete this->cache_record;
	}
}




int Session::connect_to_host(char* hostname, int port) {
    int s;
    hostent* server_host;
    sockaddr_in servaddr;


    //printf("%s\n", "Session::connect_to_host");
   

    /*std::string string_host(hostname);

    std::cout << string_host;

    printf("%s\n", "Session::connect_to_host2");
	std::map<std::string,  hostent *>::iterator it = this->dns->find(string_host);
	printf("%s\n", "Session::connect_to_host3");



	if(this->dns->end() != it){
		printf("%s\n", "use local dns");
		server_host = it->second;
	}else{
		printf("%s\n", "gethostbyname");
		server_host = gethostbyname(hostname);

		if(NULL == server_host){
			perror("gethostbyname");
			return -1;
		}
		this->dns->insert(std::pair<std::string,  hostent *>(string_host, server_host));
	}
	printf("%s\n", "after dns cache");*/




	server_host = gethostbyname(hostname);

	if(NULL == server_host){
		//perror("gethostbyname");

		herror("gethostbyname");
		return -1;
	}

    /*for (int i = 0; i < 5; i++) {
    	server_host = gethostbyname(hostname);
        if (NULL != server_host) {
            break;
	   }
    }*/

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

    if(fcntl(s, F_SETFL, O_NONBLOCK) == -1){
    	perror("fcntl");
    	return -1;
    }
 
    
    if (-1  == connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
        if(EINPROGRESS != errno){
        	perror("connect");
        	return -1;
        }
    }
    //printf("%s\n", "connecting 3");

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


	this->buffer[request_length + strlen("\r\n\r\n")] = '\0';
	std::string keep_request(this->buffer);



	// for parse host, resource
	this->buffer[request_length] = '\0';
	/*printf("Get request%s\n", "--------------------------------------");
	printf("%s\n", this->buffer);
	printf("Get end request%s\n\n", "--------------------------------------");*/


	char * char_url, * char_host, * char_resource, * char_protocol;


	int res = HttpParser::parse_client_request(
	this->buffer, request_length, &char_url, &char_protocol, &char_host, &char_resource);


	if(res < 0){
		return -1;
	}


	//printf("%s\n", "Parsing end.");



	std::string string_resource(char_resource);


	this->host = new std::string();
	this->host->assign(char_host, strlen(char_host));

	if(NULL == char_url){
		//std::string string_resource(char_resource);
		this->url = new std::string((*this->host) + string_resource);

	}else{
		this->url = new std::string();
		this->url->assign(char_url, strlen(char_url));
		free(char_url);
	}
	free(char_protocol);
	free(char_resource);




	/*std::cout << "Url= ++" << *this->url << "++\n";
	std::cout << "Host= ++" << *this->host << "++\n";
	std::cout << "Resource= ++" << string_resource << "++\n";*/





	// GET запрос http1.1

	//!
	/*char buf[1024] = "";


	strcat(buf, this->host);
	strcat(buf, this->resource);










	//printf("Key is: ++%s++\n", buf);

	std::string string_key(buf);


	std::string * str_key = new std::string();
	str_key->assign(buf, strlen(buf));*/

	//std::cout << "Key is: ++" << *str_key << "++\n";




	std::map<std::string, CacheRecord *>::iterator it = this->cache->find(*this->url);



	sending_to_client = true;
	// if is cache finished


	// можно юзать кэш даже если он не закончен
	// так и надо сделать
	// 
	if(this->cache->end() != it){
		//printf("Use cache for: %s, res: %s\n", this->host, this->resource);
		std::cout << "Use cache for: ++" << *this->url << "++\n";

		//this->state = SessionState::USE_CACHE;
		this->state = USE_CACHE;
		buffer_write_position = 0;
		return 0;
	}else{
		//std::cout << "CACHE NOT FOUND FOR: ++" << *this->url << "++\n";
	}












	int port = 80;
	//printf("%s-%s-\n", "dns lookup for: ", this->host);
	int remote_socket = connect_to_host(char_host, port);
	free(char_host);

	//printf("%s\n", "dns lookup 2");


	if(remote_socket < 0){
		//close client socket=========================================================================================
		return -1;
	}



	// dangerous if data has Proxy-Connection, keep-alive


	//this->state = SessionState::SEND_REQUEST;
	this->state = SEND_REQUEST;
	this->remote_socket = remote_socket;





	/*std::string string_resource(this->resource);
	std::string string_url(this->url);*/


	replace_field(keep_request, *this->url, string_resource);



	//replace_field(keep_request, "Cache-Control: max-age=0", "Cache-Control: no-cache");
	replace_field(keep_request, "keep-alive", "close");
	replace_field(keep_request, "Proxy-Connection", "Connection");




	const char * b_copy = keep_request.c_str();

	this->request_length = strlen(b_copy);
	bcopy(b_copy, this->buffer, this->request_length);
	//strcpy(this->buffer, b_copy);
	this->buffer[this->request_length] = '\0';
	this->buffer_write_position = 0;



	// восстанваливаем запрос
	/*this->buffer[request_length] = '\r';


	std::string string_buffer(this->buffer);


	// dangerous if data has Proxy-Connection, keep-alive


	//replace_field(string_buffer, "Proxy-Connection", "Connection");


	replace_field(string_buffer, "keep-alive", "close");

	const char * b_copy = string_buffer.c_str();

	strcpy(this->buffer, b_copy);

	this->request_length = strlen(this->buffer);

	//printf("Len1: %d\n", this->request_length);


	//buffer_write_position = 0;
	this->request_length = this->request_length + strlen("\r\n\r\n");
	this->buffer[this->request_length] = '\0';*/



	//size_t REQUEST_LEN = 256;


	std::cout << "Send request to: ++" << *this->url << "++\n";

	//================================================================================seg
	//printf("Send request to: %s, res: %s\n", this->host, this->resource);


	    /*char* pattern = "GET %s %s\r\n";
    sprintf(changed_request, pattern, page, version);*/



	//printf("Len2: %d\n",this->request_length);

	/*printf("Send request to: %s, res: %s\n", this->host, this->resource);
	printf("%s\n", this->buffer);
	printf("End of request%s\n", "--------------------------------------");*/
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


	int r_end_length = strlen("\r\n\r\n");

	if(read_count > r_end_length){
		char * sub_position = strstr(this->buffer, "\r\n\r\n"); // ==============================cond jump uninit variable(fixed)

		if(NULL != sub_position){
			//printf("%s\n", "Reached end of request");

			int request_length = sub_position - this->buffer;


			return handle_client_request(request_length);
		}
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
		//this->state = SessionState::MANAGE_RESPONSE;
		this->state = MANAGE_RESPONSE;
	}
	//printf("Wrote to server: %d\n", write_count);	
	return 0;
}













int Session::manage_response(int poll_read_ready, int poll_write_ready){
	//printf("Session::manage_response\n");
	
	if(NULL == this->cache_record){
		this->cache_record = new CacheRecord(true);
	}





	std::map<std::string, CacheRecord *>::iterator cache_it = this->cache->find(*this->url);




	//тут может быть уже кэш ремувнут, мы его снова создадим без надобнасти
	if(this->cache->end() == cache_it && -1 == this->response_code){
		CacheRecord * record = new CacheRecord(false);

		this->cache->insert(std::pair<std::string, CacheRecord *>(*this->url, record)); 
	}






	if(poll_read_ready){


		assert(this->remote_socket	>= 0);

		int read_count = read(this->remote_socket, this->buffer, IO_BUFFER_SIZE);
		

		//write(1, this->buffer, read_count);


		if(0 == read_count){

			
			//this->sending_to_client	= false;


			//char  r_end[5] = "\r\n\r\n";


			int cache_size = this->cache_record->get_size();

			int r_end_length = strlen("\r\n\r\n");
			if(cache_size < r_end_length){

				// remote server send end of data to early
				return -1;
			}


			// can do this cause capasity is 2 * size
			//this->cache_record->get_data()[cache_size] = '\0';

			// refactor, search threw all response


			// found end of header
			//char * found = strstr(this->cache_record->get_data(), "\r\n\r\n");//====================================
			



			close(this->remote_socket);
			this->remote_socket = -1;
			printf("%s\n", "close server socket");
			//std::cout << *this->url << "\n";


			// после этого падает иногда
		

		}else if(read_count < 0){

			// ошибка чтения из сокета сервера
			perror("read");
			return -1;
		}







		int cache_size = this->cache_record->get_size();


		// check response code
		if(-1 == this->response_code && cache_size > 2){

			
			char * data = this->cache_record->get_data();
			char keep_char = data[cache_size]; 

			data[cache_size] = '\0';
			char * found = strstr(data, "\r\n");

			if(NULL != found){
				int first_line_length = found - data;
				//HTTP/1.1 200 OK
				/*if(first_line_length < ){

				}*/
				found = strstr(data, " ");

				if(NULL == found){

					// bad response
					return -1;
				}

				//cheack borders: advanced


				++found;	

				*(found + 3) = '\0';

				// dang
				this->response_code = atoi(found);

				if(this->response_code < 100 || this->response_code > 505){
					printf("%s%d\n", "Unknown response code: ", this->response_code);
					return -1;
				}

				std::map<std::string, CacheRecord *>::iterator cache_it = this->cache->find(*this->url);
				

				assert(this->cache->end() != cache_it);
				data[cache_size] = keep_char;
				*(found + 3) = ' ';

				// если код 206 например, то будет для каждого запроса новое подключение
				if(200 == this->response_code){
					/*data[cache_size] = keep_char;
					*(found + 3) = ' ';*/
					cache_it->second->add_data(this->cache_record->get_data(), this->cache_record->get_size());

					assert(this->cache_record->is_local());
					delete this->cache_record;

					this->cache_record = cache_it->second;
				}else{
					cache_it->second->outdated();
					delete cache_it->second;
					this->cache->erase(cache_it);
				}



				std::cout << "Response code for: " << *this->url << " is " << this->response_code << "\n";

				//*(found + 3) = ' ';

			}
			data[cache_size] = keep_char;
		}



		int res = 0;


		



		if(this->cache_write_position + read_count < cache_size){
			// не нужно писать в кэш, ждем пока данные придут с нужного смещения
			this->cache_write_position+=read_count;
		}else{
			if(0 == read_count){
				this->cache_record->finish();
				printf("%s\n", "Cache finish");
				//std::cout << *this->url << "\n";
			}else{
				int to_add = this->cache_write_position + read_count - cache_size;


				
				
				// добавляем данные в кэш
				this->cache_write_position+=read_count;



				//write(1, this->buffer + (read_count - to_add), to_add);


				res = this->cache_record->add_data(this->buffer + (read_count - to_add), to_add);
				

				if(res < 0){
					// just exit from function --> delete current session
					return -1;
				}
			}


		}
	}









	if(poll_write_ready){


		assert(this->client_socket	>= 0);

		char * data = this->cache_record->get_data();
		int size = this->cache_record->get_size();

		if(NULL == data){
			return 0;
		}

		//printf("%s\n", "poll_write1");

		int to_write = size - this->cache_read_position;


		assert(to_write >= 0);
		//printf("%s\n", "poll_write2");

		if(0 == to_write && this->cache_record->is_full()){
			this->sending_to_client	= false;


			close(this->client_socket);
			this->client_socket = -1;

			printf("%s\n", "All data had writen");
			//std::cout << *this->url << "\n";
			//printf("%s\n", "close connection");

			//delete session: ok
			return -1;
		}

		//printf("%s\n", "poll_write3");

		if(0 != to_write){




			/*if(to_write > IO_BUFFER_SIZE){
				to_write = IO_BUFFER_SIZE;
			}*/





			//printf("%s\n", "poll_write4");

			int write_count = write(this->client_socket, data + this->cache_read_position, to_write);



			//write(1, data + this->cache_read_position, to_write);

			//printf("%s\n", "poll_write5");
			if(write_count < 0){

				// возможно отсоединился клиент
				printf("%s\n", "poll_write6");
				close(this->client_socket);
				this->client_socket	= -1;
				this->sending_to_client = false;


				printf("%s\n", "close client socket");
				perror("write");


				// do not delete session, докачиваем данные
				return 0;
			}

			this->cache_read_position+=write_count;
			

		}
	}

	return 0;
}




//=================================================================
int Session::use_cache(){

	if(NULL == this->cache_record){
		std::map<std::string, CacheRecord *>::iterator it = this->cache->find(*this->url);
		this->cache_record = it->second;

		if(this->cache->end() == it){
			printf("%s\n", "Cache error");
			return -1;
		}
	}

	//std::cout << "Bad:\n" << *this->url << "\n";

	// what if cache is not full and is not in progress--> open connection

	
	if(this->cache_record->is_outdated()){
		// connect to host and unchange cache_read_position
		
	}



	char * data = this->cache_record->get_data();
	int size = this->cache_record->get_size();



		/*std::cout << "Size:\n" << size << "\n";
	std::cout << "read_pos:\n" << this->cache_read_position << "\n";
	std::cout << "outdated:\n" << this->cache_record->is_outdated() << "\n";*/

	if(0 == size){
		return 0;
	}
	assert(NULL != data);

	int to_write = size - this->cache_read_position;
	assert(to_write >= 0);

	/*if(to_write > IO_BUFFER_SIZE){
		to_write = IO_BUFFER_SIZE;
	}*/

	if(0 == to_write && this->cache_record->is_full()){
		//delete session ok
		return -1;
	}

	//нет crlf в конце у сайта с исо при перезагрузке

	int write_count = write(this->client_socket, data + this->cache_read_position, to_write);

	//write(1, data + this->cache_read_position, to_write);

	if(write_count < 0){
		perror("cache write");
		return -1;
	}

	this->cache_read_position+=to_write;
	return 0;
}




int Session::close_sockets(){


	if(-1 != this->client_socket){
		close(this->client_socket);
	}

	if(-1 != this->remote_socket){
		close(this->remote_socket);
	}

}