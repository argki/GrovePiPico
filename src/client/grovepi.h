// Copyright Dexter Industries, 2016
// http://dexterindustries.com/grovepi

#ifndef GROVEPI_H
#define GROVEPI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdexcept>

namespace GrovePi
{

  extern const uint8_t INPUT;
  extern const uint8_t OUTPUT;
  extern const bool LOW;
  extern const bool HIGH;
  extern uint8_t GROVE_ADDRESS;

  void SMBusName(char *smbus_name);

  void initGrovePi();  
  int initDevice(uint8_t address);
  void setMaxI2CRetries(int _max_i2c_retries);
  void setGrovePiAddress(uint8_t addr);
  void writeBlock(uint8_t command, uint8_t pin_number, uint8_t opt1 = 0, uint8_t opt2 = 0);
  void writeByte(uint8_t byte_val);
  uint8_t readBlock(uint8_t *data_block);
  uint8_t readByte();

  void delay(unsigned int milliseconds);
  void pinMode(uint8_t pin, uint8_t mode);
  void digitalWrite(uint8_t pin, bool value);
  bool digitalRead(uint8_t pin);
  void analogWrite(uint8_t pin, uint8_t value);
  short analogRead(uint8_t pin);
  short ultrasonicRead(uint8_t pin);

  void setText(uint8_t bus, const char *text);
  void setRGB(uint8_t bus, uint8_t r, uint8_t g, uint8_t b);

  void dhtRead(uint8_t pin, uint8_t module_type, float &temp, float &humidity);


  // this class purpose is to give a more meaningful
  // description of problem that's encountered
  // and to redefine the function name for getting error details
  // (as suggested by Karan)
  class I2CError : public std::runtime_error
  {
	  public:
		  explicit I2CError(const char *str) : std::runtime_error(str) {
		  }
		  const char* detail();
  };

}

#endif
