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
#define MAX_MSG_SIZE 100
#define MAX_BUFFER_SIZE 111
#define MAX_NAME_SIZE 11

static int user_count = 0; // look up more about this

// Create a structure that represents a user

typedef struct {
	int userid;
	char name[MAX_NAME_SIZE];
	int sockfd;
	struct sockaddr_un addr;
} user_t;

// An array of all users

user_t *list_of_users[MAX_USERS];

// Function to send message to users

void send_message(char *msg, int userid, int msg_type) {

	// Message type 1 is sent by a user and type 2 is if user joins or leaves

	if (msg_type == 2) {
		int start_index = 0;

		for (int i = 0; msg[i] != '\0'; i++) {
			if (msg[i] == ':') {
				start_index = i + 2;
				break;
			}
		}

		char receiver_name[MAX_NAME_SIZE];

		if (msg[start_index] == '@') {
			int j = 0;

			for (int i = start_index + 1; msg[i] != ' '; i++) {
				receiver_name[j++] = msg[i];
			}

			receiver_name[j] = '\0';
		}

		else {
			sprintf(receiver_name, "default");
		}

		for (int i = 0; i < MAX_USERS; i++) {
			if (list_of_users[i] != NULL){
				if (list_of_users[i]->userid != userid && (strcmp(receiver_name, list_of_users[i]->name) == 0 || strcmp(receiver_name, "all") == 0)) {
					int n = send(list_of_users[i]->sockfd, msg, strlen(msg), 0);

					if (n == -1) {
						perror("send");
						break;
					}
				}
			}
		}
	}

	else {
		for (int i = 0; i < MAX_USERS; i++) {
			if (list_of_users[i] != NULL){
				if (list_of_users[i]->userid != userid) {
					int n = send(list_of_users[i]->sockfd, msg, strlen(msg), 0);

					if (n == -1) {
						perror("send");
						break;
					}
				}
			}
		}
	}


}

void *welcome_user(void *new_user) {

	// Function for every user thread

	user_t *user = (user_t *) new_user;
	char buffer[MAX_BUFFER_SIZE];

	// Receive the user's name using the recv blocking call

	int n = recv(user->sockfd, user->name, MAX_NAME_SIZE, 0);

	if (n == -1) {
		printf("Didn't enter name");
	}

	else {
		sprintf(buffer, "%s has joined the chatroom.\n", user->name);
		printf("%s", buffer);
		send_message(buffer, user->userid, 1);
	}

	bzero(buffer, MAX_BUFFER_SIZE);

	char exit_condition[20];
	sprintf(exit_condition, "%s: quit\n", user->name);

	// Start the infinite loop to receive messages from user

	while(1) {
		n = recv(user->sockfd, buffer, MAX_BUFFER_SIZE, 0);
	
		if(n == -1) {
			perror("receive");
			break;
		}

		else if(n == 0 || !strcmp(buffer, exit_condition)) {
			sprintf(buffer, "%s left.\n", user->name);
			printf("%s", buffer);
			send_message(buffer, user->userid, 1);

			// Remove user from the list on quitting

			for(int i=0; i < MAX_USERS; ++i){
				if(list_of_users[i]) {
					if(list_of_users[i]->userid == user->userid) {
						list_of_users[i] = NULL;
						break;
					}
				}
			}

			user_count--;

			break;
		}

		else {
			send_message(buffer, user->userid, 2);
			printf("%s", buffer);
		}

		bzero(buffer, MAX_BUFFER_SIZE);
	}
	
	return NULL;
}

int main() {
	int sockfd, newsockfd, len;
	struct sockaddr_un local, remote;
	pthread_t thread;

	// Create a socket file descriptor

	sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sockfd == -1) {
		perror("socket");
	}

	// Add information about the Unix domain address

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCK_PATH);
	unlink(local.sun_path);
	len = strlen(local.sun_path) + sizeof(local.sun_family);

	// Bind the socket to an address

	if (bind(sockfd, (struct sockaddr *)&local, len) == -1) {
		perror("bind");
		exit(1);
	}

	// Listen to incoming connections from client programs

	if (listen(sockfd, 5) == -1) {
		perror("listen");
		exit(1);
	}

	printf("\n\n___WELCOME TO THE CHAT___\n\n");

	while (1) {
		socklen_t remotelen = sizeof(remote);

		// We create a new socked fd which is connected to the client

		newsockfd = accept(sockfd, (struct sockaddr *)&remote, &remotelen);

		if(newsockfd == -1) {
			perror("accept");
			exit(1);
		}

		// Check for maximum users

		if (user_count >= MAX_USERS) {
			printf("Maximum users connected\n");
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
				break;
			}
		}

		// Create a new thread for the user

		pthread_create(&thread, NULL, &welcome_user, (void *) new_user);

	}
}