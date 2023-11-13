


#define SCAN_DELAY              HZ/20

static unsigned int key_status = 0;

static struct timer_list my_timer;


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



no init
        /*
         * Set up the keypad scanner timer the first time
         */
        init_timer(&my_timer);
        my_timer.function = my_timer_func;
        my_timer.expires = jiffies + SCAN_DELAY;
        add_timer(&my_timer);

em device_read:
	char *st_ptr = (char *)&key_status;


        if (length<2)   return 0;

        put_user(st_ptr[0], buffer++);
        put_user(st_ptr[1], buffer++);



em cleanup_module:
        del_timer(&my_timer);






/// do 1 para o 2

#define MAX_TABLE_KEYS          32

typedef struct
{
        unsigned int scan_code;
        char         char_code;
} keypad_table_element;

static keypad_table_element keypad_table[MAX_TABLE_KEYS];

static keypad_table_element default_num1_table[]  =
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



/*
 *  Translates key_status into char, using keypad_table
 */
static char keypad_key(unsigned int key_status)
{
        int i;
        for (i=0 ; i<keypad_table_elements ; i++ )
        {
                if ( keypad_table[i].key_status==keypad_table[i].scan_c )
			 return char_c;
        }
        return (char)0;
}


em keypad_proc
        newkey = keypad_key(key_status);


no init:
        memcpy(keypad_table,  default_num1_table, sizeof(default_num1_table) );
        keypad_table_elements = sizeof(default_num1_table) / sizeof(keypad_table_element);


/////////////////////////////////////////////////////////////////////////////////////
/// do 2 para 3

static int KeypadModel = NUM1;  /* Keyboard model identifier */
static char *model = "num1    ";        /* Model name parameter */



module_param(model, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(model, "Keyboard model name");


no init:


        if (strlen(model)>0) {
                switch (model[0]) {
                        case 'A':               // model: alpha2
                        case 'a':
                                KeypadModel = ALPHA2;
                                memcpy(keypad_table,  default_alpha2_table, sizeof(default_alpha2_table) );
                                keypad_table_elements = sizeof(default_alpha2_table) / sizeof(keypad_table_element);
                                printk(KERN_INFO "  Model switched to 'Alpha2'\n");
                                break;
                        case 'N':               // model: num1
                        case 'n':
                        default:
                                KeypadModel = NUM1;
                                break;
                }
        }





/////////////////////////////////////////////////////////////////////////////////////

do 3 para 4



para autorepeat:

static long  AutoRepeatFirstDelay = HZ;
static long  AutoRepeatDelay = HZ/10;


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


em keypad_proc
        newkey = keypad_char();


///   Do 4 +para 5

#include <linux/proc_fs.h>      /* Necessary because we use the proc fs */



#define PROCFS_MAX_SIZE         1024
#define PROCFS_DIR              "keypad16"
#define PROCFS_TABLE            "table"
#define PROCFS_BUFFER           "buffer"
#define PROCFS_REPEAT           "rep_time"
#define PROCFS_FIRST            "rep_first"

/*
 * This structures hold information about the /proc files
 */
static struct proc_dir_entry *Our_Proc_Dir;
static struct proc_dir_entry *Our_Proc_Table;
static struct proc_dir_entry *Our_Proc_Buffer;
static struct proc_dir_entry *Our_Proc_Repeat;
static struct proc_dir_entry *Our_Proc_RepFirst;

/*
 * The buffer used to store character for this module
 */
static char procfs_buffer[PROCFS_MAX_SIZE];

/*
 * The size of the buffer
 */
static unsigned long procfs_buffer_size = 0;


/*
 * This function is called then the /proc file is read
 */
int
procfile_read_table(char *buffer,
              char **buffer_location,
              off_t offset, int buffer_length, int *eof, void *data)
{
        int i;
        char *buf;
        buf = buffer;
        for (i=0 ; i<keypad_table_elements && i*10<buffer_length ; i++ )
        {
                char lineT[20];
                int li;
                down_interruptible(&TableMtx);
                sprintf(lineT, "%04X : %02X\n", keypad_table[i].scan_code, keypad_table[i].char_code );
                up(&TableMtx);
                for( li=0 ; li<strlen(lineT) ; li++ )
                        *(buf++) = lineT[li];
        }
        *buf = (char)0;
        return i*10;
}
/**
 * This function is called when the /proc file is written
 *
 */
int procfile_write_table(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
        char *tabp;
        int tabi = 0;
        /* get buffer size */
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }
        procfs_buffer[procfs_buffer_size] = (char)0;
        printk(KERN_INFO "Leu %ld bytes: '%s'\n",procfs_buffer_size,buffer);

        /* parse received data and generate a new table */
        tabp = procfs_buffer;
        while ( strlen(tabp)>6 && tabi<MAX_TABLE_KEYS ) {
                down_interruptible(&TableMtx);
                sscanf(tabp, "%4X : %2hhX\n", &(keypad_table[tabi].scan_code), &(keypad_table[tabi].char_code) );
                up(&TableMtx);
                tabi++;
                tabp = strchr(tabp, 0x0a);
                if ( tabp==NULL )       break;
                tabp++;
        }
        keypad_table_elements = tabi;
        return procfs_buffer_size;
}

/**
 * This function is called then the /proc file is read
 *
 */
int
procfile_read_buffer(char *buffer,
              char **buffer_location,
              off_t offset, int buffer_length, int *eof, void *data)
{
        int ret;

        printk(KERN_INFO "procfile_read_buffer (/proc/%s/%s) called\n", PROCFS_DIR, PROCFS_TABLE);

        if (offset > 0) {
                /* we have finished to read, return 0 */
                ret  = 0;
        } else {
                /* fill the buffer, return the buffer size */
                char *mptr, *buf;
                mptr = msg;
                buf  = buffer;
                while( buffer_length && mptr!=end_Ptr )
                {
                        *buf = *mptr;
                        buf++;
                        INC_MSG_PTR(mptr);
                }
                ret = msg_len;
        }
        return ret;
}

/*
 * This function is called when the /proc file is written
 */
int procfile_write_buffer(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
        /* get buffer size */
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }
        printk(KERN_INFO "Leu %ld bytes: '%s'\n",procfs_buffer_size,buffer);
        return procfs_buffer_size;
}

/*
 * This function is called then the /proc file is read
 */
int
procfile_read_first(char *buffer,
              char **buffer_location,
              off_t offset, int buffer_length, int *eof, void *data)
{
        int i;
        char *buf;
        char mymsg[20];
        buf = buffer;
        sprintf( mymsg, "%ld\n", AutoRepeatFirstDelay );

        for( i=0 ; i<strlen(mymsg) && i<buffer_length ; i++ )
                        *(buf++) = mymsg[i];
        return i;
}

/*
 * This function is called then the /proc file is read
 */
int
procfile_read_repeat(char *buffer,
              char **buffer_location,
              off_t offset, int buffer_length, int *eof, void *data)
{
        int i;
        char *buf;
        char mymsg[20];
        buf = buffer;
        sprintf( mymsg, "%ld\n", AutoRepeatDelay );

        for( i=0 ; i<strlen(mymsg) && i<buffer_length ; i++ )
                        *(buf++) = mymsg[i];
        return i;
}


/*
 * This function is called when the /proc file is written
 */
int procfile_write_first(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
        /* get buffer size */
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }
        printk(KERN_INFO "procfile_write_first (not implemented) got %ld bytes: '%s'\n",procfs_buffer_size,buffer);
        return procfs_buffer_size;
}

/*
 * This function is called when the /proc file is written
 */
int procfile_write_repeat(struct file *file, const char *buffer, unsigned long count,
                   void *data)
{
        /* get buffer size */
        unsigned long procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }
        printk(KERN_INFO "procfile_write_repeat (not implemented) got %ld bytes: '%s'\n",procfs_buffer_size,buffer);
        return procfs_buffer_size;
}:

static int __init create_proc_file(struct proc_dir_entry **entry, const char* name,
        read_proc_t *read_func,  write_proc_t *write_func )
{
        (*entry) = create_proc_entry(name, 0644, Our_Proc_Dir);

        if ( (*entry) == NULL ) {
                remove_proc_entry(PROCFS_TABLE, Our_Proc_Table);
                printk(KERN_ALERT "Error: Could not initialize /proc/%s/%s\n",
                        PROCFS_DIR, PROCFS_TABLE);
                return -ENOMEM;
        }
        (*entry)->read_proc  = read_func;
        (*entry)->write_proc = write_func;
        (*entry)->owner     = THIS_MODULE;
        (*entry)->mode      = S_IFREG | S_IRUGO;
        (*entry)->uid       = 0;
        (*entry)->gid       = 0;
        (*entry)->size      = 180;

        printk(KERN_INFO "  /proc/%s/%s created\n", PROCFS_DIR, name);
        return 0;
}

no init_module()


...



na função cleanup_module()
        remove_proc_entry(PROCFS_TABLE, Our_Proc_Dir);
        remove_proc_entry(PROCFS_BUFFER, Our_Proc_Dir);
        remove_proc_entry(PROCFS_FIRST, Our_Proc_Dir);
        remove_proc_entry(PROCFS_REPEAT, Our_Proc_Dir);

        remove_proc_entry(PROCFS_DIR, &proc_root);


