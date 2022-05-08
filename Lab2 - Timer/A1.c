/*
 * GccApplication1.c
 *
 * Created: 04.05.2022 13:41:35
 * Author : acx211
 */ 

#define F_CPU 16E6

#include <avr/io.h>
#include <avr/interrupt.h>

int main(void)
{
	init();
	while(1){}// While True ...
}

void init(void){
	// LEDs als Output definieren
	// DDRx - DataDirektionRegister 
	// => gibt die Richtung an in welche Richtung die Daten fließen
	//BitPos  76543210	
	DDRB |=        0b1;
	DDRC |=   0b111110; 
	DDRD |= 0b11110000;

	// Interrupt (Timer = Interupt-Funktion)
	sei(); // Interrupts generell aktivieren 
	TIMSK0 |= (1<<OCIE0A); 
	OCR0A = 155;
	TCCR0A |= (1<<WGM01); 
	TCCR0A &= ~(1<<WGM00);
	TCCR0B &= ~(1<<WGM02);
	TCCR0B |=(1<<CS02) | (1<<CS00); 
	TCCR0B &= ~(1<<CS01);
}

ISR(TIMER0_COMPA_vect){
	static volatile uint8_t counter = 0;
	static volatile uint8_t led_nr = 0;
	counter++;
	if (counter==100){
		counter = 0;
		if (led_nr < 5) {
			PORTB &= ~(0b1);
			PORTC &= ~(0b111110);
			PORTC = (1 << (led_nr + 1));
		}
		else if(led_nr < 9) {
			PORTC &= ~(0b111110);
			PORTD &= ~(0b11110000);
			PORTD = (1 << (led_nr - 1));
			} else {
			PORTD &= ~(0b11110000);
			PORTB &= ~(0b1);
			PORTB = 1;
		}
		led_nr = (led_nr + 1) % 10;
	}
}
