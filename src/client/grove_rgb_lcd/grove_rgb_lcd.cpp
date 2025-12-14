#include "grove_rgb_lcd.h"

using namespace GrovePi;

void LCD::connect()
{
	initGrovePi();
}

/**
 * set rgb color
 * if there are writes errors, then it throws exception
 * @param red   8-bit
 * @param green 8-bit
 * @param blue  8-bit
 */
void LCD::setRGB(uint8_t red, uint8_t green, uint8_t blue)
{
	GrovePi::setRGB(1, red, green, blue);
}

/**
 *  Grove RGB LCD
 *
 *     |                  Column
 * ------------------------------------------------------
 * Row | 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
 * ------------------------------------------------------
 * 1   | x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x
 * 2   | x  x  x  x  x  x  x  x  x  x  x  x  x  x  x  x
 * ------------------------------------------------------
 *
 * Whatever text is sent to the LCD via the [str] variable
 * Gets printed on the screen.
 * The limit of text is determined by the amount of available
 * Characters on the LCD screen.
 *
 * Every newline char ['\n'] takes the cursor onto the next line
 * (aka on the row no. 2) and uses that space
 * This means that if we have a text such "Hello\n World!",
 * The first 5 characters of the word "Hello" get printed on the first line,
 * whilst the rest of the string gets printed onto the 2nd line.
 * So, if you use the newline char ['\n'] before the row ends,
 * You will end up with less space to put the rest of the characters (aka the last line)
 *
 * If the text your putting occupies more than the LCD is capable of showing,
 * Than the LCD will display only the first 16x2 characters.
 *
 * @param string of maximum 32 characters excluding the NULL character.
 */
void LCD::setText(const char *str)
{
	GrovePi::setText(1, str);
}
