#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>

void
server()
{
	int fd, newfd, evfd, crfd, n;
	uint64_t val;
	struct sockaddr_in sin, c;
	int wstatus = 0;
	struct pollfd pfds[1];
	eventfd_t cnt;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) {
		perror("socket");
		return;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
		perror("setsockopt");
		return;
	}
	sin.sin_family = AF_INET;
	sin.sin_port = htons(50000);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(fd, (struct sockaddr *)&sin, sizeof(sin))) {
		perror("bind");
		close(fd);
		goto error_wait;
	}
	if (listen(fd, 1) < 0) {
		perror("listen");
		close(fd);
		goto error_wait;
	}
	fprintf(stdout, "listen done\n");
	newfd = accept(fd, (struct sockaddr *)&c, &(socklen_t){sizeof(c)});
	if (newfd < 0) {
		perror("accept");
		close(fd);
		goto error_wait;
	}
	fprintf(stdout, "accept done\n");

	evfd = eventfd(0, 0);
	fprintf(stdout, "eventfd created\n");
	crfd = open("/dev/creme", O_RDWR);
	if (crfd < 0) {
		perror("open");
		close(evfd);
		goto error_wait;
	}
	fprintf(stdout, "/dev/creme opened\n");
	val = ( (uint64_t)evfd << 32) | newfd;
	if (ioctl(crfd, 0, (unsigned long)&val, sizeof(val))) {
		perror("ioctl");
	}
	fprintf(stdout, "ioctl done\n");

	close(newfd);
	fprintf(stdout, "closed newfd\n");

	for (n = 0; n < 10; n++) {
		int nev;

		pfds[0].fd = evfd;
		pfds[0].events = POLLIN | POLLERR;
		nev = poll(pfds, 1, 500);
		if (nev < 0) {
			perror("poll");
		} else if (nev == 0) {
			continue;
		}
		if (pfds[0].revents & POLLIN) {
			fprintf(stdout, "eventfd POLLIN\n");
			break;
		} else if (pfds[0].revents & POLLERR) {
			fprintf(stdout, "eventfd POLLERR\n");
			break;
		}
	}
	if (n == 10) {
		fprintf(stderr, "eventfd has never fired\n");
	}
error_wait:
	wait(&wstatus);
	fprintf(stdout, "wait done\n");
	return;
}

void
client()
{
	int fd;
	struct sockaddr_in sin;
       
	usleep(1000); // wait for server setup

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) {
		perror("socket");
		return;
	}
	sin.sin_family = AF_INET;
	sin.sin_port = htons(50000);
	inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);
	if (connect(fd, (struct sockaddr *)&sin, sizeof(sin))) {
		perror("connect");
		return;
	}
	fprintf(stdout, "connect done\n");
}

int
main()
{
	pid_t pid = fork();
	if (pid > 0)
		server();
	else
		client();
	return 0;
}
