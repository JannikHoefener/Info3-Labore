/******************************************************************************
;*	Hochschule für Angewandte Wissenschaften Hamburg						  *
;*	Fakultät DMI															  *
;*	Department Medientechnik											 	  *
;*	Veranstaltung: Informatik  & Elektronik                                   *
;******************************************************************************
;*	TFT-Display per SPI-Schnittstelle										  *
;*	Das Display wird im Querformat betrieben.   							  *
;*	Der Hintergrund wird grün gefärbt und ein rotes Rechteck aus 140 x 76     *
;*  Pixeln wird mittig erzeugt. Das rote Rechteck wird durch ein Bild ersetzt,*
;*  dessen Daten in komprimierter Form per Farbwiederholung in picture.c abge-*
;*  legt sind.																  *
;*  																		  *
;*	Versuch-Nr.: 6															  *
;******************************************************************************
;*	Namen/Matrikel-Nr.:  1.	2359614, Helms, Sören Richard                     *
;*              		 2.	2574970, Hoefener, Jannik   	     			  *
;******************************************************************************
;* 	Abgabedatum:             												  *
;******************************************************************************/

#define F_CPU 16E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "picture.h"

#define SPI_DDR DDRB
#define CS      PINB2
#define MOSI    PINB3
#define SCK     PINB5
#define D_C		PIND2		//display: Data/Command
#define Reset	PIND3		//display: Reset

void SPI_init(){
	//set CS, MOSI and SCK to output
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

void SPISend16Bit(uint16_t data){
	uint8_t SendeByte;
	SendeByte = (data >> 8) & 0xFF;		//High-Byte des Kommandos
	SPISend8Bit(SendeByte);
	
	SendeByte = data & 0xFF;			//Low-Byte des Kommandos
	SPISend8Bit(SendeByte);
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
		//pause
		0xEF00, 0xEE04, 0x1B04, 0xFEFE, 0xFEFE,
		0xEF90, 0x4A04, 0x7F3F, 0xEE04, 0x4306,
		//pause
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
	PORTD &= ~(1<<Reset);	//Reset-Eingang des Displays auf Low => Beginn Hardware-Reset
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
// 	_delay_ms(75);
// 	SendCommandSeq(&InitData[35], 6);
}

uint16_t Fenster[] = {

	0xEF08, 
	0x1805, 
	0x1268, // x1
	0x151D, // x2
	0x1312, // y1
	0x169D, // y2
};

int main(void){
	DDRD |= (1<<D_C)|(1<<Reset);		//output: PD2 -> Data/Command; PD3 -> Reset
	SPI_init();
	Display_init();

	//Display-Hintergrund grün "färben"
	for(int i = 0; i < 132*176; i++){
		SPISend16Bit(0x07E0);
	};
	
	// große rotes Rechteck mit 140x76 px malen
	SendCommandSeq(&Fenster[0],6);
	for(int i = 0; i < 140*76 ;i++){

    SPISend16Bit(0xF800);	
	}
	
	//Rechteck mit Bild übermalen
	for (int i = 0; i <= 2900; i++){
		if (Bild1[i] == Bild1[i+1]){
			SPISend16Bit(Bild1[i]);

			for (int j=0; j <= Bild1[i+2]; j++){
				SPISend16Bit(Bild1[i]);
			}
			i += 2;
			
			}else{
			SPISend16Bit(Bild1[i]);
		}
		
	}
	while(1){;}
}
