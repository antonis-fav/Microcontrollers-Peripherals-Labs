#include <platform.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <uart.h>
#include <string.h>
#include "queue.h"
#include <gpio.h>
#include "leds.h"



#define BUFF_SIZE 128

int b, count;

Queue rx_queue;

// A function that handles the user input, by storing every character into a queue
void uart_rx_isr(uint8_t rx) {
	if ((rx >= 'a' && rx <= 'z') || rx == 0x7F || rx == '\r') {
		// Store the received character
		queue_enqueue(&rx_queue, rx);
	}
}


void button_press_isr(int sources) {

	if ((sources << GET_PIN_INDEX(P_SW)) & (1 << GET_PIN_INDEX(P_SW))) {
		// With this variable we keep track how many times the USER button has been pushed
		count++;
		if(b == 1){
			leds_set(0,0,0);		//Turning leds off.
			uart_print("Interrupt caught -> All the leds are turned off \r\n");
			uart_print("Enter your full name:");
		}else{
			leds_set(0,0,1);		//Turning the led blue.
			uart_print("Interrupt caught -> The blue led is on \r\n");
			uart_print("Enter your full name:");
		}
		b=1;
	}
}



int main() {
	uint8_t rx_char = 0;
	char buff[BUFF_SIZE];
	uint32_t buff_index;
	char my_string[20];
	char c;
	
	queue_init(&rx_queue, 128);
	uart_init(57600);
	uart_set_rx_callback(uart_rx_isr);
	uart_enable();
	
	leds_init();
	leds_set(0,0,0);
	
	gpio_set_mode(P_SW, PullUp);
	gpio_set_trigger(P_SW, Rising);
	gpio_set_callback(P_SW, button_press_isr);
	
	__enable_irq();
	
	uart_print("\r");
	while(1) {

		uart_print("Enter your full name:");
		buff_index = 0;
		do {
			while (!queue_dequeue(&rx_queue, &rx_char))
				__WFI();
			if (rx_char == 0x7F) { // Handle backspace character
				if (buff_index > 0) {
					buff_index--;
					uart_tx(rx_char);
				}
			} else {
				// Store and echo the received character back
				buff[buff_index++] = (char)rx_char;
				uart_tx(rx_char);
			}
		} while (rx_char != '\r' && buff_index < BUFF_SIZE);
		// Replace the last character with \0
		buff[buff_index - 1] = '\0';
		uart_print("\r\n");
		
		
		if (buff_index < BUFF_SIZE) {
			// Convert buffer contents to float
			if (sscanf(buff, "%s", my_string) == 1) {
				
				sprintf(buff, "The last character of string %s is %c and count is %d.\r\n", my_string, my_string[strlen(my_string)-1], count);
				c = my_string[strlen(my_string)-1];
				
				uart_print(buff);
				
				if(c == 'a' || c == 'e' || c== 'i' || c == 'o' || c == 'u' || c == 'y' || c == 'w'){
					uart_print("The last character is a vowel \r\n");
					uart_print("The red led is on \r\n");
					leds_set(1,0,0);
					b=0;
					
				} else{
					uart_print("The last character is a consonant \r\n");
					uart_print("The green led is on \r\n");
					leds_set(0,1,0);
					b=0;
					}
					
			} else {
				
				
				uart_print(buff);
				uart_print("The input name is not valid!\r\n");
				
			}
		} else {
			uart_print("Stop trying to overflow my buffer! I resent that!\r\n");
		}
	}
}

