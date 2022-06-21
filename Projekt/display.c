#include "display.h"
#include "spi.h"
#include "tft.h"

void initDisplay(){
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

void displayMessage(int messageId){
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