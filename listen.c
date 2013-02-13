#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <libnotify/notify.h>

#define PORT "3490"
#define QUEUE_LENGTH 10

// a function to reap future child processes
void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// look for an IPv4 host:port in an address structure
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
}

int main(void)
{
    // new connections on activefd, always listening on sockfd
	int sockfd, activefd;
	struct addrinfo hints, *servinfo, *port;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	struct sigaction sa;
	char source[INET6_ADDRSTRLEN];
	int yes=1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

    // look up local address information
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop over available interfaces and try to bind to port
	for (port = servinfo; port != NULL; port = port->ai_next) {
		if ((sockfd = socket(port->ai_family, port->ai_socktype,
				port->ai_protocol)) == -1) {
			perror("irssi-notify: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, port->ai_addr, port->ai_addrlen) == -1) {
			close(sockfd);
			perror("irssi-notify: bind");
			continue;
		}
		break;
	}

	if (port == NULL)  {
		fprintf(stderr, "irssi-notify: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	if (listen(sockfd, QUEUE_LENGTH) == -1) {
		perror("listen");
		exit(1);
	}

    // bind sigchld_handler to reap future child processes
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("irssi-notify: waiting for connections...\n");

	while (1) {
		sin_size = sizeof their_addr;
		activefd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (activefd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			source, sizeof source);

        // whitelist allowed
        if (strcmp(source, "50.16.219.8") != 0 &&
                strcmp(source, "127.0.0.1") != 0){
            close(activefd);
            continue;
        }                                                                       

        // start a child process to handle notifications
		if (!fork()) {
            // the child process doesn't need the listener
			close(sockfd);
            char buf[4096];
            if (recv(activefd, buf, 99, 0) == -1) {
                perror("recv");
                exit(1);
            }
            buf[4096] = '\0';
            printf("irssi-notify: recieved '%s'\n", buf);
            notify_init("irssi-notify");
            NotifyNotification * notification =
                notify_notification_new("", buf, "irssi-notify");
            notify_notification_show(notification, NULL);
			close(activefd);
			exit(0);
		}
		close(activefd);
	}

	return 0;
}
