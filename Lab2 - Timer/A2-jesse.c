#define F_CPU 16E6

#include <avr/io.h>
#include <avr/interrupt.h>

#define BUTTON_1_PRESS !(PIND & (1<<PIND1))

static volatile int pressed = 0;

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
	
	//Button 1 als Input festlegen
	DDRD &= ~(1<<1);
	PORTD |= 1<<1;

	// Interrupt //
	sei();
	// Timer0 A Match enable
	TIMSK0 |= (1<<OCIE0A);
	OCR0A = 250;
	// Configure CTC Mode
	TCCR0A |= (1<<WGM01);
	TCCR0A &= ~(1<<WGM00);
	TCCR0B &= ~(1<<WGM02);
	//Prescaler on 64
	TCCR0B |= 0b11;
	TCCR0B &= ~(1<<2); 
}

ISR(TIMER0_COMPA_vect){
	static volatile uint8_t counter = 0;
	static volatile uint8_t pressed = 0;
	if (BUTTON_1_PRESS) {
		if (!pressed)
		{
			counter++;
			if (counter == 50)
			{
				next_led();
				counter = 0;
				pressed = 1;
			}
		}
	} else
	{
		counter = 0;
		pressed = 0;
	}
}

void next_led() {
	static volatile uint8_t led_nr = 0;
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
