#define F_CPU16E6

#include <avr/io.h>
#include <avr/interrupt.h>

int main(void)
{
    init();
    while(1){}
}

void init(void){
	// LEDs as Outputs
	DDRC |= 0b111110;
	DDRD |= 0b11110000;
	DDRB |= (1<<0);

	// Interrupt
	sei();
	TIMSK0 |= (1<<OCIE0A); // Timer0 A Match enable
	OCR0A = 155;
	TCCR0A |= (1<<WGM01); // Configure CTC Mode
	TCCR0A &= ~(1<<WGM00);
	TCCR0B &= ~(1<<WGM02);
	TCCR0B |=(1<<CS02) | (1<<CS00); // Prescaler 1024
	TCCR0B &= ~(1<<CS01);
}

ISR(TIMER0_COMPA_vect){
	static volatile uint8_t counter = 0;
	static volatile uint8_t led_nr = 0;
	counter++;
	if (counter==100){
		counter = 0;
		if (led_nr < 5) {
			PORTB &= ~(1<<0);
			PORTC &= ~(0b111110);
			PORTC = (1 << (led_nr + 1));
		}
		else if(led_nr < 9) {
			PORTC &= ~(0b111110);
			PORTD &= ~(0b11110000);
			PORTD = (1 << (led_nr - 1));
		} else {
			PORTD &= ~(0b11110000);
			PORTB &= ~(1<<0);
			PORTB = 1;
		}
		led_nr = (led_nr + 1) % 10;
	}
}