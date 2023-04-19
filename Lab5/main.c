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
#include <stdio.h>
#include <math.h>
#include <avr/io.h>
#include <stdbool.h>


#include "uart.h"
#include "i2cmaster.h"


// Define UART baud rate here
#define UART_BAUD_RATE      9600      

// Define i2c address
#define I2C_DEVICE 0x58

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

	// Enable ADC and set ADC pre-scaler to 128 for 16MHz system clock
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

float convertToFloat(uint16_t input){
	return (((float)input) / 1024.0) * 5.0;
}

void float_to_ascii(float num, char* str) {
	int32_t whole_part = (int32_t) num;
	int32_t fractional_part = (int32_t) ((num - whole_part) * 100.0); // Gives 2 digits of precision. Increase by multiple of 10 to increase number of digits displayed.
	uint8_t i = 0;
	
	bool isNegative = false;
	
	// Note: Digits are calculated in reverse order.
	// First the fractional part is calculated.
	// Then the whole part is calculated.

	if (num < 0.0) {
		isNegative = true;
		num = -num;
		whole_part = (int32_t) num;
		fractional_part = (int32_t) ((num - whole_part) * 1000.0); // Gives 3 digits of precision. Increase by multiple of 10 to increase number of digits displayed.
	}
	
	while (fractional_part > 0) {
		str[i++] = (fractional_part % 10) + '0';
		fractional_part /= 10;
	}
	
	if (i == 0){
		// Must have at least one decimal point.
		str[i++] = '0';
	}

	str[i++] = '.';
	
	uint8_t j = 0;

	while (whole_part > 0) {
		str[i + j++] = (whole_part % 10) + '0';
		whole_part /= 10;
	}

	if (j == 0) {
		str[i + j++] = '0';
	}

	i = i + j;

	
	if (isNegative){
		str[i++] = '-';
	}
	
	str[i] = '\0';

	uint8_t k = 0;
	j = i - 1;

// Reverse digits. Because digits are calculated least-significant to most-significant, but are displayed most-significant to least-significant, the digits must be reversed.

	while (k < j) {
		char temp = str[k];
		str[k++] = str[j];
		str[j--] = temp;
	}

}

float combineDigits(int ones, int tenths, int hundredths){
	return ones + (((float) tenths)/10.0) + (((float) hundredths)/100.0);
}

unsigned char convertFloatToChar(float input){
	return (input / 5.0) * 255;
	};

unsigned int read_UART(){
	
	// uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU) );
	unsigned int c = uart_getc();
	
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
			
        }
		
		return c;
	 
	
}


int main(void)
{
    unsigned int c;
	uint16_t ADC_value;
	
	uint8_t sinewave[64] = {128, 141, 153, 165, 177, 188, 199, 209, 219, 227, 234, 241, 246, 250, 254, 255, 255, 255, 254, 250, 246, 241, 234, 227, 219, 209, 199, 188, 177, 165, 153, 141, 128, 115, 103, 91, 79, 68, 57, 47, 37, 29, 22, 15, 10, 6, 2, 1, 0, 1, 2, 6, 10, 15, 22, 29, 37, 47, 57, 68, 79, 91, 103, 115};
	
    uart_init( UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU)); 
    
    /*
     * now enable interrupt, since UART library is interrupt controlled
     */
    sei();
	
	ADC_Init();
	i2c_init();
	
   
    uart_puts("Welcome to program.\n");
	uart_puts("Please enter command:\n");
    
   
    
    while(1){
		
		c = read_UART();
		
		if(c & UART_NO_DATA){
			continue;
		}
		
		if(c == 'G'){
			// Single voltage mode.
			char digitString[20];
			ADC_value = ADC_Read();
			float voltage = convertToFloat(ADC_value);
			float_to_ascii(voltage, digitString);
			uart_puts("v=");
			uart_puts(digitString);
			uart_puts(" V\n");
			
			while(!(c & UART_NO_DATA)){
				c = read_UART(); // Read remaining characters until buffer is empty.
				uart_putc(c);
			}
			
		} else if (c == 'W'){
			
			_delay_ms(10.0);
			c = read_UART();
			
			if(c != ","){
				uart_puts("Error. Command not recognized. Please try again.\n");
			}
			
	
			
			for(int j = 0; j < 100; ++j){
				for(int i = 0; i < 64; ++i){
					i2c_start(I2C_DEVICE+I2C_WRITE);
					i2c_write(0x00);
					i2c_write(sinewave[i]);
					i2c_stop();
					_delay_ms(1.25);
				}
			}
			
			
		} else if (c == 'T'){
			
			_delay_ms(10.0);
			
			c = read_UART();
			
			if(c & UART_NO_DATA){
				uart_puts("No data!\n");
			}
			
			if (c == 'c'){
				uart_puts("Success!\n");
			} else{
				uart_puts("Failure.\n");
			}
			
		}
		else if (c == 'S'){
			// Set DAC output voltage
			
			_delay_ms(10.0);
			c = read_UART();
			
			if(c != ','){
				uart_puts("Error. Command not recognized. Please try again.\n");
			}
			
			int DAC_Channel;
			
			_delay_ms(10.0);
			c = read_UART();
			
			if(c == '0'){
				DAC_Channel = 0;
				} else if (c == '1'){
				DAC_Channel = 1;
				} else{
				uart_puts("Error. Command not recognized. Please try again.\n");
				continue;
			}
			
			int ones, tenths, hundredths;
			
			_delay_ms(10.0);
			c = read_UART(); // Expecting second comma
			
			if(c != ','){
				uart_puts("Error. Command not recognized. Please try again.\n");
				continue;
			}
			
			_delay_ms(10.0);
			c = read_UART(); // Get first digit.
			
			if(!(c >= 48 && c <= 57)){ // c is not a digit
				uart_puts("Error. Command not recognized. Please try again.\n");
				continue;
			}

			ones = c - 48; // Convert ASCII to digit.
			
			_delay_ms(10.0);
			c = read_UART(); // Expecting .
			
			if(c != '.'){
				uart_puts("Error. Command not recognized. Please try again.\n");
				continue;
			}
			
			
			_delay_ms(10.0);
			c = read_UART(); // Expecting tenths.
			
			if(!(c >= 48 && c <= 57)){ // c is not a digit
				uart_puts("Error. Command not recognized. Please try again.\n");
				continue;
			}
			
			tenths = c - 48;
			
			_delay_ms(10.0);
			c = read_UART();
			
			if(!(c >= 48 && c <= 57)){ // c is not a digit
				uart_puts("Error. Command not recognized. Please try again.\n");
				continue;
			}
			
			hundredths = c - 48;
			
			// Empty remaining UART buffer
			while(!(c & UART_NO_DATA)){
				_delay_ms(10.0);
				c = read_UART();
			}
			
			float valueToConvert = combineDigits(ones, tenths, hundredths);
			
			unsigned char DAC_value = convertFloatToChar(valueToConvert);
			
			if(DAC_Channel == 0){
				
				i2c_start(I2C_DEVICE+I2C_WRITE);
				i2c_write(0x00);
				i2c_write(DAC_value);
				i2c_stop();
				
			}
			else{
				i2c_start(I2C_DEVICE+I2C_WRITE);
				i2c_write(0x01);
				i2c_write(DAC_value);
				i2c_stop();
			}
			
			char DAC_string[5];
			
			uart_puts("DAC channel ");
			uart_putc(DAC_Channel + 48);
			uart_puts(" set to ");
			uart_putc(ones + 48);
			uart_putc('.');
			uart_putc(tenths + 48);
			uart_putc(hundredths + 48);
			uart_puts(" V (");
			itoa16(DAC_value, DAC_string);
			uart_puts(DAC_string);
			uart_puts("d)\n");
			
			
			
		}
		/*
		else if (c == 'W'){
			
			for(int i = 0; i < 64; ++i){
				i2c_start(I2C_DEVICE+I2C_WRITE);
				i2c_write(0x00);
				i2c_write(sinewave[i]);
				i2c_stop();
				_delay_ms(1.25);
				
			}
			
		}
		*/
		
		/*
		else {
			uart_puts("Error. Command not recognized. Please try again.\n");
			continue;
		}*/
        
		
		
	}
	
}
