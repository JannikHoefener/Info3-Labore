#define F_CPU 16E6

#include <avr/io.h>
#include <avr/interrupt.h>

int main(void)
{
    init();
    while(1){}
}

void init(void){
	// Output: LED Bank
    // DDR = Data Direction Register
    //  0 = es ist ein Eingang
    //  1 = es ist ein Ausgang
    // dementsprechend definieren wir an den Positionen unserer LED Bank die LEDs als Ausgänge, den Rest als Eingänge
	DDRB |= 0b1;
	//    Pos 0
	DDRC |= 0b111110;
	//    Pos 543210
	DDRD |= 0b11110000;
	//    Pos 76543210

	// Interrupt - see page 186 in documentation for more details about Timers
	sei();								// Interrupts aktivieren
	TIMSK0 |= (1<<OCIE0A); 				// Timer0 A Match enable
	OCR0A = 155;						// 
	TCCR0A |= (1<<WGM01); 				// Configure CTC Mode
	TCCR0A &= ~(1<<WGM00);				// 
	TCCR0B &= ~(1<<WGM02);				//
	TCCR0B |=(1<<CS02) | (1<<CS00); 	// Prescaler 1024
	TCCR0B &= ~(1<<CS01);				// 
}

ISR(TIMER0_COMPA_vect){
	static volatile uint8_t counter = 0;
	static volatile uint8_t position = 0;
	counter++;
	if (counter==100){					// 1 sec = 1000 ms, if 100 is required, 1 step should be 10 ms
		counter = 0;					// reset the counter to wait for another 100 steps (1 sec)
		// different handling, depending on position (in the light array)
		if (position < 5) {				// 0 1 2 3 4
			PORTB &= ~(0b1);
			PORTC &= ~(0b111110);
			PORTC = (1 << (position + 1));
		}
		else if(position < 9) {			// 5 6 7 8
			PORTC &= ~(0b111110);
			PORTD &= ~(0b11110000);
			PORTD = (1 << (position - 1));
		} else {						// 9
			PORTD &= ~(0b11110000);
			PORTB &= ~(0b1);
			PORTB = 1;
		}
		position = (position + 1) % 10;		// makes sure, that position stays in range 0 to 9
	}
}