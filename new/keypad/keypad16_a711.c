/*
 *  keypad16.c: Creates a char device driver to manage 4x4 keyboards 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/tty.h>      	/* For the tty declarations */
#include <asm/uaccess.h>	/* for put_user */
#include <asm/io.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Malta de ARCOM");
MODULE_DESCRIPTION("Device driver for 4x4 keyboards");	/* What does this module do */
MODULE_VERSION("a07.1.1");

/*  
 *  Prototypes - this would normally go in a .h file
 */
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "keypad16"	/* Dev name as it appears in /proc/devices   */
#define DEVICE_MAJOR 216
#define BUF_SIZE 80		/* Max length of char buffer */

#define BASEPORT  		0x378
#define SCAN_DELAY   		HZ/20

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
				   Used to prevent multiple access to device */


static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static struct timer_list my_timer;
static unsigned int key_status = 0;




static void keypad_scan(void)
{
        unsigned int local_kstatus = 0;
        unsigned char row_sel = 0x10;
        int row;
        unsigned char row_data;
        for( row=0 ; row<4 ; row++ )
        {
          outb( 0x0F | ~row_sel, BASEPORT);
          row_data =  inb(BASEPORT+1) ^ 0x80;
          local_kstatus = local_kstatus << 4;
          local_kstatus |= (row_data & 0xF0) >> 4;
          row_sel = row_sel << 1;
        }
        key_status = 0x0000ffff & ~local_kstatus;
}


static void keypad_proc(void)
{
        keypad_scan();
}


static void my_timer_func(unsigned long ptr)
{
	keypad_proc();
	my_timer.expires = jiffies + SCAN_DELAY;
	add_timer(&my_timer);
}



/*
 * This function is called when the module is loaded
 */
int __init init_module(void)
{
 	printk(KERN_INFO "Loading module keypad16\n");

        Major = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);

	if (Major < 0) {
	  printk(KERN_ALERT "  Registering char device failed with %d\n", Major);
	  return Major;
	}
	if (DEVICE_MAJOR > 0) Major = DEVICE_MAJOR;
	printk(KERN_INFO "  I was assigned major number %d.\n", Major);

	/*
	 * Set up the keypad scanner timer the first time
	 */
	init_timer(&my_timer);
	my_timer.function = my_timer_func;
	my_timer.expires = jiffies + SCAN_DELAY;
	add_timer(&my_timer);

	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void __exit cleanup_module(void)
{
	/* 
	 * Unregister the device 
	 */
	unregister_chrdev(Major, DEVICE_NAME);
	del_timer(&my_timer);
	printk(KERN_INFO "Module keypad16 unloaded.\n");
}


/*
 * Device File Methods
 */

/* 
 * Called when a process tries to open the device file.
 */
static int device_open(struct inode *inode, struct file *file)
{
	if (Device_Open) {
		printk(KERN_ALERT "Device keypad16 is already open\n");
		return -EBUSY;
	}

	Device_Open++;

	try_module_get(THIS_MODULE);
	printk(KERN_INFO "Device keypad16 open\n");

	return SUCCESS;
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;		/* We're now ready for our next caller */

	/* 
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module. 
	 */
	module_put(THIS_MODULE);
	printk(KERN_INFO "Device keypad16 closed\n");

	return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,   /* see include/linux/fs.h   */
                           char *buffer,        /* buffer to fill with data */
                           size_t length,       /* length of the buffer     */
                           loff_t * offset)
{
	char *st_ptr = (char *)&key_status;

	if (length<2)	return 0;

	/* 
	 * Actually put the data into the buffer 
	 */

	put_user(st_ptr[0], buffer++);
	put_user(st_ptr[1], buffer++);
	put_user('\n', buffer++);

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return 3;
}

/*  
 * Called when a process writes to dev file.
 */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "Sorry, write operation isn't supported.\n");
	return -EINVAL;
}

