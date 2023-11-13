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
MODULE_VERSION("a07.1.3");

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

#define MAX_TABLE_KEYS		32

#define	NUM1			1756
#define ALPHA2			1758

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
				   Used to prevent multiple access to device */
static int KeypadModel = NUM1;  /* Keyboard model identifier */
static long  AutoRepeatFirstDelay = HZ;
static long  AutoRepeatDelay = HZ/10;
static char *model = "num1    ";	/* Model name parameter */


#define INC_MSG_PTR(ptr)	ptr = ( ptr-msg<BUF_SIZE-1 ? ptr+1 : msg );

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static struct timer_list my_timer;
static unsigned int key_status = 0;
static char newkey;

module_param(model, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(model, "Keyboard model name");

typedef struct
{
	unsigned int scan_code;
	char	     char_code;
} keypad_table_element;

static keypad_table_element keypad_table[MAX_TABLE_KEYS];

static keypad_table_element default_num1_table[]  = 
{
	{ 0x0001,	'd' },
	{ 0x0002,	'c' },
	{ 0x0004,	'a' },
	{ 0x0008,       'b' },
        { 0x0010,       '\n' },
        { 0x0020,       '9' },
        { 0x0040,       '3' },
        { 0x0080,       '6' },
        { 0x0100,       '0' },
        { 0x0200,       '8' },
        { 0x0400,       '2' },
        { 0x0800,       '5' },
        { 0x1000,       '*' },
        { 0x2000,       '7' },
        { 0x4000,       '1' },
        { 0x8000,       '4' },
};

static keypad_table_element default_alpha2_table[]  =
{
        { 0x0001,       'd' },
        { 0x0002,       'c' },
        { 0x0004,       'a' },
        { 0x0008,       'b' },
        { 0x0010,       '\n' },
        { 0x0020,       '9' },
        { 0x0040,       '3' },
        { 0x0080,       '6' },
        { 0x0100,       '0' },
        { 0x0200,       '8' },
        { 0x0400,       '2' },
        { 0x0800,       '5' },
        { 0x1000,       '*' },
        { 0x2000,       '7' },
        { 0x4000,       '1' },
        { 0x8000,       '4' },
};


static int keypad_table_elements = 0;

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


/*
 *  Translates key_status into char, using keypad_table
 */
static char keypad_key(unsigned int key_status)
{
	int i;
	for (i=0 ; i<keypad_table_elements ; i++ )
	{
	      unsigned int scan_c;
		scan_c = keypad_table[i].scan_code;
		if ( key_status==scan_c ) 
			return keypad_table[i].char_code;
	}
	return (char)0;
}

static char keypad_char(void)
{
        static unsigned int last_key_status = 0;
        static long last_key_jiffies;
	char keypad_newchar = '\0';

        if (key_status != 0)
        {
          if ( key_status != last_key_status )          // new key
          {
            last_key_jiffies = jiffies + AutoRepeatFirstDelay;
	    keypad_newchar = keypad_key(key_status);
          }
          else
            if ( jiffies>=last_key_jiffies )
            {
               last_key_jiffies = jiffies + AutoRepeatDelay;
               keypad_newchar = keypad_key(key_status);
            }  
        }
        last_key_status = key_status;
        return keypad_newchar;
}

static void keypad_proc(void)
{
        keypad_scan();
        newkey = keypad_char();
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

	memcpy(keypad_table,  default_num1_table, sizeof(default_num1_table) );
        keypad_table_elements = sizeof(default_num1_table) / sizeof(keypad_table_element);

	if (strlen(model)>0) {
		switch (model[0]) {
			case 'A':		// model: alpha2
			case 'a':
				KeypadModel = ALPHA2;
				memcpy(keypad_table,  default_alpha2_table, sizeof(default_alpha2_table) );
				keypad_table_elements = sizeof(default_alpha2_table) / sizeof(keypad_table_element);
				printk(KERN_INFO "  Model switched to 'Alpha2'\n");
				break;
			case 'N':		// model: num1
			case 'n':
			default:
				KeypadModel = NUM1;
				break;
		}
	}
			
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

	printk(KERN_INFO "  keypad16 teclas=%d\n", keypad_table_elements ); 
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
	if (Device_Open)
		return -EBUSY;

	Device_Open++;

	try_module_get(THIS_MODULE);

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
	if (length==0)	return 0;

	/* 
	 * Actually put the data into the buffer 
	 */

	/* 
	 * The buffer is in the user data segment, not the kernel 
	 * segment so "*" assignment won't work.  We have to use 
	 * put_user which copies data from the kernel data segment to
	 * the user data segment. 
	 */
	put_user(newkey, buffer);
	newkey = 0;

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return 1;
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

