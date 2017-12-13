/* morse.c
Modified from the code on
http://homepage3.nifty.com/rio_i/lab/driver24/00201chardev.html

As shown in the code, this code can be distributed under GNU GPL.
*/
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/time.h>

static inline unsigned ccnt_read(void)
{
	unsigned cc;
	asm volatile("mrc p15, 0, %0, c15, c12, 1" : "=r" (cc));
	return cc;
}

#include "parse.h"

MODULE_AUTHOR("Yoshiaki Jitsukawa");
MODULE_DESCRIPTION("Telegraph Key Handler");
MODULE_LICENSE("GPL");

static int devmajor = 0x0123;
static char* devname  = "morse";
static char* msg = "module [morse.o]";

#define MAXBUF 32
static unsigned char devbuf[MAXBUF];
static int buf_pos;
static int access_num;
static spinlock_t spn_lock;

static int irqNumber;
static unsigned gpioButton = 2;

static int morse_open(struct inode* inode, struct file* filp);
static ssize_t morse_write(struct file* filp, const char* buf, size_t count, loff_t* pos );
static ssize_t morse_read( struct file* filp, char* buf, size_t count, loff_t* pos );
static int morse_release( struct inode* inode, struct file* filp );
int unregister_morse( unsigned int major, const char* name );

static irq_handler_t morse_irq_handler(unsigned irq, void *dev_id, struct pt_regs *regs);

static DECLARE_WAIT_QUEUE_HEAD(q);

static unsigned cc_last, cc_current, cc_diff;
static struct timespec ts_last, ts_current, ts_diff;
static int button_last;

static struct file_operations morse_fops =
{
	owner   : THIS_MODULE,
	read    : morse_read,
	write   : morse_write,
	open    : morse_open,
	release : morse_release,
};

static int morse_open(struct inode* inode, struct file* filp){
	int result;

	//printk( KERN_INFO "%s : open()  called\n", msg );

	spin_lock( &spn_lock );

	if(access_num){
		spin_unlock( &spn_lock );
		printk( KERN_ERR "Busy\n" );
		return -EBUSY;
	}

	access_num++;

	spin_unlock( &spn_lock );

	init_keymap();

	asm volatile("mcr p15, 0, %0, c15, c12, 0" :: "r" (7));

	gpio_request(gpioButton, "sysfs");
	gpio_direction_input(gpioButton);
	gpio_export(gpioButton, false);

//	getnstimeofday(&ts_last);
//	ts_diff = timespec_sub(ts_last, ts_last);
	cc_last = ccnt_read();
	button_last = !gpio_get_value(gpioButton);

	irqNumber = gpio_to_irq(gpioButton);
	//printk( KERN_INFO "IRQ %d\n", irqNumber);

	result = request_irq(irqNumber, (irq_handler_t)morse_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "morse_button_handler", NULL);

	return result;
}

static int morse_release( struct inode* inode, struct file* filp )
{
	//printk( KERN_INFO "%s : close() called\n", msg );

	free_irq(irqNumber, NULL);
	gpio_unexport(gpioButton);
	gpio_free(gpioButton);

	asm volatile("mcr p15, 0, %0, c15, c12, 0" :: "r" (0));

	spin_lock( &spn_lock );
	access_num--;
	spin_unlock( &spn_lock );

	return 0;
}

static ssize_t morse_read( struct file* filp, char* buf, size_t count, loff_t* pos )
{
	int copy_len;
	int i;
	//long a;
	int result;
	unsigned long flags;

	//printk( KERN_INFO "%s : read()  called\n", msg );

	result = wait_event_interruptible(q, buf_pos > 0);
	if (result)
		return result;

	local_irq_save(flags);
	if(count > buf_pos)
		copy_len = buf_pos;
	else
		copy_len = count;

	if(copy_to_user( buf, devbuf, copy_len )) {
		//printk( KERN_INFO "%s : copy_to_user failed\n", msg );
		local_irq_restore(flags);
		return -EFAULT;
	}

	*pos += copy_len;

	for (i = copy_len;i < buf_pos;i++)
		devbuf[i - copy_len] = devbuf[i];

	local_irq_restore(flags);

	buf_pos -= copy_len;
	//printk( KERN_INFO "%s : buf_pos = %d\n", msg, buf_pos );

	return copy_len;
}

static ssize_t morse_write(struct file* filp, const char* buf, size_t count, loff_t* pos )
{
	int copy_len;

	//printk( KERN_INFO "%s : write() called\n", msg );

	if (buf_pos == MAXBUF){
		//printk( KERN_INFO "%s : no space left\n", msg );
		return -ENOSPC;
	}

	if(count > ( MAXBUF - buf_pos ))
		copy_len = MAXBUF - buf_pos;
	else
		copy_len = count;

	if(copy_from_user( devbuf + buf_pos, buf, copy_len )){
		//printk( KERN_INFO "%s : copy_from_user failed\n", msg );
		return -EFAULT;
	}

	*pos += copy_len;
	buf_pos += copy_len;

	//printk( KERN_INFO "%s : buf_pos = %d\n", msg, buf_pos );
	return copy_len;
}

static irq_handler_t morse_irq_handler(unsigned irq, void *dev_id, struct pt_regs *regs)
{
	int ms;
	int button;

//	getnstimeofday(&ts_current);
//	ts_diff = timespec_sub(ts_current, ts_last);

	cc_current = ccnt_read();
//	ms = ts_diff.tv_nsec / 1000000 + ts_diff.tv_sec * 1000;

	cc_diff = cc_current - cc_last;
	ms = cc_diff / 16384;

	if (ms < 6)
		return (irq_handler_t)IRQ_HANDLED;

//	ts_last = ts_current;
	cc_last = cc_current;

	button = !gpio_get_value(gpioButton);
	if (button == button_last) {
		return (irq_handler_t)IRQ_HANDLED;
	}
	button_last = button;

	button = !button;
//	printk( KERN_CRIT "button %d ms %d\n", button, (int)ms);

	if (buf_pos < MAXBUF)
	{
		char c = morse_parse(button, ms);
	
		if (c) {
			devbuf[buf_pos++] = c;
			wake_up_interruptible(&q);
		}
	}

	return (irq_handler_t)IRQ_HANDLED;
}

int init_module( void )
{
	if(register_chrdev(devmajor, devname, &morse_fops)){
		printk( KERN_INFO "%s : register_chrdev failed\n" );
		return -EBUSY;
	}

	spin_lock_init( &spn_lock );
	printk( KERN_INFO "%s : loaded  into kernel\n", msg );

	return 0;
}

void cleanup_module( void )
{
	unregister_chrdev( devmajor, devname );
	printk( KERN_INFO "%s : removed from kernel\n", msg );
}
