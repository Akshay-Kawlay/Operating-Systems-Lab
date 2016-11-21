#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>

struct wc;
struct element;

struct element*
cache_lookup(struct file_data* file, int LRU_flag);

struct element*
cache_insert(struct file_data* file);

void
cache_evict(int amount_to_evict);

//void
//upgrade_LRU(struct file_data *file);

void
upgrade_LRU_2(struct element *table_entry, int inserting);

void *stub(void *sv);

unsigned long
hash(char *str);/*hash function declaration*/

struct wc*
create_hash(struct wc* wc, long size);/*creates empty hash table*/

void
wc_destroy(struct wc *wc);

struct wc {
	/* you can define this struct to have whatever fields you want. */
	struct element** hashtable;
	int cache_size;
	int remaining_cache_size;
	int table_size;
	struct element* LRU_element;
	struct element* LRU_tail;
	pthread_mutex_t *table_lock;
	//int pin_count;
};

struct element{
	//char *filename;
	struct file_data *file;
	struct element *next;
	struct element *pre;
	int rc;
};

struct wc *cache;

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	/* add any other parameters you need */
	int *buffer;
	int in;
	int out;
	pthread_t *worker_TCB;
	pthread_cond_t *buffer_full;
	pthread_cond_t *buffer_empty;
	pthread_mutex_t *buffer_lock;
};

/* static functions */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	//int ret;
	
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fills data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}

	struct element *cached_entry = NULL;
	
	if(sv->max_cache_size > 0){
		pthread_mutex_lock(cache->table_lock);
		
		cached_entry = cache_lookup(data, 1);
		
		pthread_mutex_unlock(cache->table_lock);
	}

	if(cached_entry != NULL){

		request_set_data(rq, cached_entry->file);
		//pthread_mutex_unlock(cache->table_lock);
		
		request_sendfile(rq);
		//request_destroy(rq);
		//printf("sending-cache hit!: %s\n", cached_entry->file->file_name);

	}else{
		//pthread_mutex_unlock(cache->table_lock);
		request_readfile(rq);
		//printf("read: %s\n", data->file_name);
		request_sendfile(rq);
		//printf("sent\n");
		//request_destroy(rq);
		if(sv->max_cache_size > 0){
			pthread_mutex_lock(cache->table_lock);
			struct element *check = cache_lookup(data, 0);
			
			if(check == NULL){
				//printf("inserting cache\n");
				//printf("%s\n", data->file_name);
				cached_entry = cache_insert(data);
				//printf("inserted in cache");
				//printf("inserted in cache: %s\n", cached_entry->file->file_name);
			}else{
				//printf("Avoided double caching\n");
			}
			pthread_mutex_unlock(cache->table_lock);
		
		}
	//request_sendfile(rq);

	}
	request_destroy(rq);
	/* reads file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	//ret = request_readfile(rq);
	//if (!ret)
	
	/* sends file to client */
	//request_sendfile(rq);

	//request_destroy(rq);
	
}

/* entry point functions */

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;
	//pthread_t *worker_TCB = malloc(nr_threads*sizeof(pthread_t));

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;

	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		
		/*Lab 5 addition*/
		if(max_cache_size >0){
			cache = malloc(sizeof(struct wc));
			cache->cache_size = max_cache_size;
			cache->remaining_cache_size = max_cache_size;
			int table_size = max_cache_size*11;
			cache->table_size = table_size;
			cache = create_hash(cache, table_size);
			
			cache->LRU_element = NULL;
			cache->LRU_tail = NULL;
			cache->table_lock = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(cache->table_lock, NULL);
			
		}
		
		
		sv->buffer = malloc(sizeof(int)*(max_requests+1));
		sv->in = 0;
		sv->out = 0;
		sv->worker_TCB = malloc(nr_threads*sizeof(pthread_t));
		sv->buffer_full = malloc(sizeof(pthread_cond_t));
		sv->buffer_empty = malloc(sizeof(pthread_cond_t));
		sv->buffer_lock = malloc(sizeof(pthread_mutex_t));
		pthread_cond_init(sv->buffer_full, NULL);
		pthread_cond_init(sv->buffer_empty, NULL);
		pthread_mutex_init(sv->buffer_lock, NULL);
		
		int i;
		for(i = 0; i < nr_threads; i++){
			pthread_create(&sv->worker_TCB[i], NULL, stub, (void *)sv);
		}

	}

	/* Lab 4: create queue of max_request size when max_requests > 0 */
	
	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */
	
	return sv;
}

/*_____________Hash Function________________________*/

unsigned long
hash(char *str)
{
	unsigned long hash = 5381;
	int c;

	while ((c = *str++) != 0)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}


/*__________________Create Hash Table_______________________*/

struct wc*
create_hash(struct wc* wc, long size){

	long table_size = size;///3;/*((long)sqrt(size)+2);*/
	//TABLE_SIZE = table_size;
	wc->hashtable = (struct element **)malloc(sizeof(struct element *) * table_size);
	int i;
	assert(wc->hashtable!=NULL);
	for(i=0; i<table_size; i++){
		wc->hashtable[i] = NULL;
	}

	return wc;
}


void
wc_destroy(struct wc *wc)
{
	struct element **hasht = wc->hashtable;
	unsigned long i;
	for(i = 0; i < cache->table_size; i++){
		if(hasht[i] != NULL){
			//free(hasht[i]->file_name);
			free(hasht[i]);
		}
	}
	free(hasht);
	free(wc);
}

/*_________________________________________________*/


void *
stub(void *sv)
{
	struct server *sv2 = (struct server *)sv;
	while(sv2->max_requests >=0 && sv2->nr_threads > 0){
		pthread_mutex_lock(sv2->buffer_lock);
		/*while buffer is empty, wait*/
		while(sv2->in == sv2->out){
			pthread_cond_wait(sv2->buffer_empty, sv2->buffer_lock);
		}
	
		int connfd = sv2->buffer[sv2->out];
	
		if((sv2->in - sv2->out + sv2->max_requests + 1)%(sv2->max_requests + 1) == sv2->max_requests){
			pthread_cond_signal(sv2->buffer_full);
		}
		/*update sv->out*/
		sv2->out = (sv2->out + 1)%(sv2->max_requests+1);	
	
		pthread_mutex_unlock(sv2->buffer_lock);
		do_server_request(sv2, connfd);

	}

	assert(0);

}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		pthread_mutex_lock(sv->buffer_lock);
		
		while((sv->in - sv->out + sv->max_requests + 1)%(sv->max_requests + 1) == sv->max_requests){
			pthread_cond_wait(sv->buffer_full, sv->buffer_lock);
		}
		sv->buffer[sv->in] = connfd;
		
		if(sv->in == sv->out){
			pthread_cond_signal(sv->buffer_empty);
		}
		/*update sv->in*/
		sv->in = (sv->in + 1)%(sv->max_requests+1);
		
		pthread_mutex_unlock(sv->buffer_lock);
	}
}


/*_______________CACHE FUNCTIONS LAB 5______________________________________________*/

struct element*
cache_lookup(struct file_data* file, int LRU_flag){
	//printf("cache lookup\n");
	int key = hash(file->file_name)%(cache->table_size);
	//printf("%s\n", file->file_name);
	//printf("%d\n", LRU_flag);
	//printf("%d\n", key);	
	while(cache->hashtable[key] != NULL){
		if(!strcmp(file->file_name, cache->hashtable[key]->file->file_name)){
			/*update LRU_element*/
			if(LRU_flag)
				upgrade_LRU_2(cache->hashtable[key], 0);
			//printf("Cache Hit\n");
			return cache->hashtable[key];
		}
		//printf("table not null");
		key = (key*key)%(cache->table_size);//probe
	}
	return NULL;
}

struct element*
cache_insert(struct file_data* file){
  	//printf("In cache_insert\n");
	if(file->file_size > 0.5*cache->cache_size)
  		return NULL;
	
	if(file->file_size >= cache->remaining_cache_size){
		//printf("evict\n");
		cache_evict(file->file_size);
	}
	unsigned long key = hash(file->file_name)%(cache->table_size);
	struct element *temp_new = malloc(sizeof(struct element));
	temp_new->file = file;
	temp_new->next = NULL;
	temp_new->pre = NULL;
	
	if(cache->hashtable[key] == NULL){
		//printf("entry free\n");
		cache->hashtable[key] = temp_new;
		cache->remaining_cache_size -= file->file_size;
	}else{
	  	unsigned long new_key = (key*key)%(cache->table_size);
		while(cache->hashtable[new_key] != NULL){
			new_key = (new_key*new_key)%(cache->table_size);//probe
		}
		cache->hashtable[new_key] = temp_new;
		cache->remaining_cache_size -= file->file_size;
	}
	
	//printf("%s\n", temp_new->file->file_name);
	//printf("%d\n", temp_new->file->file_size);
	/*update_LRU_element*/
	upgrade_LRU_2(temp_new, 1);
	//printf("insert function returns\n");
	//printf("%s\n", temp_new->file->file_name);
	//printf("%d\n", temp_new->file->file_size);

	return temp_new;
}

void
cache_evict(int amount_to_evict){
	//printf("cache_evict\n");
	while(cache->remaining_cache_size > amount_to_evict){
		struct element *temp = cache->LRU_element;
		if(cache->LRU_element != NULL){
			cache->LRU_element = cache->LRU_element->next;
			cache->LRU_element->pre = NULL;
		
			cache->remaining_cache_size += temp->file->file_size;
			unsigned long key = hash(temp->file->file_name)%(cache->table_size);
			while(cache->hashtable[key] != temp){
				key = (key*key)%(cache->table_size);//probe
			}

			file_data_free(temp->file);
			free(temp);
			cache->hashtable[key] = NULL;
		}
	}
}

/*____________________Least_Recently_Used_Replacement_Implementation___________________*/

void
upgrade_LRU_2(struct element *table_entry, int inserting){
	//printf("LRU enterd\n");
	if(cache->LRU_element == NULL){//list is empty
		//printf("LRU head is NULL\n");
		cache->LRU_element = table_entry;
		table_entry->next = NULL;
		table_entry->pre = NULL;
		cache->LRU_tail = cache->LRU_element;

	}else if(cache->LRU_element->next == NULL){//only one element present
		if(cache->LRU_element->file != table_entry->file){
			//printf("one element in LRU\n");
			cache->LRU_tail->next = table_entry;
			table_entry->pre = cache->LRU_tail;
			cache->LRU_tail = cache->LRU_tail->next;
		}
	}else{
		if(inserting){
			cache->LRU_tail->next = table_entry;
			table_entry->pre = cache->LRU_tail;
			cache->LRU_tail = cache->LRU_tail->next;
			return;
		}
		
		if(!strcmp(cache->LRU_element->file->file_name, table_entry->file->file_name)){//special case
			cache->LRU_element = cache->LRU_element->next;
			cache->LRU_element->pre = NULL;
		}else
			table_entry->pre->next = table_entry->next;
		if(!strcmp(cache->LRU_tail->file->file_name, table_entry->file->file_name)){//special case
			//do nothing because its already at the tail
			cache->LRU_tail = cache->LRU_tail->pre;
			cache->LRU_tail->next = NULL;
		}else
			table_entry->next->pre = table_entry->pre;

		cache->LRU_tail->next = table_entry;
		table_entry->pre = cache->LRU_tail;
		cache->LRU_tail = cache->LRU_tail->next;
		table_entry->next = NULL;
	}
	//printf("upgrade_LRU function returns\n");
	return;
}
