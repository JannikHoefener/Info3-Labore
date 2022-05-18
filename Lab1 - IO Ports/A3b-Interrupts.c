/*
 * Lab1-3.c
 *
 * Created: 26.04.2022 14:04:12
 * Author : acx211
 * -> https://www.mikrocontroller.net/articles/Interrupt
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define BUTTON_2_PRESS !(PINB & (1<<PINB1))
#define BUTTON_1_PRESS !(PIND & (1<<PIND1))

// \ ignoriert newLine und verhindert compiler error
// letzte 0 => Pentiometer (Drehwiderstand)
// Port4 u 5 ansteuern

#define GREEN_LED_ON() {			\
	PORTC |= 0b111110;				\ 
	PORTD |= ((1<<4) | (1<<5));		\ 
}
#define GREEN_LED_OFF() {			\
	PORTC &= ~(0b111110);			\
	PORTD &= ~((1<<4) | (1<<5));	\
}

#define RED_LED_ON() {				\
	PORTB |= (1<<0);				\
	PORTD |= ((1<<6) | (1<<7));		\	
}
#define RED_LED_OFF() {				\
	PORTB &= ~(1<<0);				\
	PORTD &= ~((1<<6) | (1<<7));	\
}

#define GREEN_LED_TOGGLE {			\
	PORTC ^= ((1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5)); \
	PORTD ^= ((1<<4) | (1<<5));		\
}

#define RED_LED_TOGGLE {			\
	PORTB ^= (1<<0);				\
	PORTD ^= ((1<<6) | (1<<7));		\
}

// Main
int main(void)
{
	init();
	while (1)
	{
		// doin' nothing
	}
}

// Methoden
void init(void)
{
	// Output: LED Bank
	// DDR = Data Direction Register
	//  0 = es ist ein Eingang
	//  1 = es ist ein Ausgang
	// dementsprechend definieren wir an den Positionen unserer LED Bank die LEDs als Ausgänge, den Rest als Eingänge
	DDRB |= 0b1;
	DDRC |= 0b111110;
	DDRD |= 0b11110000;
	//    Pos 76543210
	
	// Input: Button 1
	DDRD &= ~(1<<1);
	PORTD |= 1<<1;
	
	// Definition des Buttons (PD1 als INPUT und Aktivierung des Pull-Up)
	
	// Input: Button 2
	DDRB &= ~(1<<1);
	PORTB |= 1<<1;
	// Definition des Buttons (PB1 als INPUT und Aktivierung des Pull-Up)
	
	// Interrupt
	sei(); // enable global interrupt
	PCICR |= (1<<PCIE2); // enable PB Port D interrupt
	PCMSK2 |= 1<<PCINT17; // enable PB17 interrupt
	PCICR |= (1<<PCIE0); // enable PB Port B interrupt
	PCMSK0 |= 1<<PCINT1; // enable PB1 interrupt
	// documentation page 49 & 56 (12.2.4 PCICR)
}

ISR(PCINT2_vect) {
	GREEN_LED_ON();
	while(BUTTON_1_PRESS) {};
	GREEN_LED_OFF();
}

ISR(PCINT0_vect) {
	RED_LED_ON();
	while(BUTTON_2_PRESS) {};
	RED_LED_OFF();
}