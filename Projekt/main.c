/******************************************************************************
* Projektcode auf Basis des Display Projekts aus EMIL
;******************************************************************************/

#define F_CPU 16E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "spi.h"
#include "tft.h"

#define BUTTON_2_PRESS !(PINB & (1<<PINB1))

uint16_t i;
unsigned int state = 0;

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
	ADMUX= 0x40;// AVCC on; Right adjust;MUXuse A0
	ADCSRA= 0xC7;// ADC enable; Stop Conversion; No Autotrigger; Interrupt disable; Prescaler= 128 means 125 kHz
	
	// Interrupts aktivieren
	sei();
	// Timer0 A Match Disable
	TIMSK0 |= (0<<OCIE0A);
	OCR0A = 155; //OCR => wann match
	// Configure CTC (Clear Timer on Compare) Mode
	TCCR0A |= (1<<WGM01);
	TCCR0A &= ~(1<<WGM00);
	TCCR0B &= ~(1<<WGM02);
	// Prescaler konfigurieren (CPU Faktor reduzieren)
	TCCR0B |=(1<<CS02) | (1<<CS00);
	TCCR0B &= ~(1<<CS01);
	
	asm("nop");
}

void timerOn(void){
	// timer anschalten
	TIMSK0 |= (1<<OCIE0A);
}

void timerOff(void){
	// timer ausschalten
	TIMSK0 |= (0<<OCIE0A);
}

// Timer Interrupt alle 10 ms
//ISR(TIMER0_COMPA_vect)
//{
//static volatile uint8_t counter = 0;
//counter++;
//// da der Interrupt alle 10 ms kommt und wir aber nur einmal pro Sekunde
//// wirklich was machen wollen, zählen wir halt bis 100. (100*10 ms = 1 s)
//if (counter == 100)	{
//counter = 0;
//
//switch(state) {
//case 3: // State Timer Phase
//
//break;
//case 4: // State Buzzer Signal
//
//break;
//case 5: // State Timer Phase
//
//break;
//case 6: // State Buzzer Signal
//
//break;
//case 7: // State reset
//
//break;
//}
//
//}

void displayMessage(int messageID) {
	char* message1;
	char* message2;
	
	switch(messageID) {
		case 0:
		message1 = "";
		message2 = "";
		break;
		case 1:
		message1 = "Willkommen";
		message2 = "";
		break;
		case 2:
		message1 = "Poti drehen";
		message2 = "Button drücken";
		break;
		case 3:
		message1 = "Konzentration!";
		message2 = "Bald geschafft";
		break;
		default:
		message1 = "Fehler!";
		message2 = "";
		break;
	}
	// Nachdem die entsprechende Nachricht eingefügt wurde, kann diese
	// auf das Display übertragen werden:
	TFT_Print(message1, 4, 94, 2, TFT_16BitDark_Blue, TFT_16BitWhite, TFT_Landscape180);
	TFT_Print(message2, 4, 114, 2, TFT_16BitDark_Blue, TFT_16BitWhite, TFT_Landscape180);
}

uint16_t readPoti(void) {
	//select ADC channel with safety mask
	ADCSRA|= (1 << ADSC);// Start conversion
	while(ADCSRA& (1<<ADSC)); // wait while
	return ADC;
}

int main(void){
	// State 0 - Init Phase
	state = 0;
	init();
	
	// todo: Serial Out?
	state++;
	
	// State 1 - Willkommen Nachricht
	displayMessage(1); // Nachricht aufs Display schicken
	
	while (!BUTTON_2_PRESS){;};
	state++;
	
	// State 2 - Konfiguraton
	
	displayMessage(2);
	// messwert über Poti auslesen erhalten
	//while (!BUTTON_2_PRESS){
		//uint16_t temp = readPoti();
		//char* t = temp;
		//TFT_Print(t, 30, 30, 2, TFT_16BitOrange, TFT_16BitWhite, TFT_Landscape180);
	//}
	uint16_t messwert = readPoti();
	TFT_Print(messwert, 30, 30, 2, TFT_16BitBlack, TFT_16BitWhite, TFT_Landscape180);
	// messwert in die Variablen schreiben
	
	
	state++;

	// State 3 - Work Timer Phase
	timerOn;
	// ab hier muss die Main() Funktion nichts mehr machen,
	// wird daher in eine unendliche while true schleife geschickt
	while(1){;}
	
	
	// char mytext1[] = "00:00";
	//Übergabe von 7 "Werten": Adresse des 1. Elements von mytext, x1, y1, scale,
	// TFT_Print(mytext1, 25, 44, 4, TFT_16BitBlack, TFT_16BitWhite, TFT_Landscape180);		//Schriftfarbe, Hintergrundfarbe, Display-Orientierung
}
