#ifndef LED_H
#define LED_H

#include <stdbool.h>

void toggleRed(bool state);

void toggleGreen(bool state);

void toggleLeds(bool state);//TODO should this toggle both leds (red on::green off ==> red off::green on)

#endif
