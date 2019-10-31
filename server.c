#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/un.h>

#define SOCK_PATH "socketfile"
#define MAX_USERS 10

static _Atomic int user_count = 0; // look up more about this

typedef struct {
	int userid;
	char name[50];
	int sockfd;
	struct sockaddr_un addr;
} user_t;

user_t *list_of_users[MAX_USERS];

void send_message(char *msg, int userid) {
	for (int i = 0; i < MAX_USERS; i++) {
		if (list_of_users[i] != NULL){
			if (list_of_users[i]->userid != userid) {
				int n = write(list_of_users[i]->sockfd, msg, strlen(msg));

				if (n == -1) {
					perror("write");
					break;
				}
			}
		}
	}
	
}

void *welcome_user(void *new_user) {
	user_t *user = (user_t *) new_user;
	char buffer[100];

	int n = recv(user->sockfd, user->name, 100, 0), flag = 1;

	if (n == -1) {
		printf("Didn't enter name");
		flag = 1;
	}

	else {
		sprintf(buffer, "%s has joined the chatroom.\n", user->name);
		printf("%s", buffer);
		send_message(buffer, user->userid);
	}

	bzero(buffer, 100);

	while(1) {
		n = recv(user->sockfd, buffer, 100, 0);

		if(n == -1) {
			perror("receive");
		}

		else if(n == 0) {
			printf("user left");
		}

		else {
			send_message(buffer, user->userid);
			printf("%s", buffer);
		}
	}
	
	return NULL;
}

int main() {
	int sockfd, newsockfd, len;
	struct sockaddr_un local, remote;
	pthread_t thread;

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sockfd == -1) {
		perror("socket");
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);

	if (bind(sockfd, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(1);
	}

	printf("===WELCOME TO THE CHATROOM===\n");

	while (1) {
		socklen_t remotelen = sizeof(remote);
		newsockfd = accept(sockfd, (struct sockaddr *)&remote, &remotelen);

		if(newsockfd == -1) {
			perror("accept");
			exit(1);
		}

		// Check for maximum users

		if (user_count >= MAX_USERS) {
			printf("FAILED: Maximum users connected\n");
			close(newsockfd);
			continue;
		
		}

		// Set user details

		user_t *new_user = (user_t *) malloc(sizeof(user_t));
		new_user->userid = ++user_count;
		new_user->sockfd = newsockfd;
		new_user->addr = remote;

		// Add user to list of users

		for (int i = 0; i < MAX_USERS; i++) {
			if (list_of_users[i] == NULL) {
				list_of_users[i] = new_user;
				printf("Added new user to index %d", i);
				break;
			}
		}

		// Create a new thread for the user

		pthread_create(&thread, NULL, &welcome_user, (void *) new_user);

		sleep(1);
	}
}