#include"proxy.h"


Proxy::Proxy(int listener){
	this->listener = listener;
	this->is_alive = true;
}


void Proxy::start(){

	int poll_wait_infinite = -1;

	pollfd listener_pollfd;
	int listener_index = 0;
	listener_pollfd.fd = this->listener;
	listener_pollfd.events = POLLIN;


	this->fdset.push_back(listener_pollfd);

	pollfd * fds_array = NULL;

	

	while(this->is_alive){

		int result = 0;

		result = update_sessions();

		if(result < 0){
			printf("%s\n", "update sessions error...");
			break;
		}

		fds_array = &this->fdset[0];

		assert(NULL != fds_array);


		int fdset_size = this->fdset.size();	
		int poll_result = poll(fds_array, fdset_size, poll_wait_infinite);


		if(poll_result < 0){
			perror("poll");
			break;
		}

		int sessions_count = this->sessions.size();
		
		for(int i = 0; i < sessions_count; ++i){
			result = serve_session(this->sessions[i], &fds_array[i * 2 + 1]);
			if(result < 0){
				assert(NULL != this->sessions[i]);
				delete this->sessions[i];
				this->sessions.erase(this->sessions.begin() + i);
				this->fdset.erase(this->fdset.begin() +  (i * 2 + 1));
				this->fdset.erase(this->fdset.begin() +  (i * 2 + 1));
				--i;
				--sessions_count;
			}
		}

		if(POLLIN & fds_array[listener_index].revents){
			result = accept_connection();
			if(result < 0){
				break;
			}
		}
	}

	close_all_sessions();

}





int Proxy::accept_connection(){
	int client_socket = accept(this->listener, NULL, NULL);


    if(fcntl(client_socket, F_SETFL, O_NONBLOCK) == -1){
        perror("fcntl");
        return -1;
    }

	if(client_socket < 0){
		perror("accept");
		return -1;
	}

    Session * session = NULL;
    try
    {
        session = new Session(client_socket, &this->cache);
    }
    catch (std::bad_alloc& ba)
    {
        std::cerr << "bad_alloc caught: " << ba.what() << '\n';
        return -1;
    }
	
	this->sessions.push_back(session);
	return 0;
}	


int Proxy::serve_session(Session * session, pollfd * fds){
	
	assert(NULL != session);
	assert(NULL != fds);


	session->getState();


	int res = 0;

    switch (session->getState()) {

        case RECEIVE_CLIENT_REQUEST:
            
            if((POLLHUP | POLLERR) & fds[0].revents){
            	fprintf(stderr, "Client closed!\n");
            	session->close_sockets();
            	return -1;
            }

            if(POLLIN & fds[0].revents){
            	res = session->read_client_request();
            	if(res < 0){
            		session->close_sockets();
            	}
            	return res;
        	}
            break;
        case SEND_REQUEST:

            
            if ((POLLHUP | POLLERR) & fds[1].revents) {
                fprintf(stderr, "Remote server closed!\n");
                session->close_sockets();
                return -1;
            }

            if (POLLOUT & fds[1].revents) {
            	res = session->send_request();
            	if(res < 0){
            		session->close_sockets();
            	}

            	return res;
            }
            break;

        case MANAGE_RESPONSE:

            if ((fds[0].revents & (POLLHUP | POLLERR)) || (fds[1].revents & (POLLHUP | POLLERR))) {
            	session->close_sockets();
                std::cout << "Close connections for:===========================" << "\n";
                fprintf(stderr, "Connection closed POLLHUP!\n");
                return -1;
            }

            if ((fds[1].revents & POLLIN) || (session->is_sending() && (fds[0].revents & POLLOUT))) {
            	int res = session->manage_response(fds[1].revents & POLLIN, fds[0].revents & POLLOUT);
            	if(res < 0){
            		session->close_sockets();
            	}
                return res;
            }

            

            break;



        case USE_CACHE:
            if (fds[0].revents & POLLHUP) {
                fprintf(stderr, "Connection closed!\n");
                session->close_sockets();
                return -1;
            }
            if(fds[0].revents & POLLOUT){

            	res = session->use_cache();
            	if(res < 0){
            		session->close_sockets();
            	}
            	return res;

            }

            break;
        default:
            printf("%s\n", "Invalid session state");
        	return -1;

	}



	return 0;
}




int Proxy::update_sessions(){
	int sessions_count = this->sessions.size();
	this->fdset.reserve(sessions_count * 2 + 1);
	int fds_count = (this->fdset.size() - 1) / 2;


    this->fdset[0].fd = this->listener;
    this->fdset[0].events = POLLIN;


    pollfd new_pollfd;
    
    for (int i = 0; i < sessions_count; i++) {

    	
    	int client_i = 1 + i * 2;
    	int server_i = client_i + 1;



        if(fds_count < i + 1){
       		this->fdset.push_back(new_pollfd);
       		this->fdset.push_back(new_pollfd);
        }


        this->fdset[client_i].fd = this->sessions[i]->client_socket;
        this->fdset[server_i].fd = this->sessions[i]->remote_socket;


         this->fdset[client_i].revents = 0;
         this->fdset[server_i].revents = 0;

        switch (sessions[i]->getState()) {
            case RECEIVE_CLIENT_REQUEST:
                this->fdset[client_i].events = POLLIN;
                this->fdset[server_i].events = 0;
                break;
            case SEND_REQUEST:
                this->fdset[client_i].events = 0;
                this->fdset[server_i].events = POLLOUT;
                break;

            case MANAGE_RESPONSE:
                this->fdset[client_i].events = POLLOUT;
                this->fdset[server_i].events = POLLIN;
                break;
            case USE_CACHE:
                this->fdset[client_i].events = POLLOUT;
                this->fdset[server_i].events = 0;
                break;
            default:
            	printf("%s\n", "Invalid session state");
            	return -1;
        }
    }

    return 0;
}






void Proxy::close_all_sessions(){
	printf("%s\n", "All sessions closed");
	for(size_t i = 0; i < this->sessions.size(); ++i){
		this->sessions[i]->close_sockets();
		assert(NULL != this->sessions[i]);
		delete this->sessions[i];
	}
}


Proxy::~Proxy(){
    close_all_sessions();

    for(std::map<std::string, CacheRecord *>::iterator it = this->cache.begin(); it != this->cache.end(); ++it) {
        delete it->second;
    }
    this->cache.clear();
}
