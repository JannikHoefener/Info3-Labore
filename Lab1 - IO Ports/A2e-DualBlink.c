#define F_CPU 16E6
#include <avr/io.h>
#include <util/delay.h>
#define BUTTON_2_PRESS !(PINB & (1<<PINB1))
#define BUTTON_1_PRESS !(PIND & (1<<PIND1))


int main(void)
{
    init();
    char counter = 0;
    while (1)
    {
        checkResetButton();
        checkIncrementButton();
    }
}

void checkIncrementButton()
{
    if (BUTTON_1_PRESS) // PinD1
        {
            counter++;
            counter = counter % 16;
            displayDual(counter);
            while(BUTTON_1_PRESS){}
        }
}

void checkResetButton()
{
    if (BUTTON_2_PRESS) // PinB1
        {
            counter = 0;
            resetDisplay();
        }
}

void displayDual(int counter)
{
    resetDisplay();
    PORTC |= counter<<1;
}

void resetDisplay() {
    PORTC &= 0b00000;
}

void init(void)
{
    //Register C als Output festlegen
    DDRC |= 0b11110; 
    
    //Button 1 als Input festlegen
    DDRD &= ~(1<<1);
    PORTD |= 1<<1;
    
    //Button 2 als Input festlegen
    DDRB &= ~(1<<1);
    PORTB |= 1<<1;
}