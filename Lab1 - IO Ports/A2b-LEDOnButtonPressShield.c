/*
 * LEDOnButtonPress with Shield
 * Will light a LED while the button is pressed
 * 
 * Created: 05.04.2022 13:45:42
 * Author: Jannik Hoefener, Soeren Helms
 */ 

#include <avr/io.h>

#define LED_ON PORTC|= (1<<1)
#define LED_OFF PORTC &= ~(1<<1)
#define LED_TOGGLE PINC |= (1<<1)
#define BUTTON_PRESS (!(PIND & (1<<1)))

void init()
{
	// LED
	DDRC |= (1<<1);  // Configure ~PB5~ PC1 as Output
	LED_ON;
	
	// Button
	DDRD &= ~(1<<1); //Configure ~PB7~ PD1 as Input
	PORTD |= 1<<1;   //Enable Internal Pull-Up at ~PB7~ PD1
	asm("nop");
}

void check_button()
{
	if(BUTTON_PRESS)
	LED_ON;
	else
	LED_OFF;
}

int main()
{
	init();
	while (1)
	{
		check_button();
	}
}