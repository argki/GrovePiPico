// GrovePi C++ library
// v0.2
//
// This library provides the basic functions for using the GrovePi in C
//
// The GrovePi connects the Raspberry Pi and Grove sensors.  You can learn more about GrovePi here:  http://www.dexterindustries.com/GrovePi
//
// Have a question about this example?  Ask on the forums here: http://forum.dexterindustries.com/c/grovepi
//
//      History
//      ------------------------------------------------
//      Author		Date                    Comments
//	    Karan		  28 Dec 2015		            Initial Authoring
//	    Robert		April 2017							  Continuing

/*
   License

   The MIT License (MIT)

   GrovePi for the Raspberry Pi: an open source platform for connecting Grove Sensors to the Raspberry Pi.
   Copyright (C) 2017  Dexter Industries

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#include "grovepi.h"

#include <errno.h>
#include <string>
#include <termios.h>

static const bool DEBUG = false;

static int serial_fd = -1;

namespace GrovePi
{

  // variables which can be used
  // in the user-program
  const uint8_t INPUT = 0;
  const uint8_t OUTPUT = 1;
  const bool LOW = false;
  const bool HIGH = true;
  uint8_t GROVE_ADDRESS = 0x04;

}

void GrovePi::SMBusName(char *smbus_name)
{
	if(smbus_name)
		smbus_name[0] = '\0';
}

int GrovePi::initDevice(uint8_t)
{
	throw I2CError("[initDevice is not supported in USB GrovePi mode]\n");
}

void GrovePi::setMaxI2CRetries(int)
{

}

void GrovePi::setGrovePiAddress(uint8_t address)
{
	GROVE_ADDRESS = address;
}

void GrovePi::writeBlock(uint8_t, uint8_t, uint8_t, uint8_t)
{
	throw I2CError("[writeBlock is not supported in USB GrovePi mode]\n");
}

void GrovePi::writeByte(uint8_t)
{
	throw I2CError("[writeByte is not supported in USB GrovePi mode]\n");
}

uint8_t GrovePi::readBlock(uint8_t *)
{
	throw I2CError("[readBlock is not supported in USB GrovePi mode]\n");
}

uint8_t GrovePi::readByte()
{
	throw I2CError("[readByte is not supported in USB GrovePi mode]\n");
}

static int open_serial_port()
{
	if(serial_fd >= 0)
		return serial_fd;

	const char *env_path = getenv("GROVEPI_SERIAL");
	const char *candidates[] = {
	    env_path,
	    "/dev/ttyACM0",
	    "/dev/ttyUSB0",
	};

	for(size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i)
	{
		const char *path = candidates[i];
		if(path == NULL || path[0] == '\0')
			continue;

		int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
		if(fd < 0)
			continue;

		struct termios tio;
		if(tcgetattr(fd, &tio) != 0)
		{
			close(fd);
			continue;
		}

		cfmakeraw(&tio);
		cfsetispeed(&tio, B115200);
		cfsetospeed(&tio, B115200);
		tio.c_cflag |= (CLOCAL | CREAD);
		tio.c_cflag &= ~CRTSCTS;

		if(tcsetattr(fd, TCSANOW, &tio) != 0)
		{
			close(fd);
			continue;
		}

		serial_fd = fd;
		if(DEBUG)
			fprintf(stderr, "[GrovePi] opened serial at %s\n", path);
		break;
	}

	if(serial_fd < 0)
		throw GrovePi::I2CError("[GrovePiError opening serial device]\n");

	return serial_fd;
}

static void serial_write_line(const std::string &line)
{
	int fd = open_serial_port();
	std::string data = line;
	data.push_back('\n');

	const char *buf = data.c_str();
	ssize_t total = 0;
	ssize_t len = (ssize_t)data.size();

	while(total < len)
	{
		ssize_t w = write(fd, buf + total, len - total);
		if(w < 0)
		{
			if(errno == EINTR)
				continue;
			throw GrovePi::I2CError("[GrovePiError writing to serial]\n");
		}
		total += w;
	}
}

static std::string serial_read_line()
{
	int fd = open_serial_port();
	std::string line;
	char ch;
	int empty_loops = 0;

	while(true)
	{
		ssize_t r = read(fd, &ch, 1);
		if(r < 0)
		{
			// retry if the read is interrupted
			if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
			{
				if(++empty_loops > 5000)
					throw GrovePi::I2CError("[GrovePiError reading from serial: timeout]\n");
				usleep(1000);
				continue;
			}
			throw GrovePi::I2CError("[GrovePiError reading from serial]\n");
		}
		if(r == 0)
		{
			if(++empty_loops > 5000)
				throw GrovePi::I2CError("[GrovePiError reading from serial: timeout]\n");
			usleep(1000);
			continue;
		}

		if(ch == '\n')
			break;
		if(ch == '\r')
			continue;
		line.push_back(ch);
	}

	return line;
}

void GrovePi::initGrovePi()
{
	open_serial_port();
}

/**
 * sleep raspberry
 * @param milliseconds time
 */
void GrovePi::delay(unsigned int milliseconds)
{
	usleep(milliseconds * 1000);
}

/**
 * set pin as OUTPUT or INPUT
 * @param  pin  number
 * @param  mode OUTPUT/INPUT
 */
void GrovePi::pinMode(uint8_t pin, uint8_t mode)
{
	const char *mode_str = (mode == INPUT) ? "INPUT" : "OUTPUT";
	char buf[64];
	snprintf(buf, sizeof(buf), "pinMode(%u, %s)", pin, mode_str);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in pinMode]\n");
}

/**
 * set a pin as HIGH or LOW
 * @param  pin   number
 * @param  value HIGH or LOW
 */
void GrovePi::digitalWrite(uint8_t pin, bool value)
{
	const char *val_str = value ? "HIGH" : "LOW";
	char buf[64];
	snprintf(buf, sizeof(buf), "digitalWrite(%u, %s)", pin, val_str);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in digitalWrite]\n");
}

/**
 * reads whether a pin is HIGH or LOW
 * @param  pin number
 * @return     HIGH or LOW
 */
bool GrovePi::digitalRead(uint8_t pin)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "digitalRead(%u)", pin);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in digitalRead]\n");
	int v = atoi(resp.c_str());
	return v != 0;
}

/**
 * describe at a desired pin a voltage between 0 and VCC
 * @param  pin   number
 * @param  value 0-255
 */
void GrovePi::analogWrite(uint8_t pin, uint8_t value)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "analogWrite(%u, %u)", pin, value);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in analogWrite]\n");
}

/**
 * reads analog data from grovepi sensor(s)
 * @param  pin number
 * @return     16-bit data
 */
short GrovePi::analogRead(uint8_t pin)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "analogRead(%u)", pin);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in analogRead]\n");

	long raw = strtol(resp.c_str(), NULL, 10);
	if(raw < 0)
		return -1;
	
	short scaled = (short)(raw >> 6);
	return scaled;
}

/**
 * to be completed
 * @param  pin number
 * @return     time taken for the sound to travel back?
 */
short GrovePi::ultrasonicRead(uint8_t pin)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "ultrasonicRead(%u)", pin);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		return -1;

	long dist = strtol(resp.c_str(), NULL, 10);
	if(dist < 0)
		return -1;
	return (short)dist;
}

/**
 * LCD にテキストを表示する
 * @param  bus   I2C バス番号 (0/1)
 * @param  text  表示文字列（最大 32 文字程度を推奨）
 */
void GrovePi::setText(uint8_t bus, const char *text)
{
	if(text == NULL)
		text = "";

	std::string t(text);
	for(size_t i = 0; i < t.size(); ++i)
	{
		if(t[i] == '\r' || t[i] == '\n')
			t[i] = ' ';
	}

	char header[32];
	snprintf(header, sizeof(header), "setText(%u, ", bus);

	std::string cmd(header);
	cmd += t;
	cmd.push_back(')');

	serial_write_line(cmd);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in setText]\n");
}

/**
 * LCD のバックライト色を設定する
 * @param  bus I2C バス番号 (0/1)
 * @param  r   赤成分 (0-255)
 * @param  g   緑成分 (0-255)
 * @param  b   青成分 (0-255)
 */
void GrovePi::setRGB(uint8_t bus, uint8_t r, uint8_t g, uint8_t b)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "setRGB(%u, %u, %u, %u)", bus, r, g, b);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in setRGB]\n");
}

/**
 * DHT 温湿度センサーから値を取得する
 * @param  pin          センサー接続ピン (16/18/20 など)
 * @param  module_type  0=BLUE(DHT11), 1=WHITE(DHT22)
 * @param  temp         取得した温度[℃]
 * @param  humidity     取得した湿度[%]
 */
void GrovePi::dhtRead(uint8_t pin, uint8_t module_type, float &temp, float &humidity)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "dhtRead(%u, %u)", pin, module_type);
	serial_write_line(buf);
	std::string resp = serial_read_line();
	if(resp == "error")
		throw I2CError("[GrovePiError in dhtRead]\n");

	float t = 0.0f;
	float h = 0.0f;
	if(sscanf(resp.c_str(), "%f %f", &t, &h) != 2)
		throw I2CError("[GrovePiError parsing dhtRead response]\n");

	temp = t;
	humidity = h;
}

const char* GrovePi::I2CError::detail()
{
	return this->what();
}
