#include "grove_dht_pro.h"

using std::runtime_error;
using GrovePi::DHT;

/**
 * function for connecting to the GrovePi
 * doesn't matter whether you call it multiple times
 * just check with the isConnected() function to see if you got a connection
 */
void DHT::init()
{
	GrovePi::initGrovePi();
}

/**
 * returns via its parameters the temperature and humidity
 * this function is NaN-proof
 * it always gives "accepted" values
 *
 * if bad values are read, then it will retry reading them
 * and check if they are okay for a number of [MAX_RETRIES] times
 * before throwing a [runtime_error] exception
 *
 * @param temp     in Celsius degrees
 * @param humidity in percentage values
 */
void DHT::getSafeData(float &temp, float &humidity)
{

	this->getUnsafeData(temp, humidity);

	if(std::isnan(temp) || std::isnan(humidity))
		throw runtime_error("[GroveDHT NaN readings - check sensor or wiring]\n");

	if(!DHT::areGoodReadings((int)temp, (int)humidity))
		throw runtime_error("[GroveDHT bad readings - check sensor or wiring]\n");
}

/**
 * function for returning via its arguments the temperature & humidity
 * it's not recommended to use this function since it might throw
 * some NaN or out-of-interval values
 *
 * use it if you come with your own implementation
 * or if you need it for some debugging
 *
 * @param temp     in Celsius degrees
 * @param humidity in percentage values
 */
void DHT::getUnsafeData(float &temp, float &humidity)
{
	GrovePi::dhtRead(this->pin, this->module_type, temp, humidity);
}

const bool DHT::areGoodReadings(int temp, int humidity)
{
	return (temp > -100.0 && temp < 150.0 && humidity >= 0.0 && humidity <= 100.0);
}
