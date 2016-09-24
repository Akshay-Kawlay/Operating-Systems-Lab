#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <math.h>
#include <string.h>
#include <ctype.h>

long TABLE_SIZE;

unsigned long
hash(char *str);/*hash function declaration*/

struct wc*
create_hash(struct wc* wc, long size);/*creates empty hash table*/

struct wc {
	/* you can define this struct to have whatever fields you want. */
	struct element** hashtable;
};

struct element {
	char *word;
 /*for counting same words in the array*/
	unsigned long count;
};

struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;
	long table_size = size;/*((long)sqrt(size)+2);*/
	TABLE_SIZE = table_size;
	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);
	wc->hashtable = NULL;
	wc = create_hash(wc,size);
	
	/*parse the word_array*/
	long i=0;
	int k = 0, word_insert = 0;
	char buff[500];
	/*memset(buff, '\0', sizeof(buff)); assign all entries of buff to NULL*/

	struct element **hasht = wc->hashtable; /* For easier writing */
	for(i = 0; i<size; i++){
		if(!isspace(word_array[i])){
			buff[k] = word_array[i];
			k++;	
		}else{
			buff[k] = '\0';
			k=0;
			word_insert = 1;
		}
		if(buff[0] == '\0') /*to eliminate cases of 2 continuous spaces*/
			word_insert = 0;
		if(word_insert){
			word_insert = 0;
			unsigned long key = hash(buff) % table_size;
			/*printf("%s , %d \n", buff, (int)strlen(buff));*/
			char *word = (char *)malloc(sizeof(char)*(strlen(buff)+1));
			word = strcpy(word,buff);
			
			if(hasht[key] == NULL){
				struct element *ele = (struct element *)malloc(sizeof(struct element));
				ele->word = word;
				ele->count = 1;
				hasht[key] = ele;

			}else if(hasht[key] != NULL && strcmp(hasht[key]->word, word) == 0){
				hasht[key]->count++;
				free(word);	
			}else{/*collision*/
				long new_key = key;
				int dublicate = 0;

				while(hasht[new_key] != NULL){
					if(strcmp(hasht[new_key]->word, word) == 0){
						hasht[new_key]->count++;
						free(word);
						dublicate = 1;
						break;
					}
					if(dublicate)
						break;
					new_key = (new_key*new_key) % table_size;
				}

				if(!dublicate){
					struct element *ele = (struct element *)malloc(sizeof(struct element));
					ele->word = word;
					ele->count = 1;
					hasht[new_key] = ele;
				}
			}


		}
	}

	return wc;
}

void
wc_output(struct wc *wc)
{
	struct element **hasht = wc->hashtable;
	unsigned long i;
	for(i = 0; i < TABLE_SIZE; i++){
		if(hasht[i] != NULL){
			printf("%s:%lu\n", hasht[i]->word, hasht[i]->count);
		}		

	}	
}

void
wc_destroy(struct wc *wc)
{
	struct element **hasht = wc->hashtable;
	unsigned long i;
	for(i = 0; i < TABLE_SIZE; i++){
		if(hasht[i] != NULL){
			free(hasht[i]->word);
			free(hasht[i]);
		}
	}
	free(hasht);
	free(wc);
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

        long table_size = size;/*((long)sqrt(size)+2);*/
	TABLE_SIZE = table_size;
	wc->hashtable = (struct element **)malloc(sizeof(struct element *) * table_size);
	int i;
	assert(wc->hashtable!=NULL);
	for(i=0; i<table_size; i++){
		wc->hashtable[i] = NULL;
	}

	return wc;
}

/*_________________________________________________*/



