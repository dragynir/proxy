

#include <unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<strings.h>
#include<string.h>


#include<errno.h>
#include<assert.h>


class CacheRecord{


public:

	CacheRecord();

	bool is_full(){return full;}

	void finish(){this->full = true;};

	bool is_empty(){return 0 == size;}

	bool in_progress(){return !full && (0 != size);}

	int add_data(char * data, size_t size);

	int get_size(){return size;}

	char * get_data();

private:


	char * data;
	size_t capacity;
	size_t size;
	bool full;

};
