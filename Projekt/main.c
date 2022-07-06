/****************************************************************************
*   Hochschule f�r Angewandte Wissenschaften Hamburg                        *
*   Fakult�t DMI - Department Medientechnik                                 *
*   Veranstaltung: Informatik & Elektronik                                  *
*****************************************************************************
*   Projektidee: Pomodoro-Timer                                             *
*                                                                           *
*   Git:    git.lumos.city/haw-ms/inf3-mikrocontroller                      *
*   Git:    git.haw-hamburg.de/acj316/inf3-pomodoro-timer                   *
*   Media:  mega.nz/folder/QRNzwS6I#41UNXoJ-3BSbrQg33y9EmA                  *
*****************************************************************************
*   2359614	Helms, S�ren Richard                                            *
*   2574970	Hoefener, Jannik                                                *
****************************************************************************/

#define F_CPU 16E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/wdt.h>
#include "spi.h"
#include "tft.h"

// Buttons
#define BUTTON_2_PRESS	!(PINB & (1<<PINB1))
#define BUTTON_1_PRESS	!(PIND & (1<<PIND1))

// PC1 : RGB LED Gr�n
#define GREEN_LED_ON	PORTC|= (1<<1);
#define GREEN_LED_OFF	PORTC &= ~(1<<1);
// PC2 : RGB LED Rot
#define RED_LED_ON		PORTC|= (1<<2);
#define RED_LED_OFF		PORTC &= ~(1<<2);
// PC1&2 : RGB LED
#define ALL_LED_OFF		{RED_LED_OFF; GREEN_LED_OFF;}
// PC3 : Active Buzzer
#define BUZZER_ON		PORTC|= (1<<3)
#define BUZZER_OFF		PORTC &= ~(1<<3)
// PC4 : Ultraschall Trigger
#define US_TRIGGER_ON	PORTC|= (1<<4);
#define US_TRIGGER_OFF	PORTC &= ~(1<<4);
// PC5 : Ultraschall Echo (input)

// Timer1 als Uhrwerk/Antrieb f�r die State Machine
#define UHRWERK_ON		{TIMSK1 |= (1<<OCIE1A); OCR1A = 15624;}
#define UHRWERK_OFF		{TIMSK1 &= ~(1<<OCIE1A); OCR1A = 0;}
	
// Timer2 f�r den Ultraschallsensor Echo
#define SONIC_TIMER_ON	{TIMSK2 |= (1 << OCIE2A); OCR2A = 115;} //17241.379 Hz (16000000/((115+1)*8))  Pre  8
#define SONIC_TIMER_OFF	{TIMSK2 |= (0 << OCIE2A); OCR2A = 0;}

// Variablendefinitionen
// uint8	 8 bit	  255
// uint16	16 bit	65535

uint8_t		state = 0;
uint16_t	timer = 0;
uint16_t	messwert = 1200;

uint16_t	pausenzeit = 300;		// default 300 s f�r 5 Min
uint16_t	endzeit = 10;			// default 10 s

uint16_t	volatile sonicTimer = 0;		// Z�hler f�r Ultraschallsensor
uint8_t		volatile sonicCounting = 0;		// l�uft eine Messung? (boolean issues problems)
uint8_t		volatile sonicThreshold = 30;	// grenzwert in centimeter

// Messwert f�r >300 cm laut Oszilloskop 19,9 ms High
// Messwert f�r ca 20 cm laut Oszi ebenfalls 19,9 ms High

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
	//Display-Hintergrundfarbe �bertragen:
	int x;
	for(x=0; x<23232; x++){
		SPISend8Bit(0xFF); 				//senden von 1. 8-Bit-Wert f�r wei�
		SPISend8Bit(0xFF); 				//senden von 2. 8-Bit-Wert f�r wei�
	}
	// �bertragen der �berschrift auf das Display
	char mytext[]  = "Pomodoro-Timer";
	TFT_Print(&mytext[0], 4, 6, 2, TFT_16BitRed, TFT_16BitWhite, TFT_Landscape180);
	
	// Buttons als Input setzen
	DDRD &= ~(1<<1);
	PORTD |= 1<<1;
	DDRB &= ~(1<<1);
	PORTB |= 1<<1;
	
	// Poti als Input setzen
	// ADC
	ADMUX= 0x100040;  // AVCC on; Right adjust; MUXuse A0
	ADCSRA= 0xC7;     // ADC enable; Stop Conversion; No Autotrigger; Interrupt disable; Prescaler= 128 means 125 kHz
	
	// PortC 1 & 2 (LEDs), 3 (Buzzer), 4 (Trigger Ultraschallsensor) als Output
	// PortC 0 (Poti) und 5 (Echo Ultraschallsensor als Input)
	DDRC |=   0b011110;
	//			543210
	
	// Interrupts aktivieren
	sei();
	
	// Timer1 A Match Disable
	TIMSK1 &= ~(1 << OCIE1A);
	OCR1A = 0;
	// configure CTC
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	TCCR1B |= (1<<WGM12);
	// Prescaler 1024
	TCCR1B |= (1<<CS12) | (1<<CS10);
	
	// Timer2 A Match Disable
	TIMSK2 |= (0 << OCIE2A);
	OCR2A = 0;	
	// Configure CTC (Clear Timer on Compare) Mode
	TCCR2A |= (1 << WGM21);
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;
	// Prescaler 8
	TCCR2B |= (1 << CS21);	
	// Prescaler 32
	// TCCR2B |= (1 << CS21) | (1 << CS20);
		
	// Button 1 Interrupt - Interrupt auf PCINT17
	PCICR |= (1<<PCIE2); // enable Port D interrupt
	PCMSK2 |= 1<<PCINT17; // enable PCINT17 interrupt
	
	// Ultraschallsensor Echo Interrupt auf PCINT13 **deaktiviert, da es nicht funktioniert**
	// entsprechend Seite 57 Datenblatt ATMEGA328P
	// PCICR |= (1<<PCIE1); // enable Port C interrupt
	// PCMSK1 |= (1<<PCINT13); // enable PCINT13 interrupt
	
	asm("nop");
}

void triggerDistanz(void) {
	// Ultraschallsensor Trigger f�r 10 �s auf HIGH setzen
	US_TRIGGER_ON;
	_delay_us(10);
	US_TRIGGER_OFF;
}

// (Uhrwerk) Timer Interrupt alle 1 s
// OCR 15624 = 1 Hz weil: (16000000/((15624+1)*1024))=1Hz
ISR(TIMER1_COMPA_vect)
{
	// Ultraschallsensor Messung ausl�sen 
	if (state == 3) {
		// nur in der Arbeitsphase (State 3) brauchen wir die Messung
		triggerDistanz();
	}
				
	// timer um eins senken
	timer--;
	// display aktualisieren
	displayTimer(timer);
		
	if (timer < 1) {
		// timer wird IMMER ausgeschaltet, falls ben�tigt, danach wieder angeschaltet.
			
		switch (state) {
			case 3:				// Arbeitsphase zu Ende, �bergabe zur Pause	Dauer: 1 Sekunde
				ALL_LED_OFF;
				BUZZER_ON;		// Buzzer output f�r 1 s
				timer = 1;
				state = 4;		// �bergabe zu state 4
				break;
			case 4:				// Pausenphase
				GREEN_LED_ON;
				BUZZER_OFF;
				timer = pausenzeit;
				displayMessage(4);
				state = 5;		// �bergabe zu state 5
				break;
			case 5:				// Pausenphase zu Ende, �bergabe zum Ende	Dauer: 1 Sekunde
				ALL_LED_OFF;
				BUZZER_ON;
				timer = 1;
				state = 6;		// �bergabe zu state 6
				break;
			case 6:				// Endphase, �bergabe zur�ck zu State 2		Dauer: 10 Sekunden
				BUZZER_OFF;
				timer = endzeit;
				displayMessage(5);
				state = 7;		// �bergabe zu state 7
				break;
			case 7:				// Die State Machine springt zur�ck in State 2 (Konfiguration)
				UHRWERK_OFF;
				state = 2;
				configuration();
				break;
		}
	}
}

// (Sonic) Timer Interrupt alle 58 �s **deaktiviert, da es nicht funktioniert**
// ISR(TIMER2_COMPA_vect){
// 	// einfach den sonicTimer um eins hochz�hlen
// 	sonicTimer = sonicTimer +1;
// }

// Button 1 Interrupt
ISR(PCINT2_vect) {
	UHRWERK_OFF;
	timer = 0; // sane
	// Zur�ck zur Konfiguration
	state = 2;
	// alles ausschalten
	GREEN_LED_OFF;
	RED_LED_OFF;
	BUZZER_OFF;
	configuration();
}

// Pin Change Interrupt f�r Ultraschallsensor **deaktiviert, da es nicht funktioniert**
// ISR(PCINT1_vect) {
// 	if (sonicCounting == 0) {
// 		// Messung l�uft nicht, Messung starten
// 		sonicTimer = 0;
// 		SONIC_TIMER_ON();
// 		// markieren, das eine Messung l�uft
// 		sonicCounting = 1;
// 	} else {
// 		// Messung l�uft, Auswerten
// 		
// 		
// 		// wenn gemessene Distanz zwischen 0 und grenzwert liegt, Fokus Nachricht einblenden
// 		if (sonicTimer < sonicThreshold) { displayMessage(3); }
// 		
// 		// Messung stoppen, markieren, dass keine Messung mehr l�uft
// 		SONIC_TIMER_OFF();
// 		sonicCounting = 0;
// 	}
// }

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
			message2 = "Button dr�cken";
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
	// Nachdem die entsprechende Nachricht eingef�gt wurde, kann diese
	// auf das Display �bertragen werden:
	TFT_Print(message1, 4, 94, 2, TFT_16BitDark_Blue, TFT_16BitWhite, TFT_Landscape180);
	TFT_Print(message2, 4, 114, 2, TFT_16BitDark_Blue, TFT_16BitWhite, TFT_Landscape180);
}

void displayTimer(uint16_t sekunden) {
	uint8_t minutes = sekunden / 60;
	uint8_t seconds = sekunden % 60;
	
	char anzeige[6];
	snprintf(anzeige, sizeof(anzeige), "%02d:%02d\n", minutes, seconds);
	
	TFT_Print(anzeige, 25, 44, 4, TFT_16BitBlack, TFT_16BitWhite, TFT_Landscape180);
}

uint16_t readPoti(void) {
	ADCSRA|= (1 << ADSC);			// Start conversion
	while(ADCSRA& (1<<ADSC));		// wait while
	return ADC;
}

void configuration(void){
	displayMessage(2);
	// messwert �ber Poti auslesen erhalten
	while (!BUTTON_2_PRESS){
		uint16_t temp = readPoti();
		
		if (temp < 128) {
			messwert = 1200; // 20 Min
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
	}
	
	// messwert in die Variablen schreiben
	timer = messwert;
	displayMessage(0);
	RED_LED_ON;
	
	UHRWERK_ON;
	// entering State 3 - Work Timer Phase
	state = 3;
}

int main(void){
	// State 0 - Init Phase
	state = 0;
	init();
	
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
}
