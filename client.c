#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <unistd.h>

#define SOCK_PATH "socketfile"
#define MAX_MSG_SIZE 100
#define MAX_BUFFER_SIZE 120
#define MAX_NAME_SIZE 11

int sockfd;

void *sending_msg(void *value) {
	char msg[MAX_MSG_SIZE];
	char buffer[MAX_BUFFER_SIZE];
	char *name = (char *) value;
	
	while(1) {
		printf("> ");
		fflush(stdout);
		fgets(msg, MAX_MSG_SIZE, stdin);

		if (strcmp(msg, "quit") == 0) {
			close(sockfd);
			printf("Quitting the chatroom...");
			exit(0);
		}

		else {
			sprintf(buffer, "%s: %s", name, msg);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(msg, MAX_MSG_SIZE);
		bzero(buffer, MAX_BUFFER_SIZE);
	}

	return NULL;
}

void *receiving_msg(void *value) {
	char buffer[MAX_BUFFER_SIZE];

	while(1) {
		int n = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);

		if (n == 0) {
			break;
		}

		else if (n == -1) {
			perror("receive");
			exit(1);
		}

		else {
			printf("%s", buffer);
			printf("> ");
  			fflush(stdout);
		}

		bzero(buffer, MAX_BUFFER_SIZE);
	}

	return NULL;
}

int main() {
	int len;
	struct sockaddr_un server;
	char name[MAX_NAME_SIZE];

	// Ask for a valid username

	printf("Enter your name (3 - 10 characters): ");
	fgets(name, MAX_NAME_SIZE, stdin);
	// printf("%s%d", name, (int)strlen(name));

	// Remove trailing newline from the name

	for (int i = 0; name[i] != '\0'; i++) {
		if (name[i] == '\n') {
			name[i] = '\0';
			break;
 		}
	}

	while(strlen(name) < 3 || strlen(name) > 10) {
		printf("Please enter a valid name (3 - 10 characters): ");
		bzero(name, MAX_NAME_SIZE);
		fgets(name, MAX_NAME_SIZE, stdin);

			// Remove trailing newline from the name

		for (int i = 0; name[i] != '\0'; i++) {
			if (name[i] == '\n') {
				name[i] = '\0';
				break;
			}
		}
	}

	// Create socket fd

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sockfd == -1) {
		perror("socket");
		exit(1);
	}

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, SOCK_PATH);

	len = strlen(server.sun_path) + sizeof(server.sun_family);

	int n = connect(sockfd, (struct sockaddr *)&server, len);

	printf("\nConnecting to server...\n\n");

	if (n == -1) {
		perror("connect");
		exit(1);
	}

	printf("Connected to server\n\n");

	send(sockfd, name, MAX_NAME_SIZE, 0);

	printf("\n\n___WELCOME TO THE CHAT___\n\n");
	printf("Do '@[name] [your message]' to send to one person\n\nDo '@all [your message]' to send to everyone\n\nEnter 'quit' to exit chatroom\n\n");

	pthread_t sending_thread, receiving_thread;

	n = pthread_create(&sending_thread, NULL, (void *) sending_msg, name);

	if (n  < 0) {
		perror("thread creation 1");
		exit(1);
	}

	n = pthread_create(&receiving_thread, NULL, (void *) receiving_msg, NULL);

	if (n  < 0) {
		perror("thread creation 2");
		exit(1);
	}

	while(1) {

	}

	return 0;
}