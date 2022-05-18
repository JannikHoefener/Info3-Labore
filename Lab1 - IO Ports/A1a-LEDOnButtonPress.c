#include <avr/io.h>

#define LED_ON PORTB |= (1<<5)
#define LED_OFF PORTB &= ~(1<<5)
#define LED_TOGGLE PINB |= (1<<5)
#define BUTTON_PRESS (!(PINB & (1<<7)))

void init()
{
    // LED
    DDRB |= (1<<5);// Configure PB5 as Output
    LED_ON;

    // Button
    DDRB &= ~(1<<7);//Configure PB1 as Input
    PORTB |= 1<<7;//Enable Internal Pull-Up at PB1
    asm("nop");
}

void check_button()
{
    if(BUTTON_PRESS)
    LED_ON;
    else
    LED_OFF;
}

int main()
{
    init();
    while (1)
    {
        check_button();
    }
}