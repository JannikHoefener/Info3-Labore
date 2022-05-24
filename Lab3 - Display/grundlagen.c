/******************************************************************************
;*	Hochschule für Angewandte Wissenschaften Hamburg						  *
;*	Fakultät DMI															  *
;*	Department Medientechnik											 	  *
;*	Veranstaltung: Informatik  & Elektronik                                   *
;******************************************************************************
;*	TFT-Display per SPI-Schnittstelle										  *
;*	Das Display wird im 16Bit-Farbmodus im Hochformat um 180° gedreht be-     *
;*  trieben. Der Hintergrund wird mit dem Farbwert 0xFFE0 gefärbt.            *
;*  Ein rotes Quadrat aus 10x10 Pixeln wird mittig per Button1 und Button2    *
;*  bis zu den Diplayrändern flüssig bewegt.                                  *
;*  																		  *
;*	Versuch-Nr.: 5															  *
;******************************************************************************
;*	Namen/Matrikel-Nr.:  1.	2359614, Helms, Sören Richard                     *
;*              		 2.	2574970, Hoefener, Jannik   	     			  *
;******************************************************************************
;* 	Abgabedatum:             												  *
;******************************************************************************/

#define F_CPU 16E6
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define BUTTON_1_PRESS !(PIND & (1<<PIND1))
#define BUTTON_2_PRESS !(PINB & (1<<PINB1))
#define SPI_DDR DDRB
#define CS      PINB2
#define MOSI    PINB3
#define SCK     PINB5
#define D_C		PIND2		//display: Data/Command
#define Reset	PIND3		//display: Reset

enum TFT_Orientation {TFT_Portrait, TFT_Landscape, TFT_Portrait180, TFT_Landscape180};
enum moveDirection {right, left};
	
void init(){
	DDRD &= ~(1<<1);
	PORTD |= 1<<1;
	// Definition des Buttons (PD1 als INPUT und Aktivierung des Pull-Up)
	
	DDRB &= ~(1<<1);
	PORTB |= 1<<1;
	// Definition des Buttons (PB1 als INPUT und Aktivierung des Pull-Up)

	// Interrupt enable
	sei();
	
	// Timer0 A Match enable
	TIMSK0 |= (1<<OCIE0A);
	OCR0A = 250;
	// Configure CTC Mode
	TCCR0A |= (1<<WGM01);
	TCCR0A &= ~(1<<WGM00);
	TCCR0B &= ~(1<<WGM02);
	// Prescaler on 64
	TCCR0B |= 0b11;
	TCCR0B &= ~(1<<2);
	// effektiv alle 1 ms haben wir einen Interrupt
}

ISR(TIMER0_COMPA_vect)
{
	static volatile uint8_t counter1 = 0;
	static volatile uint8_t counter2 = 0;
	
	// checking every millisecond if button is pressed
	if (BUTTON_1_PRESS == 1)
	// if button 1 is pressed, the counter is increased by one
	{
		counter1++;
		if (counter1 == 250) {
		// if button pressed for 250 ms the move function is called
			counter1 = 0;
			moveRight();
		}
	} else {
	counter1 = 0;
	}
	if (BUTTON_2_PRESS == 1)
	// if button 2 is pressed, the counter is increased by one
	{
		counter2++;
		if (counter2 == 250) {
		// if button pressed for 250 ms the move function is called
			counter2 = 0;
			moveLeft();
		}
	} else {
	counter2 = 0;
}
	
}

void drawYello() {
	uint16_t i;
	for (i=0; i<1000; i++) {
		SPISend8Bit(0xFC);
		// 8 bit "gelb"
	}
}

void drawRed() {
	uint16_t i;
	for (i=0; i<1000; i++) {
		SPISend8Bit(0xF0);
		// 8 bit "rot"
	}
}

void SPI_init(){
	//set CS, MOSI and SCK to output:
	SPI_DDR |= (1 << CS) | (1 << MOSI) | (1 << SCK);

	//enable SPI, set as master, and clock to fosc/4:
	SPCR = (1 << SPE) | (1 << MSTR);
}

void SPISend8Bit(uint8_t data){
	PORTB &= ~(1<<CS);				//CS low
	SPDR = data;					//load data into register
	while(!(SPSR & (1 << SPIF)));	//wait for transmission complete
	PORTB |= (1<<CS);				//CS high
}

void SendCommandSeq(const uint16_t * data, uint16_t Anzahl){
	uint32_t index;
	uint8_t SendeByte;
	for (index=0; index<Anzahl; index++){
		PORTD |= (1<<D_C);						//Data/Command auf High => Kommando-Modus
		SendeByte = (data[index] >> 8) & 0xFF;	//High-Byte des Kommandos
		SPISend8Bit(SendeByte);
		SendeByte = data[index] & 0xFF;			//Low-Byte des Kommandos
		SPISend8Bit(SendeByte);
		PORTD &= ~(1<<D_C);						//Data/Command auf Low => Daten-Modus
	}
}

void Display_init(void) {
	const uint16_t InitData[] ={
		//Initialisierungsdaten fuer 16Bit-Farben-Modus
		0xFDFD, 0xFDFD,
		/* pause */
		0xEF00, 0xEE04, 0x1B04, 0xFEFE, 0xFEFE,
		0xEF90, 0x4A04, 0x7F1F, 0xEE04, 0x4306,
		/* pause */
		0xEF90, 0x0983, 0x0800, 0x0BAF, 0x0A00,
		0x0500, 0x0600, 0x0700, 0xEF00, 0xEE0C,
		0xEF90, 0x0080, 0xEFB0, 0x4902, 0xEF00,
		0x7F01, 0xE181, 0xE202, 0xE276, 0xE183,
		0x8001, 0xEF90, 0x0000,
		/* pause */
		0xEF08, 0x1800, 0x1200, 0x1583, 0x1300,
		0x16AF // Hochformat 132 x 176 Pixel
	};
	
	_delay_ms(300);
	PORTD &= !(1<<Reset);			//Reset-Eingang des Displays auf Low => Beginn Hardware-Reset
	_delay_ms(75);
	PORTB |= (1<<CS);				//SSEL auf High
	_delay_ms(75);
	PORTD |= (1<<D_C);				//Data/Command auf High
	_delay_ms(75);
	PORTD |= (1<<Reset);			//Reset-Eingang des Displays auf High => Ende Hardware Reset
	_delay_ms(75);
	SendCommandSeq(&InitData[0], 2);
	_delay_ms(75);
	SendCommandSeq(&InitData[2], 10);
	_delay_ms(75);
	SendCommandSeq(&InitData[12], 23);
	_delay_ms(75);
	SendCommandSeq(&InitData[35], 6);
}

uint16_t Fenster[] = {
	0xEF08,
	0x1800,
	0x123C, // Position X 60 (breite / 2 - 5)	muss immer um 5 verschoben werden
	0x1545, // länge 10							muss immer X Pos + 9 sein
	0x1352, // Position Y 82 (länge / 2 - 5)
	0x165B  // länge 10
};

void moveRight(){
	// prüfen, dass die Position im Rahmen liegt
	if (Fenster[2] > 0x120A) {
		drawYello(); // Fenster gelb färben
		// x wert um 5 senken hier
		Fenster[2] =- 5;
		Fenster[3] =- 5;
		SendCommandSeq(&Fenster[0],6);
		
		drawRed(); // Fenster rot färben
	}
}

void moveLeft(){
	// prüfen, dass die Position im Rahmen liegt
	if (Fenster[2] < 0x1279) {
		drawYello(); // Fenster gelb färben
		// x wert um 5 erhöhen hier
		Fenster[2] =+ 5;
		Fenster[3] =+ 5;
		SendCommandSeq(&Fenster[0],6);
		
		drawRed(); // Fenster rot färben
	}
}

int main(void){
	uint16_t i;
	DDRD |= (1<<D_C)|(1<<Reset);		//output: PD2 -> Data/Command; PD3 -> Reset
	init();
	SPI_init();
	Display_init();
	// Display gelb färben
	for(i=0; i<23232; i++)
	{
		SPISend8Bit(0xFC);
	}
	SendCommandSeq(&Fenster[0],6);
	
	for (i=0; i<1000; i++) {
		SPISend8Bit(0xF0);
		// 8 bit "rot"
	}
	
	while(1){}	
}
