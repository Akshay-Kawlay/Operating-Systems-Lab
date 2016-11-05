#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>

void *stub(void *sv);

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
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fills data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	/* reads file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	ret = request_readfile(rq);
	if (!ret)
		goto out;
	/* sends file to client */
	request_sendfile(rq);
out:
	request_destroy(rq);
	file_data_free(data);
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
	if (max_requests>0){
		
		
	}	


	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */
	if (nr_threads > 0){
		
	}
	return sv;
}

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
