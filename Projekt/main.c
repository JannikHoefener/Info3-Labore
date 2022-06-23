/****************************************************************************
*	Hochschule für Angewandte Wissenschaften Hamburg						*
*	Fakultät DMI															*
*	Department Medientechnik											 	*
*	Veranstaltung: Informatik  & Elektronik									*
*****************************************************************************
*	Projektidee: Pomodoro-Timer												*
*	Git:	git.lumos.city/haw-ms/inf3-mikrocontroller/						*
*	Mirror: * coming soon *													*
*	Doku:	* coming soon *													*
*****************************************************************************
*	2359614	Helms, Sören Richard											*
*	2574970	Hoefener, Jannik												*
****************************************************************************/

#define F_CPU 16E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "spi.h"
#include "tft.h"

#define BUTTON_2_PRESS !(PINB & (1<<PINB1))
#define BUTTON_1_PRESS !(PIND & (1<<PIND1))

uint16_t i;
unsigned int state = 0;
unsigned int timer = 0;
unsigned int messwert = 1200;

unsigned int pausenzeit = 6;// default 300 s für 5 Min
unsigned int endzeit = 10;	// default 10 s

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

void Display_init(void) {
	uint16_t InitData[] ={
		//Initialisierungsdaten fuer 16Bit-Farben-Modus
		0xFDFD, 0xFDFD,
		/* pause */
		0xEF00, 0xEE04, 0x1B04, 0xFEFE, 0xFEFE,
		0xEF90, 0x4A04, 0x7F3F, 0xEE04, 0x4306,
		/* pause */
		0xEF90, 0x0983, 0x0800, 0x0BAF, 0x0A00,
		0x0500, 0x0600, 0x0700, 0xEF00, 0xEE0C,
		0xEF90, 0x0080, 0xEFB0, 0x4902, 0xEF00,
		0x7F01, 0xE181, 0xE202, 0xE276, 0xE183,
		0x8001, 0xEF90, 0x0000,
		// pause
		0xEF08,	0x1805,	0x1283, 0x1500,	0x1300,
		0x16AF 	//Querformat 176 x 132 Pixel
	};
	
	_delay_ms(300);
	PORTD &= !(1<<Reset);	//Reset-Eingang des Displays auf Low => Beginn Hardware-Reset
	_delay_ms(75);
	PORTB |= (1<<CS);		//SSEL auf High
	_delay_ms(75);
	PORTD |= (1<<D_C);		//Data/Command auf High
	_delay_ms(75);
	PORTD |= (1<<Reset);	//Reset-Eingang des Displays auf High => Ende Hardware Reset
	_delay_ms(75);
	SendCommandSeq(&InitData[0], 2);
	_delay_ms(75);
	SendCommandSeq(&InitData[2], 10);
	_delay_ms(75);
	SendCommandSeq(&InitData[12], 23);
	_delay_ms(75);
	SendCommandSeq(&InitData[35], 6);
}

void init(void){
	wdt_disable();
	// alles was einmal zum start erledigt werden muss
	DDRD |= (1<<D_C)|(1<<Reset);		//output: PD2 -> Data/Command; PD3 -> Reset
	SPI_init();
	sei();
	Display_init();
	//Display-Hintergrundfarbe übertragen:
	int x;
	for(x=0; x<23232; x++){
		SPISend8Bit(0xFF); 				//senden von 1. 8-Bit-Wert für weiß
		SPISend8Bit(0xFF); 				//senden von 2. 8-Bit-Wert für weiß
	}
	// Übertragen der Überschrift auf das Display
	char mytext[]  = "Pomodoro-Timer";
	TFT_Print(&mytext[0], 4, 6, 2, TFT_16BitRed, TFT_16BitWhite, TFT_Landscape180);
	
	// Buttons als Input setzen
	DDRD &= ~(1<<1);
	PORTD |= 1<<1;
	DDRB &= ~(1<<1);
	PORTB |= 1<<1;
	
	// Poti als Input setzen
	// ADC
	ADMUX= 0x100040;// AVCC on; Right adjust;MUXuse A0
	ADCSRA= 0xC7;// ADC enable; Stop Conversion; No Autotrigger; Interrupt disable; Prescaler= 128 means 125 kHz
	
	// PortC 1,2,3 als Output
	DDRC |=   0b1110;
	//			3210
	
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

void buzzerOn(void) {
	PORTC|= (1<<3);
}
void buzzerOff(void) {
	PORTC &= ~(1<<3);
}

void showOff(void) {
	// LED rot und grün ausschalten
	PORTC &= ~(1<<1);
	PORTC &= ~(1<<2);
}

void showRed(void) {
	showOff();
	PORTC|= (1<<2);
}

void showGreen(void) {
	showOff();
	PORTC|= (1<<1);
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
					showOff();
					// Buzzer output für 1 sek
					buzzerOn();
					timer = 1;
					state = 4; // übergabe zu state 4
					// timerOn();
					break;
				case 4:
					showGreen();
					// Pause
					buzzerOff();
					// timer = 300; dev
					timer = pausenzeit;
					displayMessage(4);
					state = 5; // übergabe zu state 5
					// timerOn();
					break;
				case 5:
					showOff();
					// Buzzer output für 1 sek
					buzzerOn();
					timer = 1;
					state = 6; // übergabe zu state 6
					// timerOn();
					break;
				case 6:
					// Ende
					buzzerOff();
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

void displayMessage(int messageID) {
	char* message1;
	char* message2;
	
	switch(messageID) {
		case 0:
			message1 = "              ";
			message2 = "              ";
			break;
		case 1:
			message1 = "  Willkommen  ";
			message2 = "              ";
			break;
		case 2:
			message1 = "Poti drehen   ";
			message2 = "Button drücken";
			break;
		case 3:
			message1 = "Konzentration!";
			message2 = "Bald geschafft";
			break;
		case 4:
			message1 = "     Pause    ";
			message2 = "  Bis gleich  ";
			break;
		case 5:
			message1 = "  Geschafft!  ";
			message2 = "   nochmal?   ";
			break;
		default:
			message1 = "  ! Fehler !  ";
			message2 = "              ";
			break;
		
	}
	// Nachdem die entsprechende Nachricht eingefügt wurde, kann diese
	// auf das Display übertragen werden:
	TFT_Print(message1, 4, 94, 2, TFT_16BitDark_Blue, TFT_16BitWhite, TFT_Landscape180);
	TFT_Print(message2, 4, 114, 2, TFT_16BitDark_Blue, TFT_16BitWhite, TFT_Landscape180);
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
		
		if (temp < 128) {
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
		itoa(messwert, snum, 10);
		
		
		// TFT_Print(snum, 30, 30, 2, TFT_16BitOrange, TFT_16BitWhite, TFT_Landscape180);
	}
	
	// messwert in die Variablen schreiben
	timer = messwert;
	displayMessage(0);
	showRed();
	
	timerOn();
	// entering State 3 - Work Timer Phase
	state = 3;
}

int main(void){
	// State 0 - Init Phase
	state = 0;
	init();
	
	// todo: Serial Out?
	
	// State 1 - Willkommen Nachricht
	state = 1;
	displayMessage(1); // Nachricht aufs Display schicken
	
	while (!BUTTON_2_PRESS){;};
	
	// State 2 - Konfiguraton
	state = 2;
	
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
