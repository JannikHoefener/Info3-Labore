/*
 * CountButtonAndBlink
 * 
 * 
 * Created: 05.04.2022 13:45:42
 * Author: Jannik Hoefener, Soeren Helms
 */ 

#define F_CPU 16E6
#include <avr/io.h>
#include <util/delay.h>

#define LED_ON PORTB |= (1<<5)
#define LED_OFF PORTB &= ~(1<<5)
#define LED_TOGGLE PINB |= (1<<5)
#define BUTTON_PRESS (!(PINB & (1<<7)))

int pressed = 0;   // define pressed (counter) as integer worth zero

void init()
{
	// LED
	DDRB |= (1<<5);  // Configure PB5 as Output
	LED_ON;
	
	// Button
	DDRB &= ~(1<<7); //Configure PB1 as Input
	PORTB |= 1<<7;   //Enable Internal Pull-Up at PB1
	asm("nop");
}

void check_button()
{
	if(BUTTON_PRESS)
	{
		pressed++;
		blinkntimes(pressed);
		// counter goes up, blink programm is called with param how often it should blink
	} else {
		LED_OFF;
	}
}

void blinkntimes(int pressed)
{
	for(int i = 0; i < pressed; i++)
	{
		LED_ON;
		_delay_ms(250);
		LED_OFF;
		_delay_ms(250);
	}
}

int main()
{
	init();
	LED_OFF;
	while (1)
	{
		check_button();
	}
}