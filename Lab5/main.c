/*
 * Lab5.c
 *
 * Created: 4/12/2023 4:24:31 PM
 * Author : stlondon, mpass
 */ 

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "uart.h"


/* Define UART buad rate here */
#define UART_BAUD_RATE      9600      


void itoa16(uint16_t value, char *str)
{
	// Buffer to store the ASCII string
	char buffer[6];

	int8_t index = 0;

	// Convert value to ASCII
	do
	{
		buffer[index++] = '0' + (value % 10);
		value /= 10;
	} while (value > 0);

	// Copy characters from buffer to str in reverse order
	while (index > 0)
	{
		*str++ = buffer[--index];
	}

	// Null-terminate the string
	*str = '\0';
}

void ADC_Init()
{
	// Set ADC reference to AVCC (5V)
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << REFS1);

	// Enable ADC and set ADC prescaler to 128 for 16MHz system clock
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Function to read analog value from ADC0
uint16_t ADC_Read()
{
	// Start ADC conversion
	ADCSRA |= (1 << ADSC);

	// Wait for ADC conversion to complete
	while (ADCSRA & (1 << ADSC));

	// Return ADC value
	return ADC;
}


int main(void)
{
    unsigned int c;
	uint16_t ADC_value;
	
    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) ); 
    
    /*
     * now enable interrupt, since UART library is interrupt controlled
     */
    sei();
	
	ADC_Init();
   
    uart_puts("Welcome to program.\n");
	uart_puts("Please enter command:\n");
    
        
    /* 
     * Use standard avr-libc functions to convert numbers into string
     * before transmitting via UART
    
    itoa( num, buffer, 10);   // convert interger into string (decimal format)         
    uart_puts(buffer);        // and transmit string to UART

	*/
    
    
    while(1)
    {
        
		
        c = uart_getc();
		ADC_value = ADC_Read();
		
		char digitString[7];
		
		itoa16(ADC_value, digitString);
		
		uart_puts(digitString);
		
		uart_putc('\n');
		
        if ( c & UART_NO_DATA )
        {
            /* 
             * no data available from UART 
             */
        }
        else
        {
            /*
             * new data available from UART
             * check for Frame or Overrun error
             */
            if ( c & UART_FRAME_ERROR )
            {
                /* Framing Error detected, i.e no stop bit detected */
                uart_puts_P("UART Frame Error: ");
            }
            if ( c & UART_OVERRUN_ERROR )
            {
                /* 
                 * Overrun, a character already present in the UART UDR register was 
                 * not read by the interrupt handler before the next character arrived,
                 * one or more received characters have been dropped
                 */
                uart_puts_P("UART Overrun Error: ");
            }
            if ( c & UART_BUFFER_OVERFLOW )
            {
                /* 
                 * We are not reading the receive buffer fast enough,
                 * one or more received character have been dropped 
                 */
                uart_puts_P("Buffer overflow error: ");
            }
            /* 
             * send received character back
             */
            uart_putc( (unsigned char)c );
        }
		
		_delay_ms(1000.0);
		
		
		
		
    }
    
}
