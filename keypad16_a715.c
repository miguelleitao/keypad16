/*
 *  keypad16.c: Creates a char device driver to manage 4x4 keyboards 
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/types.h>
#include <linux/proc_fs.h>	/* Necessary because we use the proc fs */
#include <linux/seq_file.h>	/* Proc file implemented by seq_file */
#include <linux/tty.h>      	/* For the tty declarations */
#include <asm/uaccess.h>	/* for put_user */
#include <asm/io.h>
#include <linux/slab.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Malta de ARCOM");
MODULE_DESCRIPTION("Device driver for 4x4 keyboards");	/* What does this module do */
MODULE_VERSION("a07.1.5");

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

#define PROCFS_MAX_SIZE         1024
#define PROCFS_DIR              "keypad16"
#define PROCFS_TABLE            "table"
#define PROCFS_BUFFER           "buffer"
#define PROCFS_REPEAT           "rep_time"
#define PROCFS_FIRST            "rep_first"

#define MAX_TABLE_KEYS		32

#define	NUM1			1756
#define ALPHA2			1758

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */
static int Device_Open = 0;	/* Is device open?  
				   Used to prevent multiple access to device */
static char msg[BUF_SIZE];	/* The msg the device will give when asked 
				   Circular buffer */
static char *msg_Ptr;		/* Begenning of message */
static char *end_Ptr;		/* End of message */
int  msg_len = 0;		/* Lenght of actual message */
static int KeypadModel = NUM1;  /* Keyboard model identifier */
static long  AutoRepeatFirstDelay = HZ;
static long  AutoRepeatDelay = HZ/10;
static char *model = "num1    ";	/* Model name parameter */

static int LOCAL_ECHO = 1;	/* Send directly to standard output */


#define INC_MSG_PTR(ptr)	ptr = ( ptr-msg<BUF_SIZE-1 ? ptr+1 : msg );

static int keypad_table_elements = 0;

static int bytes_read = 0;

typedef struct
{
        unsigned int scan_code;
        char         char_code;
} keypad_table_element;

static keypad_table_element keypad_table[MAX_TABLE_KEYS];


static int keypad_proc_table_show(struct seq_file *m, void *v) {
	int i;
        for (i=0 ; i<keypad_table_elements ; i++ ) {
                seq_printf(m, "%04X : %02X\n", keypad_table[i].scan_code, keypad_table[i].char_code );
        }

  return 0;
}
static int keypad_proc_buffer_show(struct seq_file *m, void *v) {

                char *mptr;
                mptr = msg_Ptr;

                while( mptr!=end_Ptr )
                {
                        seq_printf(m,"%c", *mptr);
                        INC_MSG_PTR(mptr);
                }
		seq_printf(m,"\n");

  return 0;
}
static int keypad_proc_first_show(struct seq_file *m, void *v) {
  seq_printf(m,"%ld\n",AutoRepeatFirstDelay);
  return 0;
}
static int keypad_proc_repeat_show(struct seq_file *m, void *v) {
  seq_printf(m,"%ld\n",AutoRepeatDelay);
  return 0;
}



static int keypad_proc_table_open(struct inode *inode, struct  file *file) {
  return single_open(file, keypad_proc_table_show, NULL);
}

static int keypad_proc_first_open(struct inode *inode, struct  file *file) {
  return single_open(file, keypad_proc_first_show, NULL);
}

static int keypad_proc_repeat_open(struct inode *inode, struct  file *file) {
  return single_open(file, keypad_proc_repeat_show, NULL);
}

static int keypad_proc_buffer_open(struct inode *inode, struct  file *file) {
  return single_open(file, keypad_proc_buffer_show, NULL);
}



static struct file_operations keypad_proc_table_fops = {
  .owner = THIS_MODULE,
  .open = keypad_proc_table_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};
static struct file_operations keypad_proc_buffer = {
  .owner = THIS_MODULE,
  .open = keypad_proc_buffer_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};
static struct file_operations keypad_proc_first = {
  .owner = THIS_MODULE,
  .open = keypad_proc_first_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};
static struct file_operations keypad_proc_repeat = {
  .owner = THIS_MODULE,
  .open = keypad_proc_repeat_open,
  .read = seq_read,
  .llseek = seq_lseek,
  .release = single_release,
};


static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

static struct timer_list my_timer;
static unsigned int key_status = 0;

/*
 * Queue of processes who want to read our device
 */
DECLARE_WAIT_QUEUE_HEAD(WaitQ);

module_param(model, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(model, "Keyboard model name");


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



static void echo_char(char c)
{
   struct tty_struct *my_tty;
   my_tty = current->signal->tty;           // The tty for the current task

   /* If my_tty is NULL, the current task has no tty you can print to (this is possible,
    * for example, if it's a daemon).  If so, there's nothing we can do.
    */
   if (my_tty != NULL) {

      /* my_tty->driver is a struct which holds the tty's functions, one of which (write)
       * is used to write strings to the tty.  It can be used to take a string either
       * from the user's memory segment or the kernel's memory segment.
       *
       * The function's 1st parameter is the tty to write to, because the same function
       * would normally be used for all tty's of a certain type.  
       * The 2rd parameter is a pointer to a string.
       * The 3th parameter is the length of the string.
       */

/*
      ((my_tty->driver)->write)(
         my_tty,                 // The tty itself
         &c,                     // String
         1 );		         // Length
*/
   }
}

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

static void ResetBuffer(void)
{
        msg[0] = (char)0;
        msg_Ptr = msg;
        end_Ptr = msg;
        msg_len = 0;
        bytes_read = 0;
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
			return  keypad_table[i].char_code;
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
	char newkey;
        keypad_scan();
        newkey = keypad_char();
	if ( newkey!=0	)
	{
	    if ( msg_len <= BUF_SIZE - 2 )
	    {
		*end_Ptr = newkey;
		INC_MSG_PTR(end_Ptr);
		*end_Ptr = (char)0;
		msg_len += 1;

	    }
	    else
	    {
		printk(KERN_INFO "Keypad16 Buffer full \n");
		// beep;
	    }
	    /* 
	     * Wake up all the processes in WaitQ, so if anybody is waiting for the
	     * file, they can have it.
	     */
	    wake_up(&WaitQ);
	}

}


static void my_timer_func(unsigned long ptr)
{
	keypad_proc();
	my_timer.expires = jiffies + SCAN_DELAY;
	add_timer(&my_timer);
}


/*
 *
 * This structures hold information about the /proc files
 *
 */ 


static struct proc_dir_entry *Our_Proc_Dir;
static struct proc_dir_entry *Our_Proc_Table;
static struct proc_dir_entry *Our_Proc_Buffer;
static struct proc_dir_entry *Our_Proc_Repeat;
static struct proc_dir_entry *Our_Proc_RepFirst;



/*
 *
 * This function is called when the /proc file is written
 *
 */
/*
int procfile_write_table(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
	char *tabp;
	int tabi = 0;
        // get buffer size 
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        // write data to the buffer 
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }
	procfs_buffer[procfs_buffer_size] = (char)0;
        printk(KERN_INFO "Leu %ld bytes: '%s'\n",procfs_buffer_size,buffer);
	
	// parse received data and generate a new table 
	tabp = procfs_buffer;
	while ( strlen(tabp)>6 && tabi<MAX_TABLE_KEYS ) {
		sscanf(tabp, "%4X : %2hhX\n", &(keypad_table[tabi].scan_code), &(keypad_table[tabi].char_code) );
		tabi++;
		tabp = strchr(tabp, 0x0a);
		if ( tabp==NULL )	break;
		tabp++;
	}
	keypad_table_elements = tabi;
        return procfs_buffer_size;
}
*/

/**
 * This function is called when the /proc file is written
 *
 */
/*
int procfile_write_buffer(struct file *file, const char *buffer, unsigned long count,
		   void *data)
{
	// get buffer size 
	 unsigned long procfs_buffer_size = count;
	if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
		procfs_buffer_size = PROCFS_MAX_SIZE;
	}
	
	// write data to the buffer 
	if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
		return -EFAULT;
	}
	printk(KERN_INFO "Leu %ld bytes: '%s'\n",procfs_buffer_size,buffer);
	return procfs_buffer_size;
}
*/

/*
 * This function is called when the /proc file is written
 */
/*
int procfile_write_first(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
        // get buffer size 
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        // write data to the buffer 
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }
        printk(KERN_INFO "procfile_write_first (not implemented) got %ld bytes: '%s'\n",procfs_buffer_size,buffer);
        return procfs_buffer_size;
}
*/

/*
 * This function is called when the /proc file is written
 */
/*
int procfile_write_repeat(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
        // get buffer size 
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        // write data to the buffer 
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }
        printk(KERN_INFO "procfile_write_repeat (not implemented) got %ld bytes: '%s'\n",procfs_buffer_size,buffer);
        return procfs_buffer_size;
}

*/

static int __init create_proc_file(struct proc_dir_entry **entry, const char* name, 
	int proc_open_function(struct inode *, struct  file *), struct file_operations *fops )
{
	(*entry) = proc_create(name, 0644, Our_Proc_Dir, fops);

	if ( (*entry) == NULL ) {
                printk(KERN_ALERT "Error: Could not initialize /proc/%s/%s\n",
                        PROCFS_DIR, name);
                return -ENOMEM;
        }
        printk(KERN_INFO "  /proc/%s/%s created\n", PROCFS_DIR, name);
	return 0;
}


/*
 * This function is called when the module is loaded
 */
int __init init_module(void)
{
	memcpy(keypad_table,  default_num1_table, sizeof(default_num1_table) );
        keypad_table_elements = sizeof(default_num1_table) / sizeof(keypad_table_element);

	if (strlen(model)>0) {
		printk(KERN_INFO "  Model name:'%s'\n", model);
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
	  printk(KERN_ALERT "Registering char device failed with %d\n", Major);
	  return Major;
	}
	if (DEVICE_MAJOR > 0) Major = DEVICE_MAJOR;
	ResetBuffer();
	printk(KERN_INFO "Loading module keypad16\n");
	printk(KERN_INFO "I was assigned major number %d.\n", Major);

	/* create the /proc files */
        Our_Proc_Dir  = proc_mkdir(PROCFS_DIR, NULL);
        if ( Our_Proc_Dir == NULL ) {
                printk(KERN_ALERT "Error: Could not create dir /proc/%s\n",
                        PROCFS_DIR);
                return -ENOMEM;
        }
/*
	struct proc_dir_entry *myproc_dir;
	myproc = proc_mkdir(PROCFS_DIR, NULL);
	if (myproc==NULL) {
	    printk(KERN_ERR "Could not create proc entry '%s'\n", PROCFS_DIR);
	    return -ENOMEM;
  	}
  	printk(KERN_INFO "Proc entry '%s' created\n", PROCFS_DIR);

	myproc = proc_create(PROCFS_DIR, 0, NULL, &keypad_proc_fops );
	if (myproc==NULL) {
    printk(KERN_ERR "Could not create proc entry '%s'\n", PROCFS_DIR);
    return -ENOMEM;
  }
  printk(KERN_INFO "Proc entry '%s' created\n", PROCFS_DIR);
*/

	create_proc_file(&Our_Proc_Table,     PROCFS_TABLE, keypad_proc_table_open, &keypad_proc_table_fops );
	create_proc_file(&Our_Proc_Buffer,    PROCFS_BUFFER, keypad_proc_buffer_open, &keypad_proc_buffer);
	create_proc_file(&Our_Proc_RepFirst,  PROCFS_FIRST, keypad_proc_first_open, &keypad_proc_first);
	create_proc_file(&Our_Proc_Repeat,    PROCFS_REPEAT, keypad_proc_repeat_open, &keypad_proc_repeat);


	/*
	 * Set up the keypad scanner timer the first time
	 */
	init_timer(&my_timer);
	my_timer.function = my_timer_func;
	my_timer.expires = jiffies + SCAN_DELAY;
	add_timer(&my_timer);

	printk(KERN_INFO "keypad16 teclas=%d\n", keypad_table_elements ); 
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

	remove_proc_entry(PROCFS_TABLE, Our_Proc_Dir);
	remove_proc_entry(PROCFS_BUFFER, Our_Proc_Dir);
	remove_proc_entry(PROCFS_FIRST, Our_Proc_Dir);
	remove_proc_entry(PROCFS_REPEAT, Our_Proc_Dir);

        remove_proc_entry(PROCFS_DIR, NULL);
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
	//ResetBuffer();		// Optional

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
        /*
         * Number of bytes actually written to the buffer
         */
	int bytes_read = 0;
	if (length==0)	return 0;
  	/* wait until there's data available (unless we do nonblocking reads) */
	while (*offset >= msg_len)
	{
   	  //if (file->f_flags & O_NONBLOCK)
    	  // return -EAGAIN;
    	  wait_event_interruptible(WaitQ, msg_len );
    	  if (signal_pending(current) )
      	  	return -ERESTARTSYS;
  	}

	/* 
	 * Actually put the data into the buffer 
	 */
	while (length && msg_len && *msg_Ptr) 
	{
		/* 
		 * The buffer is in the user data segment, not the kernel 
		 * segment so "*" assignment won't work.  We have to use 
		 * put_user which copies data from the kernel data segment to
		 * the user data segment. 
		 */
		put_user(*msg_Ptr, buffer++);
                if ( LOCAL_ECHO ) echo_char(*msg_Ptr);
		INC_MSG_PTR(msg_Ptr);
		length--;
		bytes_read++;
		msg_len--;
	}

	/* 
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
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

