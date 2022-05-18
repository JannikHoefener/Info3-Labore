/* 2. Aufgaben-Stellung
Das LED Lauflicht aus Versuch 1 soll nun manuell durch eine Taster-Betätigung weiter geschaltet werden und nicht mehr automatisch mit der Zeit durchlaufen. 
Hierbei gilt:
- Eine Taster-Betätigung schaltet das Lauflicht um genau eine LED weiter (entprellen!).
- Wird der Taster gedrückt gehalten, darf kein Lauflicht entstehen.
 */  
#define F_CPU 16E6

#include <avr/io.h>
#include <avr/interrupt.h>

#define BUTTON_1_PRESS !(PIND & (1<<PIND1))

static volatile int pressed = 0;

int main()
{
	init();
	while(1){}
}

void init()
{
	// LED Bank as Outputs
	DDRB |=        0b1;
	DDRC |=   0b111110; 
	DDRD |= 0b11110000;
	// Button 1 as Input
	// DDRD &= ~(1<<1);
	PORTD |= 1<<1;

	// Interrupt enable
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

ISR(TIMER0_COMPA_vect)
{
	static volatile uint8_t counter = 0;
	
	// checking every millisecond if button is pressed
    if (BUTTON_1_PRESS == 1)
	// if button is pressed, the counter is increased by one
    {
		counter++;
		if (counter == 50)
		// if the button is pressed for 50 ms the nextLED() can be activated
		// depending on order of command it switches on PRESS DOWN or on LET LOOSE
		{
			counter = 0;
			nextLED();
			while (BUTTON_1_PRESS == 1) {}
		}
	}
	else 
	// if not, the counter is reset immediatly
	{
		counter = 0;
	}
}

// core idea same as A1
void nextLED()
{
    static volatile uint8_t led_nr = 0;
    if (led_nr < 5)
    {
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