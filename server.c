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

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
}

int main(void)
{
	int sockfd, activefd;  // listen on sockfd, new connection on activefd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char source[INET6_ADDRSTRLEN];
//    char CLIENT_IP[15] = "107.21.205.69\0";
	int rv;

	memset(&hints, 0, sizeof hints); // make sure the struct is empty
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

    // servinfo now points to a linked list of 1 or more struct addrinfos

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("irssi-notify: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("irssi-notify: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "irssi-notify: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("irssi-notify: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		activefd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (activefd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			source, sizeof source);

        if (strcmp(source, "50.16.219.8") != 0 &&
                strcmp(source, "127.0.0.1") != 0){
            close(activefd);
            continue;
        }                                                                       

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
            char buf[4096];
            if (recv(activefd, buf, 99, 0) == -1) {
                perror("recv");
                exit(1);
            }
            buf[4096] = '\0';
            printf("irssi-notify: recieved '%s'\n", buf);
            if (!fork()){
                setenv("DISPLAY", ":0", 1); // doesn't seem to be doing the trick
                execlp("notify-display",
                        "notify-display", buf, NULL);
            // the first argument to execl is the command, the second is the first argument
            // passed to the program ($0), customarily the evocation of the command
            }
			close(activefd);
			exit(0);
		}
		close(activefd);
	}

	return 0;
}
