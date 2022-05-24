/* 1. Aufgaben-Stellung
Entwerfen und schreiben Sie ein C-Programm , das die 10 LEDs des Shields als Lauflicht ansteuert.
- Dabei soll immer nur eine LED für ca. 1 Sekunde aufleuchten, dann soll die LED ausgeschaltet werden und die Nachbar-LED soll aufleuchten, usw.
- Sind alle 10 LEDs einmal durchlaufen, so beginnt das Lauflicht wieder von vorne bei der ersten LED.
- Nutzen Sie einen Timer statt einer „delay“-Library Funktion
*/

#define F_CPU 16E6 // welche CPU, eig nicht nötig, da kein delay

#include <avr/io.h>
#include <avr/interrupt.h>

int main()
{
	init();
	while(1) {} // While True ...
}

void init()
{
	// LEDs als Output definieren
	// DDRx - DataDirektionRegister 
	// => gibt die Richtung an in welche Richtung die Daten fließen
	//BitPos  76543210	
	DDRB |=        0b1;
	DDRC |=   0b111110; 
	DDRD |= 0b11110000;

	// Interrupt (Timer = Interupt-Funktion)
	sei(); // Interrupts generell enable
	// Timer0 A Match enable
	TIMSK0 |= (1<<OCIE0A); 
	OCR0A = 155; //OCR => wann match
	// Configure CTC (Clear Timer on Compare) Mode
	TCCR0A |= (1<<WGM01); 
	TCCR0A &= ~(1<<WGM00);
	TCCR0B &= ~(1<<WGM02);
	// Prescaler konfigurieren (CPU Faktor reduzieren)
	TCCR0B |=(1<<CS02) | (1<<CS00); 
	TCCR0B &= ~(1<<CS01);
}

// wenn interupt erreicht wird
ISR(TIMER0_COMPA_vect)
{
	static volatile uint8_t counter = 0;
	static volatile uint8_t led_nr = 0;
    // unsignet(kein Vorzeichen) int 8Bit_typ
    // volatile bei Interupts notwendig
	
    counter++;
	if (counter==100) // 100*10ms = 1sek
    { 
		counter = 0;
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
}
