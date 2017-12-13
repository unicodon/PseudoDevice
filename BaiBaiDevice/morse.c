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
static int num;

static int morse_open(struct inode* inode, struct file* filp);
static ssize_t morse_write(struct file* filp, const char* buf, size_t count, loff_t* pos );
static ssize_t morse_read( struct file* filp, char* buf, size_t count, loff_t* pos );
static int morse_release( struct inode* inode, struct file* filp );
int unregister_morse( unsigned int major, const char* name );

static irq_handler_t morse_irq_handler(unsigned irq, void *dev_id, struct pt_regs *regs);

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

	printk( KERN_CRIT "%s : open()  called\n", msg );

	spin_lock( &spn_lock );

	if(access_num){
		spin_unlock( &spn_lock );
		printk( KERN_ERR "Busy\n" );
		return -EBUSY;
	}

	access_num++;

	spin_unlock( &spn_lock );
	gpio_request(gpioButton, "sysfs");
	gpio_direction_input(gpioButton);
	gpio_export(gpioButton, false);
	printk( KERN_CRIT "Button state is %d\n", gpio_get_value(gpioButton));

	irqNumber = gpio_to_irq(gpioButton);
	printk( KERN_CRIT "IRQ %d\n", irqNumber);

	result = request_irq(irqNumber, (irq_handler_t)morse_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "morse_button_handler", NULL);

	return result;
}

static int morse_release( struct inode* inode, struct file* filp )
{
	printk( KERN_CRIT "%s : close() called\n", msg );

	free_irq(irqNumber, NULL);
	gpio_unexport(gpioButton);
	gpio_free(gpioButton);

	spin_lock( &spn_lock );
	access_num--;
	spin_unlock( &spn_lock );

	return 0;
}

static ssize_t morse_read( struct file* filp, char* buf, size_t count, loff_t* pos )
{
#if 1
	if (!count) {
		return 0;
	}
	char c = gpio_get_value(gpioButton) ? ' ' : ' ';
	if(copy_to_user( buf, &c, 1)) {
		printk( KERN_CRIT "%s : copy_to_user failed\n", msg );
		return -EFAULT;
	}
	*pos += 1;
	return 1;
#else
	int copy_len;
	int i;
	long a;

	printk( KERN_CRIT "%s : read()  called\n", msg );

	if(count > buf_pos)
		copy_len = buf_pos;
	else
		copy_len = count;

	char nbuf[MAXBUF];
	a = 0;
	kstrtol(devbuf,0,&a);
	a *= 2;

	if(a < 1000000 && a > -1000000)
		sprintf(nbuf,"%d\n",a);
	else
		sprintf(nbuf,"%d\n",0);

	int len;
	len = strlen(nbuf);

	if(copy_to_user( buf, nbuf,len )) {
		printk( KERN_CRIT "%s : copy_to_user failed\n", msg );
		return -EFAULT;
	}

	*pos += copy_len;

	for (i = copy_len;i < buf_pos;i++)
		devbuf[i - copy_len] = devbuf[i];

	buf_pos -= copy_len;
	printk( KERN_CRIT "%s : buf_pos = %d\n", msg, buf_pos );

	return copy_len;
#endif
}

static ssize_t morse_write(struct file* filp, const char* buf, size_t count, loff_t* pos )
{
	int copy_len;

	printk( KERN_CRIT "%s : write() called\n", msg );

	if (buf_pos == MAXBUF){
		printk( KERN_CRIT "%s : no space left\n", msg );
		return -ENOSPC;
	}

	if(count > ( MAXBUF - buf_pos ))
		copy_len = MAXBUF - buf_pos;
	else
		copy_len = count;

	if(copy_from_user( devbuf + buf_pos, buf, copy_len )){
		printk( KERN_CRIT "%s : copy_from_user failed\n", msg );
		return -EFAULT;
	}

	*pos += copy_len;
	buf_pos += copy_len;

	printk( KERN_CRIT "%s : buf_pos = %d\n", msg, buf_pos );
	return copy_len;
}

static irq_handler_t morse_irq_handler(unsigned irq, void *dev_id, struct pt_regs *regs)
{
	printk( KERN_CRIT "INT \n" );
	return (irq_handler_t)IRQ_HANDLED;
}

int init_module( void )
{
	if(register_chrdev(devmajor, devname, &morse_fops)){
		printk( KERN_CRIT "%s : register_chrdev failed\n" );
		return -EBUSY;
	}

	spin_lock_init( &spn_lock );
	printk( KERN_CRIT "%s : loaded  into kernel\n", msg );

	return 0;
}

void cleanup_module( void )
{
	unregister_chrdev( devmajor, devname );
	printk( KERN_CRIT "%s : removed from kernel\n", msg );
}
