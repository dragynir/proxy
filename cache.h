

#include <unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<strings.h>
#include<string.h>


#include<errno.h>
#include<assert.h>


class CacheRecord{


public:

	CacheRecord(bool local);

	~CacheRecord();

	bool is_full(){return full;}

	void finish(){this->full = true;};

	bool is_outdated(){return out_of_date;}
	bool is_local(){return this->local;}

	void outdated(){
		this->out_of_date = true;
		this->links_count--;
	}


	bool is_empty(){return 0 == size;}

	bool in_progress(){return !full && (0 != size);}


	/*bool progress();
	bool stoped();*/


	void use(){
		this->out_of_date = false;
		this->links_count++;
	}

	void unuse(){this->links_count--;}
	int links(){return this->links_count;}

	int add_data(char * data, size_t size);

	int get_size(){return size;}

	char * get_data();

private:

	int links_count;

	char * data;
	size_t capacity;
	size_t size;
	bool full;
	bool out_of_date;
	bool local;

};
