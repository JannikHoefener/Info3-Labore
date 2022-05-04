#define F_CPU 16E6

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED1_ON	PORTC |= 1<<PORTC1; // LED 1 - 10 ON
#define LED2_ON	PORTC |= 1<<PORTC2; 
#define LED3_ON	PORTC |= 1<<PORTC3;
#define LED4_ON	PORTC |= 1<<PORTC4;
#define LED5_ON	PORTC |= 1<<PORTC5;
#define LED6_ON	PORTD |= 1<<PORTD4;
#define LED7_ON	PORTD |= 1<<PORTD5;
#define LED8_ON	PORTD |= 1<<PORTD6;
#define LED9_ON	PORTD |= 1<<PORTD7;
#define LED10_ON PORTD |= 1<<PORTD0;

void init(void) {
	// Interrupt Timer	
	sei();
	TIMSK0 |= (1<<OCIE0A); // Timer0 A MAtch enable
	OCR0A = 155; // ~10ms
	TCCR0A &= ~(1<<WGM00); // Set CTC Mode
	TCCR0A |= (1<<WGM01);
	TCCR0B &= ~(1<<WGM02);
	TCCR0B |= (1<<CS00); // Set Prescaler
	TCCR0B &= ~(1<<CS01);
	TCCR0B |= (1<<CS02);
}

void allLEDOff() {
	PORTC = 0;
	PORTD = 0;
}

void nextLED() {
	static volatile uint8_t ledCounter = 0;

	allLEDOff();
	ledCounter++;
	switch (ledCounter) {
	case 1:
		LED1_ON; break;
	case 2:
		LED2_ON; break;
	case 3:
		LED3_ON; break;
	case 4:
		LED4_ON; break;
	case 5:
		LED5_ON; break;
	case 6:
		LED6_ON; break;
	case 7:
		LED7_ON; break;
	case 8:
		LED8_ON; break;
	case 9:
		LED9_ON; break;
	case 10:
		LED10_ON; 
		ledCounter = 0;
		break;
	default:
		break;
	}
}

void timerAction() {
	static volatile uint8_t counter = 0;

	counter++;
	if(counter == 10) {
		nextLED();
		counter = 0;
	}
}

ISR(TIMER0_COMPA_vect){
	timerAction();
}

int main(void){
	init();
	while (1){}
}