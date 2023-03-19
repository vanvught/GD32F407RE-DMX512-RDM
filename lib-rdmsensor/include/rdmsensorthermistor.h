/**
 * @file rdmsensorthermistor.h
 *
 */
/* Copyright (C) 2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef RDMSENSORTHERMISTOR_H_
#define RDMSENSORTHERMISTOR_H_

#include <cstdint>

#include "rdmsensor.h"
#include "mcp3424.h"
#include "thermistor.h"

#include "rdm_e120.h"

#include "debug.h"

class RDMSensorThermistor: public RDMSensor, MCP3424  {
public:
	RDMSensorThermistor(uint8_t nSensor, uint8_t nAddress = 0, uint8_t nChannel = 0) : RDMSensor(nSensor), MCP3424(nAddress), m_nChannel(nChannel) {
		SetType(E120_SENS_TEMPERATURE);
		SetUnit(E120_UNITS_CENTIGRADE);
		SetPrefix(E120_PREFIX_NONE);
		SetRangeMin(rdm::sensor::safe_range_min(sensor::thermistor::RANGE_MIN));
		SetRangeMax(rdm::sensor::safe_range_max(sensor::thermistor::RANGE_MAX));
		SetNormalMin(rdm::sensor::safe_range_min(sensor::thermistor::RANGE_MIN));
		SetNormalMax(rdm::sensor::safe_range_max(sensor::thermistor::RANGE_MAX));
		SetDescription(sensor::thermistor::DESCRIPTION);
	}

	bool Initialize() override {
		return MCP3424::IsConnected();
	}

	int16_t GetValue() override {
		const auto v = MCP3424::GetVoltage(m_nChannel);
		const auto r = resistor(v);
		const auto t = sensor::thermistor::temperature(r);
		DEBUG_PRINTF("v=%1.3f, r=%u, t=%3.1f", v,r,t);
		return static_cast<int16_t>(t);
	}

private:
	uint8_t m_nChannel { 0 };
	/*
	 * The R values are based on:
	 * https://www.abelectronics.co.uk/p/69/adc-pi-raspberry-pi-analogue-to-digital-converter
	 */
	static constexpr uint32_t R_GND = 6800;		// 6K8
	static constexpr uint32_t R_HIGH = 10000;	// 10K

	uint32_t resistor(const double vin) {
		const double d = (5 * R_GND) / vin;
		return static_cast<uint32_t>(d) - R_GND - R_HIGH;
	}
};

#endif /* RDMSENSORTHERMISTOR_H_ */
