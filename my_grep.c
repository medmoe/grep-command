#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

//initialize constants
const int MAX_NUMBER_OF_FILES = 10000;
const int SIZE_ARRAY = 1000;

char * PATTERN;
int total_number_of_searched_lines = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//each thread created will run this function
void * searchfile(void * file_name){
	char * buffer; 
	size_t buffer_length;
	ssize_t read;
	int number_of_lines = 0;
	FILE * fp;

	//allocate memory for reading lines from file
	buffer = (char *)malloc(sizeof(char) * SIZE_ARRAY);
	if(buffer == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	//open file for reading
	fp = fopen((char *)file_name , "r");
	if (fp == NULL){
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	//read file line by line
	while((read = getline(&buffer, &buffer_length, fp)) != -1){
		//protect the critical section using mutex locks
		pthread_mutex_lock(&mutex);
		total_number_of_searched_lines = total_number_of_searched_lines + 1;
		pthread_mutex_unlock(&mutex);
		//end of critical section
		if(read > SIZE_ARRAY){
			printf("text line is too long");
			exit(EXIT_FAILURE);
		}
		if(strstr(buffer, PATTERN) != NULL ){
			printf("%s: %s\n", (char *)file_name, buffer);
			number_of_lines = number_of_lines + 1;
		}
	}
	printf("--%s has %d matched lines\n", (char *)file_name, number_of_lines);
	fclose(fp);
	free(buffer);
	pthread_exit((void *)(intptr_t) number_of_lines);
}

int main (int argc, char * argv[]){
	//verify that the given arguments to the command are within the range
	if(argc < 3 || argc > MAX_NUMBER_OF_FILES){
		perror("no given arguments");
		exit(EXIT_FAILURE);
	}

	pthread_t * thread_ptr; // a pointer to a thread 
	int ret; // indicates the return value by each thread
	void * read_lines; // indicates the number of read lines by each thread
	int total = 0; // indicates the total number of lines in which PATTERN was found

	//allocate memory for threads and pattern
	PATTERN = (char *)malloc(sizeof(char) * strlen(argv[1]));
	if(PATTERN == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	strcpy(PATTERN, argv[1]); // get the pattern
	thread_ptr = (pthread_t *)malloc(sizeof(pthread_t) * (argc - 2));
	if(thread_ptr == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	//create a thread for every given file
	for(int i = 0; i < argc-2; i++ ){
		ret = pthread_create(&thread_ptr[i], NULL, searchfile, (void *)(argv[i+2]));
		if(ret){
			errno = ret;
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
	//merge all threads
	for(int i = 0; i < argc-2; i++){
		ret = pthread_join(thread_ptr[i], &read_lines);
		if (ret){
			errno = ret;
			perror("pthread_join");
			exit(EXIT_FAILURE);
		}
		printf("Thread %d returned with value: %ld\n", i, (intptr_t)read_lines);
		total = total + (intptr_t)read_lines;
	}
	printf("Total of %d matched lines among total of %d lines scanned\n", total, total_number_of_searched_lines);

	free(PATTERN);
	free(thread_ptr);
	exit(EXIT_SUCCESS);
}