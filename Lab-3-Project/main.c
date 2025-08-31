#include <platform.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <uart.h>
#include <string.h>
#include "queue.h"
#include <gpio.h>
#include "leds.h"
#include <timer.h>

#define BUFF_SIZE 128
#define CF 1000000

int period = 2;		//An timer interrupt will occur every period seconds.
int count = 0;		//How many times the user's button is pressed
int period_counter = 0;		//How many times the timer_callback_isr is called
char buff[BUFF_SIZE];		//Necessary address space for the display through uart

/********Functions for creating delay in milisecond and in microseconds as well.*******************/
void delay_ms(unsigned int ms);

void delay_us(unsigned int us);

void delay_cycles(unsigned int cycles);


void delay_ms(unsigned int ms) {
	unsigned int max_step = 1000 * (UINT32_MAX / SystemCoreClock);
	unsigned int max_sleep_cycles = max_step * (SystemCoreClock / 1000);
	while (ms > max_step) {
		ms -= max_step;
		delay_cycles(max_sleep_cycles);
	}
	delay_cycles(ms * (SystemCoreClock / 1000));
}

void delay_us(unsigned int us) {
	unsigned int max_step = 1000000 * (UINT32_MAX / SystemCoreClock);
	unsigned int max_sleep_cycles = max_step * (SystemCoreClock / 1000000);
	while (us > max_step) {
		us -= max_step;
		delay_cycles(max_sleep_cycles);
	}
	delay_cycles(us * (SystemCoreClock / 1000000));
}

__asm void delay_cycles(unsigned int cycles) {
	LSRS r0, #2
	BEQ done
loop
	SUBS r0, #1
#if __CORTEX_M == 3 || __CORTEX_M == 4
	NOP
#endif
	BNE loop
done
	BX lr
}




uint8_t Rh_byte1, Rh_byte2, temp_byte1, temp_byte2;
uint16_t SUM, RH, TEMP;

float Temperature = 0;
float Humidity = 0;
uint8_t Presence = 0;


#define DHT11_PIN PA_1

void DHT11_Start(void){
	gpio_set_mode(DHT11_PIN, Output);		// set the pin as output
	gpio_set(DHT11_PIN, 0);		// set the pin low
	delay_us(18000);		// wait for 18ms 
	gpio_set(DHT11_PIN,1);		// set the pin high
	delay_us(20); 		//wait for 20 us
	gpio_set_mode(DHT11_PIN, Input);
	gpio_set_trigger(DHT11_PIN, Falling); // We choose falling based on the scheme of the sensor 
	
}

uint8_t DHT11_Check_Response(void){
	uint8_t Response =0;
	delay_us(40);
	if(!(gpio_get(DHT11_PIN))){
		delay_us(80);
		if(gpio_get(DHT11_PIN)){
			Response = 1;		//The sensor is working.
		}
		else{
			Response = 0;		//The sensor is malfunctioning.
		}
	}
	while(gpio_get(DHT11_PIN)); // Waiting for the pin to become 0
	
	return Response;
	
}

uint8_t DHT11_Read(void){
	uint8_t i, j;
	for(j=0;j<8;j++){
		while(!(gpio_get(DHT11_PIN)));		// Wait for  pin to become 1
		delay_us(40);		//Wait for 40 us
		if(!(gpio_get(DHT11_PIN))){		//If the pin is 0
			i&= ~(1<<(7-j));		// Write 0
		}
		else{
			i |= (1<<(7-j));		//Write 1
		}
		while(gpio_get(DHT11_PIN));		//Wait for the pin to become 0
		
	}
	return i;
}


float Read_data(){
	DHT11_Start();
	Presence = DHT11_Check_Response();
		
	if(Presence){
		Rh_byte1 = DHT11_Read();		//8bit integral Humidity data
		Rh_byte2 = DHT11_Read();		//Next 8bit are decimal Humidity data
		temp_byte1 = DHT11_Read(); 	//next 8bit are integral Temperature data
		temp_byte2 = DHT11_Read();	//Next 8bit are decimal Temperature data
		SUM = DHT11_Read();					//Last 8 bits are the checksum
		
		TEMP = temp_byte1;
		RH = Rh_byte1;
		
		Temperature = (float) TEMP;
		Humidity = (float) RH;
			
		delay_us(1000);
		
		return Temperature;
	
	}
}


void timer_callback_isr(void){
	period_counter++;
	if(period==period_counter){
		
			float T=0;
			T = Read_data();
			sprintf(buff, "The temperature is %f and the period for reading the sensor output is %d \r\n", T, period);
			uart_print(buff);
			period_counter=0;
		}
	

	
}
	

void button_press_isr(int sources) {

	if ((sources << GET_PIN_INDEX(P_SW)) & (1 << GET_PIN_INDEX(P_SW))) {
		
		if(count == 0){
			period = 12;		// AEM:8675 so the add of the last 2 digits is 12
			timer_init(CF);
		}
		else{
			if((count%2)==0){		// The count is an even number
				period = 4;
				timer_init(CF);		//Changing the period of the timer interrupt
			}
			else{								// The count is an odd number
				period = 3;
				timer_init(CF);		//Changing the period of the timer interrupt
			}
		}
		count++;
	}
}



int main() {
	
	//Making all the necessary initializations
	leds_init();
	uart_init(57600);
	uart_enable();
	leds_set(0,0,0);
	gpio_set_mode(P_SW, PullUp);
	gpio_set_trigger(P_SW, Rising);
	gpio_set_callback(P_SW, button_press_isr);
	timer_init(CF);		//TICK every period seconds
	timer_set_callback(timer_callback_isr);
	timer_enable();
	
	__enable_irq();
	
	
	while(1){
		
		if(Temperature > 25){
			leds_set(1,0,0);
		}
		else if(Temperature <20){
			leds_set(0,0,1);
		}
		else{
			leds_set(0,1,0);
		}
	}
}
