#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "3490"
#define MAX_MSG_ACCEPT_BYTES 100

// look for an IPv4/6 host:port in an address structure
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	struct addrinfo hints, *servinfo, *port;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf[MAX_MSG_ACCEPT_BYTES];

	if (argc != 3) {
	    fprintf(stderr, "usage: client hostname 'message'\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    // look up local address information
	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop over available interfaces and try to bind to port
	for(port = servinfo; port != NULL; port = port->ai_next) {
		if ((sockfd = socket(port->ai_family, port->ai_socktype,
				port->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (connect(sockfd, port->ai_addr, port->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}
		break;
	}

	if (port == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

    if (send(sockfd, argv[2], strlen(argv[2])+1, 0) == -1)
        perror("send");

	close(sockfd);

	return 0;
}
