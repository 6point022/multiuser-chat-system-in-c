#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define SOCK_PATH "socketfile"

int sockfd;

void *sending_msg(void *value) {
	char msg[100];
	char buffer[150];
	char *name = (char *) value;
	
	while(1) {
		printf("> ");
		fflush(stdout);
		fgets(msg, 100, stdin);

		if (strcmp(msg, "exit") == 0) {
			break;
		}

		else {
			sprintf(buffer, "%s: %s\n", name, msg);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(msg, 100);
		bzero(buffer, 150);
	}

	return NULL;
}

void *receiving_msg(void *value) {
	char msg[100];

	while(1) {
		int n = recv(sockfd, msg, 100, 0);

		if (n == 0) {
			break;
		}

		else if (n == -1) {
			perror("receive");
			exit(1);
		}

		else {
			printf("%s", msg);
			printf("> ");
  			fflush(stdout);
		}
	}


	return NULL;
}

int main() {
	int len;
	struct sockaddr_un server;
	char name[50];

	printf("Enter your name: ");
	fgets(name, 50, stdin);

	// TODO: Add a check for name

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sockfd == -1) {
		perror("socket");
		exit(1);
	}

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, SOCK_PATH);

	len = strlen(server.sun_path) + sizeof(server.sun_family);

	int n = connect(sockfd, (struct sockaddr *)&server, len);

	printf("Connecting to server...\n\n");

	if (n == -1) {
		perror("connect");
		exit(1);
	}

	printf("Connected to server\n");

	send(sockfd, name, 50, 0);

	printf("===WELCOME TO THE CHATROOM===\n");

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

	// char msg[100];

	// printf("> ");
	// fgets(msg, 100, stdin);

	// while (!feof(stdin)) {
	// 	n = send(sockfd, msg, strlen(msg), 0);

	// 	if (n == -1) {
	// 		perror("send");
	// 		exit(1);
	// 	}

	// 	n = recv(s, msg, 100, 0);

	// 	if(n == -1) {
	// 		perror("receive");
	// 		exit(1);
	// 	}

	// 	else {
	// 		msg[n] = '\0';
	// 		printf("msg: %s", msg);
	// 	}

	// }
}