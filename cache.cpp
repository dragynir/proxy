
#include"cache.h"



CacheRecord::CacheRecord(bool local){
	this->data = NULL;
	this->local = local;
	this->size = 0;
	this->full = false;
	this->out_of_date = false;
	this->capacity = 0;
	this->links_count = 0;
}


CacheRecord::~CacheRecord(){
	free(this->data);
}


int CacheRecord::add_data(char * add_data, size_t add_size){
	if(0 == add_size){
		return 0;
	}

	size_t new_size = add_size + this->size;
	int factor = 2;
	if(new_size > this->capacity){

		if(NULL == this->data){
			this->data = (char *)malloc(new_size * factor);
		}else{
			this->data = (char *)realloc(this->data, new_size * factor);
		}

		if(NULL == this->data){
			exit(EXIT_FAILURE);
		}
		

		if(NULL == this->data){
			perror("realloc");
			return -1;
		}
		this->capacity = new_size * factor;
	}



	bcopy(add_data, this->data + this->size, add_size);
	assert(new_size >= this->size);
	this->size = new_size;




	return 0;
}


char * CacheRecord::get_data(){
	return this->data;
}
