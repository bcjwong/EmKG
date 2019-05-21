/*
 *
 *  PXA25x GPIOs exposed under /proc for reading and writing
 *  They will show up under /proc/gpio/NN
 *
 *  Based on patch 1773/1 in the arm kernel patch repository at arm.linux.co.uk
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/unistd.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>

#include <asm/arch/ssp.h>
#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <asm/arch/gpio.h>

#define MOD_NAME "mygraph"


int pxa_gpio_mode( int gpio_mode );
int pxa_gpio_get_value(unsigned gpio);
void pxa_gpio_set_value(unsigned gpio, int value);
int gpio_direction_input(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
static void gpio_exit(void);
static int gpio_init(void);
static int mypulse_open(struct inode *inode, struct file *filp);
static int mypulse_release(struct inode *inode, struct file *filp);
static ssize_t mypulse_read(struct file *filp,
		char *buf, size_t count, loff_t *f_pos);
static ssize_t mypulse_write(struct file *filp,
		const char *buf, size_t count, loff_t *f_pos);

struct file_operations mypulse_fops = {
	read: mypulse_read,
	write: mypulse_write,
	open: mypulse_open,
	release: mypulse_release
};


/* Global variables of the driver */
/* Major number */
static int mypulse_major = 61;

static unsigned capacity = 1000;
static int COUNT = 0;

/* Corresponding GPIO pins used for input and output */ 
static int *sig[8] = {30,9,29,28,17,16,118,117};

static int *value[100] = {0};
static int newval;

/* length of the current message */
static int mypulse_len;

/* Buffer to store data */
static char *mypulse_buffer;

/* Timers*/
static struct timer_list *my_timer;
static struct timer_list *my_calib;
static struct proc_dir_entry *proc_entry;

//Used for setting up calibration function
static int calib_flag = 0;

/* BPM Calculation Values */
static int BEAT_THRESHOLD = 650; //threshold for what input counts as a beat
static int BEAT_COUNT = 0;
static int SAMPLE_INTERVAL_COUNT = 0;
static int timer_intervals_per_sample = 5; 
static int bpm = 0;
static int firstFound = 0;
static int count = 0;

//Set threshold variables
static int averageBot;
static int averageTop;
static int *calib_data0[500] = {0};
static int *calib_data1[500] = {0};
static int *top_ten[10] = {0};
static int *bottom_ten[10] = {0};
static int runsumBot = 0;
static int runsumTop = 0;
static int CALIB_COUNT = 0;
static int minimum = 0;
static int maximum = 0;
static int min_index;
static int max_index;
static int delthe_calib = 0; //if calibrated once, the timer is created, free the timer with rmmod

/* Dangerous bpm value - Set to 90 for demo purposes */
static int dang_bpm = 90;

/* Interrupt signal handled by performing preliminary calibration of input signal. */
static void calib_callback(unsigned long data)
{
	calib_flag = 1;
	int j,x,k;
	int base = 4;
	int vol = 0;
	runsumTop = 0;
	runsumBot = 0;

	//Converts binary inputs into decimal value. 
	for (j = 0; j < 8; j++)
	{	
		x = pxa_gpio_get_value(sig[j]);
	
		if (x != 0)
		{
			k = 1;
		}
		else
		{
			k = 0;
		}
		vol += k*base;
		base = base*2;
	}
	calib_data0[CALIB_COUNT] = vol;
	calib_data1[CALIB_COUNT] = vol;
	CALIB_COUNT++; 					//increment count for calibration data

	if (CALIB_COUNT < 500)
	{
		mod_timer(my_calib, jiffies + msecs_to_jiffies(15));			//reset timer
	}
	else
	{
		for (k = 0; k < 10; k++)
		{	
			maximum = calib_data0[0];
			minimum = calib_data1[0];
			
			max_index = 0;
			min_index = 0;
			for (j = 1; j < 500; j++)
			{	
				if (calib_data0[j] > maximum)
				{
					maximum = calib_data0[j];
					max_index = j;
				}
				if (calib_data1[j] < minimum)
				{
					minimum = calib_data1[j];
					min_index = j;
				}
			}
			calib_data0[max_index] = 0;
			calib_data1[min_index] = 100000;
			runsumTop += maximum;
			runsumBot += minimum;
			
		}
		
		averageTop = runsumTop/10;	
		averageBot = runsumBot/10;
		
		BEAT_THRESHOLD = ((8 * (averageTop - averageBot))/10) + averageBot;
		CALIB_COUNT = 0;				//reset calib count
		printk("\nBeat Threshold: %d\n", BEAT_THRESHOLD);
		calib_flag = 0;
	}

}

/* Reading values from Ardiuno and Caculating Beats per Minute*/
static void display_callback(unsigned long data)
{
	int x, i, j;
	int base = 4;
	int vol = 0;
	int check;
	
	if (!calib_flag) // When the device is not calibrating
	{
		for (i = 0; i < 8; i++)
		{
			x = pxa_gpio_get_value(sig[i]);
			if (x != 0)
			{
				j = 1;
			}
			else
			{
				j = 0;
			}
			vol += j*base;
			base = base*2;
		}
		//Pulse Sensor
		newval = vol;

		for(i = 0; i < 100-1; i++)
		{
			value[i] = value[i+1];
		}

		value[99] = vol;


		if (value[99] < BEAT_THRESHOLD && firstFound == 1)
		{
			BEAT_COUNT++;
		}
		if (value[99] >= BEAT_THRESHOLD && firstFound == 0) //if input value is above threshold
		{
			firstFound = 1;
		}
		else if (value[99] >= BEAT_THRESHOLD && firstFound == 1 && BEAT_COUNT > 20)
		{
			if (BEAT_COUNT < 15 || BEAT_COUNT > 125)
			{
				printk("bpm: %d\n", bpm);
			}
			else
			{
				bpm = 5000/(BEAT_COUNT+1);
				if (bpm >= 150 || bpm < 45)
				{
					printk("No Pulse\n");
					pxa_gpio_set_value(113, 0);
				}
				else
				{
					//For demo purposes, dangerous bpm will be >= 90
					if (bpm >= dang_bpm)
					{
						printk("!!!DANGEROUS BPM: %d!!!\n", bpm);
						pxa_gpio_set_value(113, 1);
					}
					else
					{
						printk("bpm: %d\n", bpm);
						pxa_gpio_set_value(113, 0);
					}
				//printk("bpm: %d\n", bpm);
				count = 0;
				}
			
		
			}
			firstFound = 0;
			BEAT_COUNT = 0;

		}
	
	}
	mod_timer(my_timer, jiffies + msecs_to_jiffies(10));
}

/* Button for Calibrating */
irqreturn_t irq_BTN0(int irq, void *dev_id, struct pt_regs *regs)
{	
	
    init_timer(my_calib);
    my_calib->function = calib_callback;
    my_calib->expires = jiffies + msecs_to_jiffies(15);
    add_timer(my_calib);
	
	delthe_calib = 1;
	
	return IRQ_HANDLED;
}


static int mypulse_open(struct inode *inode, struct file *filp)
{
	return 0;
}


static int mypulse_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/* Function to put the newest voltage value into the proc file*/
static int mygraph_read( char *page, char **start, off_t off,
                   int count, int *eof, void *data )
{
	int len,i;

	if (off > 0) 
	{
		*eof = 1;
		return 0;
	}

	len = sprintf(page, "%d", newval);					//put value into file

	return len;

}

/* Function to put the Beats per minute into the dev file */
static ssize_t mypulse_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{ 
	char tbuf[3000], *tbptr = tbuf;
	int i;
	
	/* end of buffer reached */
	if (*f_pos > 0)
	{
		return 0;
	}

	if (calib_flag)
	{
		tbptr += sprintf(tbptr, "999999"); // Input 999999 to indicate that the device is calibrating
	}
	else
	{
		tbptr += sprintf(tbptr, "%d", bpm);
	}



	if (copy_to_user(buf, tbuf + *f_pos, strlen(tbuf))!=0)
	{
		return -EFAULT;
	}
	
	/* Changing reading position as best suits */ 
	*f_pos += (size_t) strlen(tbuf); 
	return strlen(tbuf); 

}

/* Writing value into the dev file*/
// Not Used
static ssize_t mypulse_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	int i;
	char tbuf[256], *tbptr = tbuf;

	/* end of buffer reached */
	if (*f_pos >= capacity)
	{
		printk(KERN_INFO
			"write called: process id %d, command %s, count %d, buffer full\n",
			current->pid, current->comm, count);
		return -ENOSPC;
	}
	
	/* do not go over the end */
	if (count > capacity - *f_pos)
		count = capacity - *f_pos;
	
	if (copy_from_user(mypulse_buffer+ *f_pos, buf, count))
	{
		return -EFAULT;
	}

	*f_pos += count;
	mypulse_len = *f_pos;

	return count;
	
}

/* Registers device, creates a proc file entry, initializes I/O GPIO pins, and allocates memory for buffers*/
static int gpio_init(void)
{
	int x;
	int result;
 	u32 data;
	int ret = 0;
	
	result = register_chrdev(mypulse_major, "mypulse", &mypulse_fops);
	
	if (result < 0)
	{
		printk(KERN_ALERT
			"mypulse: cannot obtain major number %d\n", mypulse_major);
		return result;
	}

	proc_entry = create_proc_entry( "mygraph", 0644, NULL );
	if (proc_entry == NULL) 
	{
		ret = -ENOMEM;
		printk(KERN_INFO "mygraph: Couldn't create proc entry\n");

	} 
	else 
	{
		proc_entry->read_proc = mygraph_read;
		proc_entry->owner = THIS_MODULE;
		//printk(KERN_INFO "mygraph: Module loaded.\n");

	}
	
	gpio_direction_input(30);
	gpio_direction_input(9);
	gpio_direction_input(29);
	gpio_direction_input(28);
	gpio_direction_input(17);
	gpio_direction_input(101);
	gpio_direction_input(16);
	gpio_direction_input(117);
	gpio_direction_input(118);
	
	gpio_direction_output(113, 0);

	//printk("init x: %d\n",x);
	
	my_calib = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	pxa_gpio_mode(31 | GPIO_IN); //Direction for interrupt
	request_irq(IRQ_GPIO(31), &irq_BTN0, SA_INTERRUPT | SA_TRIGGER_RISING,"mypulse", NULL); //assign interrupt function
	
	my_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
        init_timer(my_timer);
        my_timer->function = display_callback;
        my_timer->expires = jiffies + msecs_to_jiffies(10);
        add_timer(my_timer);
	

	mypulse_buffer = kmalloc(capacity, GFP_KERNEL); 
	
	if (!mypulse_buffer)
	{ 
		printk(KERN_ALERT "Insufficient kernel memory\n"); 
		result = -ENOMEM;
		goto fail; 
	} 
	memset(mypulse_buffer, 0, capacity);
	mypulse_len = 0;
	

	return 0;
	
fail: 
	gpio_exit(); 
	return result;
}

/* Unregisters device, frees I/O GPIO pins, deletes the timers, and removes proc entry */
static void gpio_exit(void)
{
	unregister_chrdev(mypulse_major, "mypulse");

	gpio_free(28);
	//gpio_free(31);
	gpio_free(113);
	gpio_free(30);
	gpio_free(9);
	gpio_free(29);
	gpio_free(17);
	gpio_free(101);
	gpio_free(16);
	gpio_free(117);
	gpio_free(118); 

	del_timer(my_timer);

	if (delthe_calib)
	{
		del_timer(my_calib);
	}	

	free_irq(IRQ_GPIO(31), NULL); 

	if (proc_entry)
	{
		remove_proc_entry(MOD_NAME, &proc_root);
		proc_entry = NULL;
	}
}

module_init(gpio_init);
module_exit(gpio_exit);
MODULE_LICENSE("Dual BSD/GPL");
