/* kernel.c */
#include "keyboardMap.h"

#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08
#define ENTER_KEY_CODE 0x1C


enum textColor
{
	BLACK 			= 0x00,
	BLUE 			= 0x01,
	GREEN 			= 0x02,
	CYAN 			= 0x03,
	RED 			= 0x04,
	MAGENTA 		= 0x05,
	BROWN 			= 0x06,
	LIGHT_GREY 		= 0x07,
	DARK_GREY 		= 0x08,
	LIGHT_BLUE 		= 0x09,
	LIGHT_GREEN 	= 0x0a,
	LIGHT_CYAN 		= 0x0b,
	LIGHT_RED 		= 0x0c,
	LIGHT_MAGENTA 	= 0x0d,
	LIGHT_BROWN 	= 0x0e,
	WHITE 			= 0x0f
};


extern unsigned char keyboard_map[128];
extern void keyboardHandler(void);
extern char readPort(unsigned short port);
extern void writePort(unsigned short port, unsigned char data);
extern void loadIDT(unsigned long *idt_pointer);

unsigned int current_cursor_location = 0;
char *video_pointer = (char*)0xb8000;

/* IDT -  Interrupt Descriptor Table */
/* We implement IDT as an array comprising structures idtEntry.*/

struct idtEntry 
{
	unsigned short int 	offset_lowerbits;
	unsigned short int 	selector;
	unsigned char 		zero;
	unsigned char 		type_attribute;
	unsigned short int 	offset_higherbits;
};

struct idtEntry IDT[IDT_SIZE];


void idtInitialization(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_pointer[2];

	/* Populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboardHandler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attribute = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/* PIC - Programmable Interrupt Controller */
	/* IRQ - Interrupt Request */ 

	/*
	*Modern x86 systems have 2 PIC chips each having 8 input lines. 
	*Let’s call them PIC1 and PIC2. 
	*PIC1 receives IRQ0 to IRQ7 and PIC2 receives IRQ8 to IRQ15. 
	PIC1 uses port 0x20 for Command and 0x21 for Data. 
	*PIC2 uses port 0xA0 for Command and 0xA1 for Data.
	*/

	/* 
	*		 Ports
	*	 	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	/* ICW - Initialization command words */ 

	/*
	*ICW2 - vector offset.
	*ICW3 -How the PICs wired as master/slaves.
	*ICW4- Gives additional information about the environment.
	*/

	/* ICW1 - begin initialization (initialize command) */
	writePort(0x20, 0x11);
	writePort(0xA0, 0x11);

	/* ICW2 - remap offset address of IDT */
	/* 
	*ICW2, written to the data ports of each PIC. 
	*It sets the PIC’s offset value. 
	*This is the value to which we add the input line number to form the Interrupt number. 
	*/

	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	writePort(0x21, 0x20);
	writePort(0xA1, 0x28);

	/* ICW3 - setup cascading */
	/*
	*Cascading PICs outputs to inputs between each other,
	*and each bit represents cascading status for the corresponding IRQ. 
	*For now, we won’t use cascading and set all to zeroes.
	*/
	writePort(0x21, 0x00);
	writePort(0xA1, 0x00);

	/* 
	*ICW4 sets the additional enviromental parameters. 
	*We will just set the lower most bit to tell the PICs we are running in the 80x86 mode. 
	*/
	writePort(0x21, 0x01);
	writePort(0xA1, 0x01);
	/* Initialization finished */

	/* Mask interrupts */
	writePort(0x21, 0xff);
	writePort(0xA1, 0xff);

	/* Fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_pointer[0] = (sizeof (struct idtEntry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_pointer[1] = idt_address >> 16 ;

	loadIDT(idt_pointer);
}

void keyboardInitialization(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	writePort(0x21, 0xFD);
}

void keyPrint(const char *string)
{
	unsigned int i = 0;

	while (string[i] != '\0') 
	{
		video_pointer[current_cursor_location++] = string[i++];
		video_pointer[current_cursor_location++] = GREEN;
	}
}

void keyPrintNewline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_cursor_location = current_cursor_location + (line_size - current_cursor_location % (line_size));
}

void clearScreen(void)
{
	unsigned int i = 0;

	while (i < SCREENSIZE) 
	{
		video_pointer[i++] = ' ';
		video_pointer[i++] = GREEN;
	}
}

void keyboardHandlerMain(void)
{
	unsigned char status;
	char keycode;

	/* write EOI */
	writePort(0x20, 0x20);

	status = readPort(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01)
	{
		keycode = readPort(KEYBOARD_DATA_PORT);
		
		if (keycode < 0)
			return;

		if (keycode == ENTER_KEY_CODE) 
		{
			keyPrintNewline();
			return;
		}

		video_pointer[current_cursor_location++] = keyboard_map[(unsigned char) keycode];
		video_pointer[current_cursor_location++] = GREEN;
	}
}

void kernelMain(void)
{
	const char *string = "FUCK KERNEL WITH KEYBOARD SUPPORT";
	clearScreen();
	keyPrint(string);
	keyPrintNewline();
	keyPrintNewline();

	idtInitialization();
	keyboardInitialization();

	while(1);
}