/*
 * CountButtonAndBlink with Shield
 * 
 * 
 * Created: 05.04.2022 13:45:42
 * Author: Jannik Hoefener, Soeren Helms
 */ 

#define F_CPU 16E6
#include <avr/io.h>
#include <util/delay.h>

#define LED_ON PORTC |= (1<<1)
#define LED_OFF PORTC &= ~(1<<1)
#define LED_TOGGLE PINC |= (1<<1)
#define BUTTON_INCREMENT_PRESS (!(PIND & (1<<1)))
#define BUTTON_RESET_PRESS (!(PINB & (1<<1)))

int pressed = 0;   // define pressed (counter) as integer worth zero

void init()
{
	// LED
	DDRC |= (1<<1);  // Configure ~PB5~ PC1 as Output
	LED_ON;
	
	// Button Increment
	DDRD &= ~(1<<1); //Configure ~PB7~ PD1 as Input
	PORTD |= 1<<1;   //Enable Internal Pull-Up at ~PB7~ PD1
	
	// Button Reset
	DDRB &= ~(1<<1); //Configure PB1 as Input
	PORTB |= 1<<1;   //Enable Internal Pull-Up at PB1
	asm("nop");
}

void check_increment_button()
{
	if(BUTTON_INCREMENT_PRESS)
    {
		pressed++;
		blinkntimes(pressed);
		// counter goes up, blink programm is called with param how often it should blink
	} else {
		LED_OFF;
	}
}

void check_reset_button()
{
	if(BUTTON_RESET_PRESS)
    {
		pressed = 0;
		// resetting the pressed counter

		// <== Ansatz
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
		check_increment_button();
		check_reset_button();
	}
}