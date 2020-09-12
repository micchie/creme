#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/net.h> // sockfd_lookup()
#include <linux/socket.h>
#include <net/sock.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/eventfd.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michio Honda");
MODULE_DESCRIPTION("Connection removal notification module.");

#define DEVICE_NAME “creme”

/* Prototypes for device functions */

struct creme_sk_data {
	struct eventfd_ctx *ctx;
	void (*saved_destruct)(struct sock *sk);
};

static int
creme_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
creme_release(struct inode *inode, struct file *file)
{
	return 0;
}

static void
creme_notify(struct sock *sk)
{
	struct creme_sk_data *csd = (struct creme_sk_data *)sk->sk_user_data;
	struct eventfd_ctx *ctx = (struct eventfd_ctx *)csd->ctx;

	sk->sk_destruct = csd->saved_destruct;
	eventfd_signal(ctx, 1);
	eventfd_ctx_put(ctx);
	//printk(KERN_INFO "eventfd notified\n");
	kfree(csd);
	sk->sk_destruct(sk);
}

static long
creme_unlocked_ioctl(struct file *file, u_int cmd, u_long data /* arg */)
{
	struct eventfd_ctx *ctx;
	struct socket *socket;
	struct sock *sk;
	uint64_t fds;
	int eventfd, sockfd, error = 0;
	struct creme_sk_data *csd;

	if (copy_from_user(&fds, (void *)data, sizeof(fds))) {
		return -EFAULT;
	}
	sockfd = (int)(fds & 0xffffffff);
	eventfd = (int)((fds & 0xffffffff00000000) >> 32);
	ctx = eventfd_ctx_fdget(eventfd);

	socket = sockfd_lookup(sockfd, &error); // error is negative
	if (!socket) {
		printk(KERN_ERR "failed to lookup socket\n");
		return error;
	}
	sk = socket->sk;
	lock_sock(sk);
	csd = kmalloc(sizeof(*csd), GFP_ATOMIC | __GFP_ZERO);
	if (IS_ERR(csd)) {
		error = -ENOMEM;
		eventfd_ctx_put(ctx);
	} else {
		csd->ctx = ctx;
		csd->saved_destruct = sk->sk_destruct;
		sk->sk_user_data = csd;
		sk->sk_destruct = creme_notify;
	}
	release_sock(sk);
	sockfd_put(socket);

	return error;
}

/* This structure points to all of the device functions */
static struct file_operations creme_fops = {
	.owner = THIS_MODULE,
	.open = creme_open,
	.unlocked_ioctl = creme_unlocked_ioctl,
	.release = creme_release
};

static struct miscdevice creme_miscdev = 
{
	MISC_DYNAMIC_MINOR,
	"creme",
	&creme_fops
};

static struct miscdevice *creme_dev;

static int __init
creme_init(void)
{
	int error = misc_register(&creme_miscdev);
	if (error) {
		printk(KERN_ERR "%s misc_register() failed\n", __FUNCTION__);
	}
	creme_dev = &creme_miscdev;
	printk(KERN_INFO "loaded creme\n");
	return 0;
}

static void __exit
creme_exit(void)
{
	if (creme_dev)
		misc_deregister(creme_dev);
	printk(KERN_INFO "unloaded creme\n");
}

module_init(creme_init);
module_exit(creme_exit);
