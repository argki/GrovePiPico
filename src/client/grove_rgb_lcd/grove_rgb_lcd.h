// Copyright Dexter Industries, 2017
// http://dexterindustries.com/grovepi

#ifndef GROVE_RGB_LCD_H
#define GROVE_RGB_LCD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "grovepi.h"

namespace GrovePi
{
  class LCD
  {
	  public:

		  LCD() {
		  };
		  void connect();

		  void setRGB(uint8_t red, uint8_t green, uint8_t blue);
		  void setText(const char *str);

  };
}

#endif
