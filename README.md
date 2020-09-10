# Creme: Socket removal notification

Creme is a tiny Linux kernel module that notifies the application of 
complete socket structure removal in the kernel.
This is needed for connection handoff in Prism, to appear in [USENIX
NSDI 2021](https://www.usenix.org/conference/nsdi21).

## Compile and Test
```
make
make test
```

## Usage
1. Open /dev/creme
```
crfd = open("/dev/creme", O_RDWR);
```
2. Create an eventfd
```
evfd = eventfd(0, 0);
```
3. Register an accepted socket and eventfd with ``ioctl()``
```
uint64_t val = ((uint64_t)evfd << 32) | newfd /* accepted socket */;
ioctl(crfd, 0, (unsigned long)&val, sizeof(val)))
```

4. Wait for connection withdrawal
```
struct pollfd pfds[1];
pfds[0].fd = evfd;
pfds[0].events = POLLIN | POLLERR;
poll(pfds, 1, 500);
if (pfds[0].revents & POLLIN)
	eventfd_read(pfds[0].fd, &cnt);
```

See ``server()`` in test.c for more details.
