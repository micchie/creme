#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>

#define MAXCONN	10
#define ARRAYSIZ(a)	(sizeof(a) / sizeof(a[0]))

void
server()
{
	int fd, crfd, n, epfd, maxretry = 0;
	struct sockaddr_in sin;
	int wstatus = 0;
	struct epoll_event ev;

	crfd = open("/dev/creme", O_RDWR);
	if (crfd < 0) {
		perror("open");
		return;
	}
	fprintf(stdout, "/dev/creme opened\n");

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
	if (listen(fd, MAXCONN) < 0) {
		perror("listen");
		close(fd);
		goto error_wait;
	}
	fprintf(stdout, "listen done\n");

	epfd = epoll_create1(EPOLL_CLOEXEC);
	if (epfd < 0) {
		perror("epoll_create1");
		close(fd);
		goto error_wait;
	}
	memset(&ev, 0, sizeof(ev));
	ev.events = POLLIN;
	ev.data.fd = fd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev)) {
		perror("epoll_ctl");
		close(epfd);
		close(fd);
		goto error_wait;
	}
	for (n = 0; n <= MAXCONN && maxretry < MAXCONN * 10; n++) {
		struct epoll_event evts[MAXCONN*4];
		int nfds, i;

		maxretry++;

		nfds = epoll_wait(epfd, evts, ARRAYSIZ(evts), 1000);
		if (nfds < 0) {
			perror("epoll_wait");
			break;
		}
		fprintf(stdout, "epoll_wait %d events\n", nfds);
		for (i = 0; i < nfds; i++) {
			int j;
			int infd = evts[i].data.fd;

			if (infd == fd) {
				struct epoll_event newev;
				int newfd, evntfd;
				uint64_t val;

				newfd = accept(fd, (struct sockaddr *)&sin,
					       &(socklen_t){sizeof(sin)});
				if (newfd < 0) {
					perror("accept");
					/* XXX */
				}
				evntfd = eventfd(0, 0);
				val = ((uint64_t)evntfd << 32) | newfd;
				if (ioctl(crfd, 0, (unsigned long)&val,
				    sizeof(val))) {
					perror("ioctl");
				}
				fprintf(stdout, "  ioctl (newfd %d eventfd %d) done\n", newfd, evntfd);

				memset(&newev, 0, sizeof(newev));
				newev.events = POLLIN;
				newev.data.fd = evntfd;
				epoll_ctl(epfd, EPOLL_CTL_ADD, evntfd, &newev);

				close(newfd);
			} else {
				eventfd_t cnt;

				eventfd_read(infd, &cnt);
				fprintf(stdout,
					"  eventfd %d POLLIN counter %lu\n",
					infd, cnt);
				close(infd);
				n++;
			}
		}
	}
	close(epfd);

error_wait:
	wait(&wstatus);
	fprintf(stdout, "wait done\n");
	return;
}

void
client()
{
	int i;
	int fds[MAXCONN];
       
	usleep(100); // wait for server setup

	for (i = 0; i < MAXCONN; i++) {
		int fd;
		struct sockaddr_in sin;

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
		fds[i] = fd;
	}
	fprintf(stdout, "connected %d fds\n", i);
	for (i = 0; i < MAXCONN; i++) {
		close(fds[i]);
	}
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
