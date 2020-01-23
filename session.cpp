#include"session.h"


Session::Session(int client_socket, std::map<std::string, CacheRecord *> * cache){

	this->client_socket = client_socket;
	this->remote_socket = -1;
	this->response_code = -1;


	this->buffer = NULL;
	this->buffer_capacity = 0;


	this->cache = cache;
	this->cache_record = NULL;
	this->global_cache_record = NULL;
	this->use_global_record = false;

	state = RECEIVE_CLIENT_REQUEST;
	cache_read_position = 0;
	cache_write_position = 0;
	response_answer_code = 0;

	sending_to_client = false;
	reconnect = false;


	buffer_write_position = 0;
	buffer_read_position = 0;
	request_length = 0;
}



Session::~Session(){
	if(!use_global_record){
		delete this->cache_record; 
	}
	free(this->buffer);
}




int Session::connect_to_host(const char* hostname, int port) {
    int s;
    hostent* server_host;
    sockaddr_in servaddr;

	server_host = gethostbyname(hostname);

	if(NULL == server_host){
		printf("%s%s\n", "Can't reach: ",hostname);
		return -1;
	}


    if (server_host == NULL) {
        fprintf(stderr, "Wrong hostname. Can't resolve it\n");
        return -1;
    }


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;

    memcpy(&servaddr.sin_addr.s_addr, server_host->h_addr_list[0], sizeof(struct in_addr));
    servaddr.sin_port = htons(port);

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

    return s;
}



int Session::replace_field(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos){
        return -1;
    }
    str.replace(start_pos, from.length(), to);
    return 0;
}



int Session::handle_client_request(int request_length){


	this->keep_request.append(this->buffer, request_length + strlen("\r\n\r\n"));


	this->buffer[request_length + strlen("\r\n\r\n")] = '\0';


	this->buffer[request_length] = '\0';

	char * char_url, * char_host, * char_resource;


	int res = HttpParser::parse_client_request(
	this->buffer, request_length, &char_url, &char_host, &char_resource);


	if(res < 0){
		std::cout << "Receive unsupported request." << "\n";
		return -1;
	}

	std::string string_resource(char_resource);


	this->host.append(char_host, strlen(char_host));

	if(NULL == char_url){
		this->url = this->host + string_resource;

	}else{
		this->url.append(char_url, strlen(char_url));
		free(char_url);
	}
	free(char_resource);




	std::cout << "Url= ++" << this->url << "++\n";
	std::cout << "Host= ++" << this->host << "++\n";
	std::cout << "Resource= ++" << string_resource << "++\n";




	replace_field(this->keep_request, this->url, string_resource);
	replace_field(this->keep_request, "keep-alive", "close");
	replace_field(this->keep_request, "Keep-Alive", "close");
	replace_field(this->keep_request, "Proxy-Connection", "Connection");



	std::map<std::string, CacheRecord *>::iterator it = this->cache->find(this->url);


	sending_to_client = true;

	if(this->cache->end() != it){


		if(it->second->is_outdated()){

			this->cache->erase(it);
			if(0 == it->second->links()){
				delete it->second;
			}

		
			try
			{
				this->global_cache_record = new CacheRecord(false);
			}
			catch (std::bad_alloc& ba)
			{
				std::cout << "\n\n\n\n\n\nbad_alloc caught: " << ba.what() << '\n';
				try_erase_cache();
				return -1;
			}catch(std::exception& exc){
				std::cout << "Caught: " << exc.what() << '\n';
				try_erase_cache();
				return -1;
			}

			std::cout << "Create cache for: " << this->url << "\n";


			this->global_cache_record->use();
			this->cache->insert(std::pair<std::string, CacheRecord *>(this->url, this->global_cache_record)); 

		}else{
			std::cout << "\n<=======================Use cache for: " << this->url << "\n";
			this->state = USE_CACHE;
			use_global_record = true;
			it->second->use();
			this->buffer_write_position = 0;
			return 0;
		}

	}else{
		

		try
		{
			this->global_cache_record = new CacheRecord(false);
		}
		catch (std::bad_alloc& ba)
		{
			std::cout << "\n\n\n\n\n\nbad_alloc caught: " << ba.what() << '\n';
			try_erase_cache();
			return -1;
		}catch(std::exception& exc){
			std::cout << "Caught: " << exc.what() << '\n';
			try_erase_cache();
			return -1;
		}


		std::cout << "Create cache for: " << this->url << "\n";



		this->global_cache_record->use();
		this->cache->insert(std::pair<std::string, CacheRecord *>(this->url, this->global_cache_record));
	}

	it = this->cache->find(this->url);




	int port = 80;
	int remote_socket = connect_to_host(char_host, port);
	free(char_host);


	if(remote_socket < 0){
		delete this->global_cache_record;
		this->global_cache_record = NULL;
		this->cache->erase(it);
		return -1;
	}


	this->state = SEND_REQUEST;
	this->remote_socket = remote_socket;


	const char * b_copy = this->keep_request.c_str();

	this->request_length = strlen(b_copy);
	bcopy(b_copy, this->buffer, this->request_length);
	this->buffer[this->request_length] = '\0';
	this->buffer_write_position = 0;


	std::cout << "\n------------------------>Send request to: " << this->url << "\n";
	

	return 0;
}



int Session::read_client_request(){

	if(NULL == this->buffer){
		this->buffer = (char *)malloc(SESSION_BUFFER_SIZE);
		if(NULL == this->buffer){
			perror("malloc");
			return -1;
		}
		this->buffer_capacity = SESSION_BUFFER_SIZE;
	}


	if(this->buffer_write_position + IO_BUFFER_SIZE  + 1 > this->buffer_capacity){
		int factor = 2;
		this->buffer = (char *)realloc(this->buffer, this->buffer_capacity * factor);

		if(NULL == this->buffer){
			perror("malloc");
			return -1;
		}
		this->buffer_capacity = this->buffer_capacity * factor;
	}


	
	int read_count = read(this->client_socket, this->buffer + this->buffer_write_position, IO_BUFFER_SIZE);





	if(read_count < 0){
		perror("read: read_client_request");
		return -1;
	}

	if(read_count > 0){
		this->buffer_write_position+=read_count;
	}


	int r_end_length = strlen("\r\n\r\n");


	if(this->buffer_write_position > r_end_length){

		char * sub_position = strstr(this->buffer, "\r\n\r\n");

		if(NULL != sub_position){
			int request_length = sub_position - this->buffer;
			return handle_client_request(request_length);
		}
	}

	if(0 == read_count){
		std::cout << "Can't find request end." << "\n";
		return -1;
	}
	return 0;
}



int Session::send_request(){
	
	int to_write = this->request_length - this->buffer_read_position;


	int write_count = write(this->remote_socket, this->buffer + this->buffer_read_position, to_write);


	if(write_count < 0){
		if(EAGAIN != errno && EWOULDBLOCK != errno){
			perror("write");
			try_erase_cache();
			return -1;
		}
		return 0;
	}


	this->buffer_read_position+=write_count;

	if(this->buffer_read_position == this->request_length){
		this->buffer_read_position = 0;
		this->buffer_write_position = 0;
		this->state = MANAGE_RESPONSE;
	}
	return 0;
}










void Session::try_erase_cache(){

	if(NULL == this->global_cache_record){
		return;
	}


	this->global_cache_record->outdated();
	if(0 == this->global_cache_record->links() && 0 == this->global_cache_record->get_size()){
		std::cout << "Delete cache for: " << this->url << "\n";
		delete this->global_cache_record;
		std::map<std::string, CacheRecord *>::iterator  it = this->cache->find(this->url);


		assert(this->cache->end() != it);

		this->cache->erase(it);
		this->global_cache_record = NULL;
	}
}


int Session::manage_response(int poll_read_ready, int poll_write_ready){
	
	if(NULL == this->cache_record){

		try{
			this->cache_record = new CacheRecord(true);
		}
		catch (std::bad_alloc& ba)
		{
			std::cerr << "\n\n\n\n\n\nbad_alloc caught: " << ba.what() << '\n';
			try_erase_cache();
			return -1;
		}catch(std::exception& exc){
			std::cout << "Caught: " << exc.what() << '\n';
			try_erase_cache();
			return -1;
		}

	}


	if(poll_read_ready){


		assert(this->remote_socket	>= 0);


		int read_count = read(this->remote_socket, this->buffer, IO_BUFFER_SIZE);

		
		if(0 == read_count){

			close(this->remote_socket);
			this->remote_socket = -1;

			if(false == this->sending_to_client){
				if(NULL != this->global_cache_record){
					this->global_cache_record->unuse();
				}
			}

		
		}else if(read_count < 0){
			try_erase_cache();
			close(this->client_socket);
			close(this->remote_socket);
			this->remote_socket = -1;
			this->client_socket = -1;

			perror("read, manage_response");
			return -1;
		}







		int cache_size = this->cache_record->get_size();

		int res = 0;



		if(this->cache_write_position + read_count < cache_size){
			this->cache_write_position+=read_count;
		}else{
			if(0 == read_count){
				this->cache_record->finish();
				
				std::cout << "Write cache finish for: " << this->url << "\n";
				
			}else{
				int to_add = this->cache_write_position + read_count - cache_size;
				
				this->cache_write_position+=read_count;

				res = this->cache_record->add_data(this->buffer + (read_count - to_add), to_add);
				

				if(res < 0){
					std::cout << "Out of memory on: " << this->url << "\n";
					try_erase_cache();
					return -1;
				}

			}

		}


		cache_size = this->cache_record->get_size();


		if(-1 == this->response_code && cache_size > 2/*можем проверить конец строки*/){

			char * data = this->cache_record->get_data();
			char keep_char = data[cache_size - 1]; 
			data[cache_size - 1] = '\0';

			char * found = strstr(data, "\r\n");

			if(NULL != found){

				found = strstr(data, " ");

				if(NULL == found){
					try_erase_cache();
					std::cout << "Bad response(parse response_code)" << "\n";
					return -1;
				}

				++found;	

				*(found + 3) = '\0';

				// dang
				this->response_code = atoi(found);

				if(this->response_code < 100 || this->response_code > 505){
					printf("%s%d\n", "Unknown response code: ", this->response_code);
					try_erase_cache();
					return -1;
				}


				data[cache_size - 1] = keep_char;
				*(found + 3) = ' ';

				if(200 == this->response_code){

					this->global_cache_record->add_data(this->cache_record->get_data(), this->cache_record->get_size());

					if(this->cache_record->is_full()){
						this->global_cache_record->finish();
					}

					assert(this->cache_record->is_local());
					delete this->cache_record;
					this->cache_record = this->global_cache_record;
					use_global_record = true;
					assert(NULL != this->cache_record);

				}else{
					try_erase_cache();
				}



				std::cout << "Response code for: " << this->url << " is " << this->response_code << "\n";

			}else{
				data[cache_size - 1] = keep_char;
			}
		}



		if(0 == read_count && -1 == this->response_code){
			try_erase_cache();
			std::cout << "Response code didn't found, but found end of response for: " << this->url << "\n";
			return -1;
		}
	}












	if(poll_write_ready){


		assert(this->client_socket	>= 0);



		char * data = this->cache_record->get_data();
		int size = this->cache_record->get_size();

		if(NULL == data){
			assert(0 == size);
			return 0;
		}


		int to_write = size - this->cache_read_position;


		assert(to_write >= 0);
		

		if(0 == to_write && this->cache_record->is_full()){
			this->sending_to_client	= false;
			std::cout << "All data had writen for: " << this->url << "\n";
			close(this->client_socket);
			this->client_socket = -1;

			if(NULL != this->global_cache_record){
				this->global_cache_record->unuse();
			}
			//cache is ok, just delete session
			return -1;
		}


		if(0 != to_write){
			/*if(to_write > IO_BUFFER_SIZE){
				to_write = IO_BUFFER_SIZE;
			}*/

			int write_count = write(this->client_socket, data + this->cache_read_position, to_write);

			//write(1, data + this->cache_read_position, to_write);

			if(write_count < 0){
				if(EAGAIN != errno && EWOULDBLOCK != errno){
					close(this->client_socket);
					this->client_socket	= -1;
					this->sending_to_client = false;
					std::cout << "Close client socket for: " << this->url << "\n";
					perror("write to client: ");
				}

				// докачиваем данные
				return 0;
			}

			this->cache_read_position+=write_count;
			

		}


	}

	return 0;
}






int Session::use_cache(){


	if(NULL == this->cache_record){
		std::map<std::string, CacheRecord *>::iterator it = this->cache->find(this->url);
		this->cache_record = it->second;

		if(this->cache->end() == it){
			std::cout << "\nCache error\n" << "\n";
			return -1;
		}
	}



	if(this->cache_record->is_outdated()){
		this->global_cache_record = this->cache_record;
		try_erase_cache();
		close_sockets();
		std::cout << "Cache is outdated" << "\n";
		return -1;
	}


	char * data = this->cache_record->get_data();
	int size = this->cache_record->get_size();


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
		std::cout << "All data had writen(cache) for: " << this->url << "\n";
		this->cache_record->unuse();
		//delete session ok
		return -1;
	}


	int write_count = write(this->client_socket, data + this->cache_read_position, to_write);

	if(write_count < 0){
		if(EAGAIN != errno && EWOULDBLOCK != errno){
			perror("cache write");
			this->cache_record->unuse();
			return -1;
		}
		return 0;
	}

	this->cache_read_position+=write_count;



	return 0;
}




void Session::close_sockets(){

	if(-1 != this->client_socket){
		close(this->client_socket);
	}

	if(-1 != this->remote_socket){
		close(this->remote_socket);
	}
}
