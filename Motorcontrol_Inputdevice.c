/*
 * Motorcontrol_Inputdevice.c
 * Hardware: ATmega328P
*/ 

//--------------------------DEFINES--------------------------//
// Frequency defines:
#define F_CPU           14745600UL
#define UART_BAUD_RATE 	115200

// ENDSTOP defines:
#define ENDSTOP PIND          // End-stop readin on Port D (Input)
#define XEND    0b00010000    // XEND input on PIN 4
#define YEND    0b00100000    // YEND input on PIN 5
#define ZEND    0b01000000    // ZEND input on PIN 6

// Stepport defines:
#define STEPPORT  PORTC
#define STEPPIN   0b00000001	// Step
#define DIRPIN    0b00000010	// Direction
#define	XPIN      0b00000100	// Enable pin for X-axis
#define	YPIN      0b00001000	// Enable pin for Y-axis
#define	ZPIN      0b00010000	// Enable pin for Z-axis
#define	TPIN      0b00100000	// Toggle pin for router-spindle

// Step pulse width:
#define STEPPULSE   5         // Step pulse time (µs). Clock time > 0.5 µs (see L297 datasheet p. 7)	
#define STEPTIME    1900      // Step time hold (between steps) (µs)
//-----------------------------------------------------------//

//--------------------------INCLUDES--------------------------//
#include <avr/interrupt.h>    // Dependency comes with AVR Studio
#include <util/delay.h>       // Dependency comes with AVR Studio
#include "uart.h"             // Serial uart communication uses the dependency library (v 1.12) by Peter Fleury 
//------------------------------------------------------------//

//--------------------------Variable--------------------------//
char c[2];
char newdata,ins_dir,endf=0,endb=0;
int ins_step;
//------------------------------------------------------------//

ISR(INT0_vect)                // Interrupt routine for ENDSTOP
{
	uart_putc(0xA0);            // Tell PC that endstop has been reached
	
	// Back off from end stop:
	STEPPORT &= ~DIRPIN;        // Inverse the direction pin from previous state
	while (XEND | YEND | ZEND)  // As long as an endstop pin is active
	{			
		STEPPORT &= ~STEPPIN;     // Pull the step pin low to execute a step (falling edge)
		_delay_us(STEPPULSE);     // Step puls time				
		STEPPORT |= STEPPIN;      // Pull the step pin high again to await new step
		_delay_us(STEPTIME);
	}
	ins_step=0;                 // ins_step is reset at the end of this interrupt, so we won't step right back to the endstop when this routine is over
}

char uart_recieve()           // Get instructions via uart serial communication 
{
  int r = uart_getc();        // Poll the uart interface
  if ( r & UART_NO_DATA )
    {
      // No data
    }
    else
    {
      if ( r & UART_FRAME_ERROR )     // Framing Error. 
      {
        // Return the error code to the PC
        uart_putc(0xA1);
      }
      if ( r & UART_OVERRUN_ERROR )   // Overrun error.
      {
        // Return the error code to the PC
        uart_putc(0xA2);
      }
      if ( r & UART_BUFFER_OVERFLOW ) // Dropped character
      {
        // Return the error code to the PC
        uart_putc(0xA3);
      }
      newdata=1;                      // If these error checks are passed, there's new data. Set the newdata flag high
      return r;                       // Success. Return the received data
    }
}

void receive_data()                   // Recieve data function: Calls the uart multiple times and merges data until theres two valid bytes of data recieved
{
  //---------- Wait for two bytes to be received ---------- //
  while (newdata==0)                  // Wait for first byte
  {
    c[0] = uart_recieve();
  }
  newdata=0;                          // c[0] is read in. Set the newdata flag low to get next byte
  while (newdata==0)                  // Wait for c[1] to be read in
  {
    c[1] = uart_recieve();
  }
  newdata=0;                          // c[1] is read in and the newdata flag is set low again. End of routine
}

void read_data()
{
  //---------- Read Axis ---------- //
  switch (c[0]&0b11000000)
  {
    case 0b10000000:	// X
    // Disable other axes and enables the X-axis
    STEPPORT &=	~YPIN;
    STEPPORT &= ~ZPIN;
    STEPPORT |= XPIN;
    break;
    
    case 0b01000000:	// Y
    // Disable other axes and enables the Y-axis
    STEPPORT &= ~XPIN;
    STEPPORT &= ~ZPIN;
    STEPPORT |= YPIN;
    break;
    
    case 0b11000000:	// Z
    // Disable other axes and enables the Z-axis
    STEPPORT &= ~XPIN;
    STEPPORT &= ~YPIN;
    STEPPORT |= ZPIN;
    break;
    
    case 0b00000000:	// Toggle
    STEPPORT ^= TPIN;	// Switch the toggle pin
    //	uart_putc(0x04);
    break;
    
    default:
    //  uart_puts_P("switch case error:\n");
    //  uart_putc(c[0]);
    break;
  }
  
  //---------- Read direction ---------- //
  // Set direction pin to high or low
  if (c[0] & 0b00100000)
  {
    STEPPORT |= DIRPIN;
  }
  else
  {
    STEPPORT &= ~DIRPIN;
  }
  
  //---------- Read steps ---------- //
  // Remaining bits are steps to execute on the given axis in the given direction
  ins_step = ((c[0] & 0b00011111)<<8) + c[1];
}

  //---------- Execute steps function ---------- //
void step()
{	
  for (; ins_step > 0 ; ins_step--)	// Count down the counter for each step executed
  {
    STEPPORT &= ~STEPPIN;   // Pull the step pin low to execute step
    _delay_us(STEPPULSE);   // step pulse time					
    STEPPORT |= STEPPIN;    // Pull the step pin high again awaiting next step
    _delay_us(STEPTIME);
  }
  
  STEPPORT &= ~XPIN;        // When all steps are executed on the axis, set all enable pins low again
  STEPPORT &= ~YPIN;
  STEPPORT &= ~ZPIN;
  
  uart_putc(0xAA);          // Send a message to the PC that all steps have been executed, meaning we are ready for new instructions
}

// Run program
int main(void)
{	
  //Initialize uart:
  uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
  
  //Regarding the step port:
  DDRC |= 0b00111111;       // Change the direction of PC0-5 to outputs
  STEPPORT |= STEPPIN;      // Start by pulling the step port high, since we are stepping on the pulse falling edge
  STEPPORT &= ~TPIN;        // Start by pulling the toggle pin for the router spindle low
  
  // Regarding the ENDSTOP:
  DDRD   &=   0b10001011;   // Change the direction for PD2,4-6 to inputs
  EIMSK  |=   0b00000001;   // Enabler interrupt request on INT0 (datasheet p. 73)
  EICRA  |=   0b00000011;   // Interrupt happens on rising edge of INT0 (datasheet p. 72)
  
  sei();                    // Allow global interrupts
    
  // Main program loop
  while(1)
  {
    receive_data();         // Wait until 2 bytes have been received
    read_data();            // Read the received data into the relevant variables and select axis and direction
    step();                 // Execute steps
  }
}
