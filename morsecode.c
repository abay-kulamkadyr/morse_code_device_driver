/*
 *  *  *      Author: Abay Kulamkadyr 
 */
#include <linux/module.h>
#include <linux/miscdevice.h>		// for misc-driver calls.
#include <linux/fs.h>
#include <linux/delay.h>			//for blocking 
#include <linux/uaccess.h>			//for reading/writing to user space
#include <linux/leds.h>				//for trigger library
#include <linux/kfifo.h>			//kernel's fifo

#define FIFO_SIZE 1024
static DECLARE_KFIFO(morsecode_fifo, char, FIFO_SIZE);

//#error Are we building this?
#define MY_DEVICE_FILE  "morse-code"

/******************************************************
 * LED
 ******************************************************/

DEFINE_LED_TRIGGER(morse_code_ledtrig);


#define UPPERCASE_A_ASCII   65
#define LOWERCASE_A_ASCII   97
#define UPPERCASE_Z_ASCII   90
#define LOWERCASE_Z_ASCII   122

//Morse code timings 
#define DOT_TIME_MS                     200
#define DASH_TIME_MS                    200*3
#define TIME_BETWEEN_TWO_LETTERS_MS     200*3
#define WORD_BREAK_TIME_MS              200*7

//Mask to determine if dashes/dots in the morsecode_codes[] array
#define MASK                            0x0007
// Morse Code Encodings (from http://en.wikipedia.org/wiki/Morse_code)
//   Encoding created by Brian Fraser. Released under GPL.
static unsigned short morsecode_codes[] = {
		0xB800,	// A 1011 1
		0xEA80,	// B 1110 1010 1
		0xEBA0,	// C 1110 1011 101
		0xEA00,	// D 1110 101
		0x8000,	// E 1
		0xAE80,	// F 1010 1110 1
		0xEE80,	// G 1110 1110 1
		0xAA00,	// H 1010 101
		0xA000,	// I 101
		0xBBB8,	// J 1011 1011 1011 1
		0xEB80,	// K 1110 1011 1 
		0xBA80,	// L 1011 1010 1
		0xEE00,	// M 1110 111
		0xE800,	// N 1110 1
		0xEEE0,	// O 1110 1110 111
		0xBBA0,	// P 1011 1011 101
		0xEEB8,	// Q 1110 1110 1011 1
		0xBA00,	// R 1011 101
		0xA800,	// S 1010 1
		0xE000,	// T 111
		0xAE00,	// U 1010 111
		0xAB80,	// V 1010 1011 1
		0xBB80,	// W 1011 1011 1
		0xEAE0,	// X 1110 1010 111
		0xEBB8,	// Y 1110 1011 1011 1
		0xEEA0	// Z 1110 1110 101
};

/******************************************************
 * Helper functions
 ******************************************************/
/*
 *wrapper function to make flashing functionalities more discriptive
 */
static inline void drive_led_on(void)
{
    led_trigger_event(morse_code_ledtrig, LED_FULL);
}
static inline void drive_led_off(void)
{
   	led_trigger_event(morse_code_ledtrig, LED_OFF);
}

/*
*helper function, determines if a passed character is a letter
*returns 1 if a passed character is a letter t 
*returns 0 if not
*/
static int is_letter (char ch)
{
    return  (ch >= UPPERCASE_A_ASCII && ch <= UPPERCASE_Z_ASCII)
            ||
            (ch >= LOWERCASE_A_ASCII && ch <= LOWERCASE_Z_ASCII);
} 
static void flash_dot(void)
{
    drive_led_on();
    msleep(DOT_TIME_MS);
    drive_led_off();
}

static void flash_dash (void)
{
    drive_led_on();
    msleep(DASH_TIME_MS);
    drive_led_off();
}

//Takes a character and maps it to an index in the morsecode_codes[] array 
static unsigned int convert_char_to_index(char ch)
{
   
    if(is_letter(ch))
    {
        //if its a lower case letter, convert to upper case
        if (ch >= LOWERCASE_A_ASCII && ch<=LOWERCASE_Z_ASCII) {
            ch=ch-32; // distance between upper and lower case letters
        }
        return ch-UPPERCASE_A_ASCII;
    }
    printk(KERN_INFO "Passed an invalid letter for char to index converter");
    return -1;
}
/*
 *mirros/inverts a binary number
 *ex 1011 1010-> 0101 1101
 */
static short mirror_bit_number(unsigned short value)
{
	unsigned short result=0; 
	unsigned short shift_result = value;
	unsigned short mask = 0x0001;
	int i;
	int lsb_number = 0;
	for(i=sizeof(short)*8-1; i>=0; i=i-1)
	{
		shift_result = shift_result >> i;
		result = result + (shift_result & mask);
		mask = mask << 1; 
		lsb_number=lsb_number+1;
		shift_result = value << (lsb_number); 
	}
	return result;
}
static inline void dot_dash_separator(void)
{
    drive_led_off();
    msleep(DOT_TIME_MS);
}
static inline void two_letter_separator(void)
{
    drive_led_off();
    msleep(TIME_BETWEEN_TWO_LETTERS_MS);
}
static inline void word_separator (void)
{
    drive_led_off();
    msleep(WORD_BREAK_TIME_MS);
}

/*
 *flashes morse code for a given character ch
 */
static void flash_char(char ch)
{
    int index;
    short morse_code_value=0;
    short mask_result=0;
	short inverted_value; 
    if(is_letter(ch)) {
		// get the index of the passed character in the morsecode_codes[] array
		index=convert_char_to_index(ch);
		//get the encoding of this character
		morse_code_value= morsecode_codes[index];
		//mirror the bits of the encoding to apply a bit mask
		inverted_value = mirror_bit_number (morse_code_value);
		//apply bit mask of 7=111, to determine if its a dash or not
		mask_result = inverted_value & MASK;
	} else {	
		printk(KERN_ERR "ERROR: passed an invalid character to flash\n");
		return; 
	}

	while (mask_result!=0){
		/*
		 *if its a dash, (i.e. something with consequetive 1s, i.e. 110, 110, 111)
		 *then need to skip 4 bits 
		 */
		if( mask_result == MASK) {
			
			inverted_value= inverted_value >> 4;
			flash_dash();
			if(!kfifo_put(&morsecode_fifo, '-')) {
				printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");
			
			}
			dot_dash_separator();
		}
		/*
		 *if its a dot, then only need to skip 2 bits
		 */
		else {
			
			flash_dot();
			if(!kfifo_put(&morsecode_fifo, '.')) {
				printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");
			
			}
			dot_dash_separator();
			inverted_value= inverted_value >> 2; 
		}
		
		mask_result = inverted_value & MASK;
	}
	if(!kfifo_put(&morsecode_fifo, ' ')) { 
		printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");
	}
}



static void led_register(void)
{
	// Setup the trigger's name:
	led_trigger_register_simple("morse-code", &morse_code_ledtrig);
}

static void led_unregister(void)
{
	// Cleanup
	led_trigger_unregister_simple(morse_code_ledtrig);
}


/******************************************************
 * Callbacks
 ******************************************************/
static ssize_t my_write(struct file* file, const char *buff,
		size_t count, loff_t* ppos)
{
	int i;
	int inter_character_delay_counter = 0;
	char currently_proccessing_character = 0;
	//process one character at a time
	for (i=0; i<count; i++)
	{
		if(copy_from_user (&currently_proccessing_character, &buff[i], sizeof(char))) {
			return -EFAULT;
		}

		if(is_letter(currently_proccessing_character)) {

			if(inter_character_delay_counter == 2) {
				two_letter_separator();
				inter_character_delay_counter = 0;
			}
			printk (KERN_INFO "character being processed is %c\n", currently_proccessing_character);
			flash_char(currently_proccessing_character);
			inter_character_delay_counter++;
		}
		else if(currently_proccessing_character == ' ') {
			word_separator();
			// adding 3 spaces
			if(!kfifo_put(&morsecode_fifo, ' ')) {
				printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");
			
			}
			if(!kfifo_put(&morsecode_fifo, ' ')) {
				printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");
			
			}
			if(!kfifo_put(&morsecode_fifo, ' ')) {
				printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");
			
			}
		}
		else
		{ 
			continue;
		}
	}
	if(!kfifo_put(&morsecode_fifo, '\n')) {
		printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");
	
	}
	// Print out the message
	if (count <= 0) {
		printk(KERN_INFO "No characters provided to flash .\n");
	}
	// Return # bytes actually written.
	*ppos += count;
	return count;	
}
static ssize_t my_read(struct file* file, char *buff,
		size_t count, loff_t* ppos)
{
	int num_bytes_read = 0; 

	// if(!kfifo_put(&morsecode_fifo, '\n')) { 
	// 	printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");	

	// }
	// if(!kfifo_put(&morsecode_fifo, '\0')) { 
	// 	printk(KERN_ERR "ERROR: couldn't put character into kfifo\n");	

	// }
	
	if (kfifo_to_user(&morsecode_fifo, buff, count, &num_bytes_read)) {
		return -EFAULT;
	}
	
	
	//printk(KERN_INFO "about to put %d numbytes", num_bytes_read);
	return num_bytes_read;
}


/******************************************************
 * Misc support
 ******************************************************/
// Callbacks:  (structure defined in <kernel>/include/linux/fs.h)
struct file_operations my_fops = {
	.owner    =  THIS_MODULE,
	.write    =  my_write,
	.read	  =  my_read
};

// Character Device info for the Kernel:
static struct miscdevice my_miscdevice = {
		.minor    = MISC_DYNAMIC_MINOR,         // Let the system assign one.
		.name     = MY_DEVICE_FILE,             // /dev/.... file.
		.fops     = &my_fops                    // Callback functions.
};


/******************************************************
 * Driver initialization and exit:
 ******************************************************/
static int __init my_init(void)
{
	int ret;
	printk(KERN_INFO "----> morse-code driver init(): file /dev/%s.\n", MY_DEVICE_FILE);

	// Register as a misc driver:
	ret = misc_register(&my_miscdevice);
	//FIFO: 
	INIT_KFIFO(morsecode_fifo);
	// LED:
	led_register();
	return ret;
}

static void __exit my_exit(void)
{
	printk(KERN_INFO "<---- morse-code driver exit().\n");

	// Unregister misc driver
	misc_deregister(&my_miscdevice);

	// LED:
	led_unregister();
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Abay Kulamkadyr");
MODULE_DESCRIPTION("A simple morsecode LED flashing");
MODULE_LICENSE("GPL");
