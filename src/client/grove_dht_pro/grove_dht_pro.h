#ifndef GROVE_DHT_PRO_H
#define GROVE_DHT_PRO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdexcept>
#include <cmath>

#include "grovepi.h"

namespace GrovePi
{
  class DHT
  {
	  public:

		  const static uint8_t BLUE_MODULE = 0;
		  const static uint8_t WHITE_MODULE = 1;

		  DHT(const uint8_t _module_type = BLUE_MODULE, const uint8_t _pin = 4)
			  : module_type(_module_type), pin(_pin) {
		  }

		  void init();
		  void getSafeData(float &temp, float &humidity);
		  void getUnsafeData(float &temp, float &humidity);

	  private:

		  const uint8_t module_type;
		  const uint8_t pin;
		  static const bool areGoodReadings(int temp, int humidity);

  };
}


#endif
