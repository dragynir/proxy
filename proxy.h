#pragma once



#include"session.h"


#include<string>
#include<vector>
#include<map>
#include<poll.h>



class Proxy{


public:

	Proxy(int listener);

	~Proxy();


	void start();

private:

	int accept_connection();

	int serve_session(Session * session, pollfd * fds);

	int update_sessions();

	void close_all_sessions();


	int listener;
	bool is_alive;
	std::map<std::string, CacheRecord *> cache;
	std::vector<pollfd> fdset;
	std::vector<Session *> sessions;
};
