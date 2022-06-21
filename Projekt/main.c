/******************************************************************************
* Projektcode auf Basis des Display Projekts aus EMIL
;******************************************************************************/

#define F_CPU 16E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "spi.h"
#include "tft.h"
#include "LED.h"
#include "buzzer.h"
#include "display.h"

#define BUTTON_2_PRESS !(PINB & (1<<PINB1))
#define BUTTON_1_PRESS !(PIND & (1<<PIND1))

uint16_t i; //TODO what used for? bad name
unsigned int state = 0; //TODO state..
unsigned int timer = 0; 
unsigned int messwert = 1200; //TODO german and what used for?

unsigned int pausenzeit = 6;// default 300 s für 5 Min TODO german
unsigned int endzeit = 10;	// default 10 s TODO german

void SPI_init()
{
	//set CS, MOSI and SCK to output
	SPI_DDR |= (1 << CS) | (1 << MOSI) | (1 << SCK);

	//enable SPI, set as master, and clock to fosc/4 or 128
	SPCR = (1 << SPE) | (1 << MSTR);// | (1 << SPR1) | (1 << SPR0); 4MHz bzw. 125kHz
	//SPSR |= 0x1;
}

void SPISend8Bit(uint8_t data){
	PORTB &= ~(1<<CS);				//CS low
	SPDR = data;					//load data into register
	while(!(SPSR & (1 << SPIF)));	//wait for transmission complete
	PORTB |= (1<<CS);				//CS high
}

void init(void){
	wdt_disable();
	// alles was einmal zum start erledigt werden muss
	DDRD |= (1<<D_C)|(1<<Reset);		//output: PD2 -> Data/Command; PD3 -> Reset
	SPI_init();
	sei();
	initDisplay();
	//Display-Hintergrundfarbe übertragen:
	int x;
	for(x=0; x<23232; x++){ //TODO magic number and shouldnt be that in init from display?
		SPISend8Bit(0xFF); 				//senden von 1. 8-Bit-Wert für weiß
		SPISend8Bit(0xFF); 				//senden von 2. 8-Bit-Wert für weiß
	}
	// Übertragen der Überschrift auf das Display
	char mytext[]  = "Pomodoro-Timer";
	TFT_Print(&mytext[0] /*TODO reicht es nicht ohne [0] ? $*/, 4, 6, 2, TFT_16BitRed, TFT_16BitWhite, TFT_Landscape180);
	
	// Buttons als Input setzen
	DDRD &= ~(1<<1);
	PORTD |= 1<<1;
	DDRB &= ~(1<<1);
	PORTB |= 1<<1;
	
	// Poti als Input setzen
	// ADC
	ADMUX= 0x100040;// AVCC on; Right adjust;MUXuse A0
	ADCSRA= 0xC7;// ADC enable; Stop Conversion; No Autotrigger; Interrupt disable; Prescaler= 128 means 125 kHz
	
	// Interrupts aktivieren
	sei();
	// Timer0 A Match Disable
	TIMSK0 |= (0<<OCIE0A);
	OCR0A = 0; //OCR => wann match
	// Configure CTC (Clear Timer on Compare) Mode
	TCCR0A |= (1<<WGM01);
	TCCR0A &= ~(1<<WGM00);
	TCCR0B &= ~(1<<WGM02);
	// Prescaler konfigurieren (CPU Faktor reduzieren)
	TCCR0B |=(1<<CS02) | (1<<CS00);
	TCCR0B &= ~(1<<CS01);
	
	// Button 1 Interrupt
	PCICR |= (1<<PCIE2); // enable PB Port D interrupt
	PCMSK2 |= 1<<PCINT17; // enable PB17 interrupt
	
	asm("nop");
}

void timerOn(void){
	// timer anschalten
	TIMSK0 |= (1<<OCIE0A);
	OCR0A = 155;
}

void timerOff(void){
	// timer ausschalten, contains bug
	TIMSK0 &= ~(1<<OCIE0A);
	OCR0A = 0;
}

// Timer Interrupt alle 10 ms
ISR(TIMER0_COMPA_vect)
{
	static volatile uint8_t counter = 0;
	counter++;
	// da der Interrupt alle 10 ms kommt und wir aber nur einmal pro Sekunde
	// wirklich was machen wollen, zählen wir halt bis 100. (100*10 ms = 1 s)
	if (counter == 100)	{
		counter = 0;

		// debug stuff
		// TFT_Print("Timer läuft", 30, 50, 2, TFT_16BitOrange, TFT_16BitWhite, TFT_Landscape180);
		
		// timer um eins senken
		timer--;
		// display aktualisieren
		displayTimer(timer);
		
		if (timer < 1) {
			// timer wird IMMER ausgeschaltet, falls benötigt, danach wieder angeschaltet.
			// timerOff();
			
			switch (state) {
				case 3:
					// Buzzer output für 1 sek
					toggleBuzzer(true);
					timer = 1;
					state = 4; // übergabe zu state 4
					// timerOn();
					break;
				case 4:
					// Pause
					toggleBuzzer(false);
					// timer = 300; dev
					timer = pausenzeit;
					displayMessage(4);
					state = 5; // übergabe zu state 5
					// timerOn();
					break;
				case 5:
					// Buzzer output für 1 sek
					toggleBuzzer(true);
					timer = 1;
					state = 6; // übergabe zu state 6
					// timerOn();
					break;
				case 6:
					// Ende
					toggleBuzzer(false);
					timer = endzeit;
					displayMessage(5);
					state = 7; // übergabe zu state 7
					// timerOn();
					break;
				case 7:
					// Neustart ist keine Option, die State Machine springt zurück in State
					timerOff();
					state = 2;
					configuration();
					break;
			}
		}
		
	}
}

// Button 1 Interrupt
ISR(PCINT2_vect) {
	timerOff();
	timer = 0; // sane
	// Zurück zur Konfiguration
	state = 2;
	configuration();
}

void displayTimer(int sekunden) {
	int minutes = sekunden / 60;
	int seconds = sekunden % 60;
	
	char anzeige[6];
	snprintf(anzeige, sizeof(anzeige), "%02d:%02d\n", minutes, seconds);
	
	TFT_Print(anzeige, 25, 44, 4, TFT_16BitBlack, TFT_16BitWhite, TFT_Landscape180);
}

uint16_t readPoti(void) {
	ADCSRA|= (1 << ADSC);// Start conversion
	while(ADCSRA& (1<<ADSC)); // wait while
	return ADC;
}

void configuration(void){
	displayMessage(2);
	// messwert über Poti auslesen erhalten
	while (!BUTTON_2_PRESS){
		uint16_t temp = readPoti();
		
/*
if(temp<128){
	messwert = 6;
}else{
	messwert = 1500 * ((int)temp/(int)128) //due to integer, it will always floor
}

*/

		if (temp < 128) { //todo what about calculating the "messwert"? Like 
			// messwert = 1200; // 20 Min
			messwert = 6; // 6 s for dev
			} else if (temp < 256) {
			messwert = 1500; // 25 Min
			} else if (temp < 384) {
			messwert = 1800; // 30 Min
			} else if (temp < 512) {
			messwert = 2100; // 35 Min
			} else if (temp < 640) {
			messwert = 2400; // 40 Min
			} else if (temp < 768) {
			messwert = 2700; // 45 Min
			} else if (temp < 896) {
			messwert = 3000; // 50 Min
			} else {
			messwert = 3300; // 55 Min
		}
		
		displayTimer(messwert);
		
		char snum[5];
		snprintf(buffer, sizeof(snum), "%d", messwert);
		// TFT_Print(snum, 30, 30, 2, TFT_16BitOrange, TFT_16BitWhite, TFT_Landscape180);
	}
	
	// messwert in die Variablen schreiben
	timer = messwert;
	displayMessage(0);
	
	timerOn();
	// entering State 3 - Work Timer Phase
	state = 3;//TODO bad state should handle itself
}

int main(void){
	// State 0 - Init Phase
	state = 0;
	init();
	
	// todo: Serial Out?
	
	// State 1 - Willkommen Nachricht
	state = 1; //TODO do in state machine
	displayMessage(1); // Nachricht aufs Display schicken
	
	while (!BUTTON_2_PRESS){;};
	
	// State 2 - Konfiguraton
	state = 2; //TODO MOVE TO STATEMACHINE!
	
	displayMessage(2);
	// warten bis knopf losgelassen
	while (BUTTON_2_PRESS){;};
	configuration();
		
	// ab hier muss die Main() Funktion nichts mehr machen,
	// wird daher in eine unendliche while true schleife geschickt
	while(1){;}
	
	
	// char mytext1[] = "00:00";
	//Übergabe von 7 "Werten": Adresse des 1. Elements von mytext, x1, y1, scale,
	// TFT_Print(mytext1, 25, 44, 4, TFT_16BitBlack, TFT_16BitWhite, TFT_Landscape180);		//Schriftfarbe, Hintergrundfarbe, Display-Orientierung
}


// convert 123 to string [buf]
// char snum[5];
// itoa(temp, snum, 10);

