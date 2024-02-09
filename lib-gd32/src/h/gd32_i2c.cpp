/**
 * @file gd32_i2c.cpp
 *
 */
/* Copyright (C) 2024 by Arjan van Vught mailto:info@gd32-dmx.org
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

#include <cstdint>

#include "gd32_i2c.h"
#include "gd32.h"

#include "debug.h"

enum class State {
    I2C_START,
    I2C_TRANSMIT_DATA,
    I2C_RELOAD,
    I2C_STOP
};

static constexpr uint8_t MAX_RELOAD_SIZE = 255;
static constexpr int32_t I2C_TIME_OUT  = 50000;
static uint8_t s_nAddress;

static void gpio_config() {
    rcu_periph_clock_enable(I2C_GPIO_SCL_CLK);
    rcu_periph_clock_enable(I2C_GPIO_SDA_CLK);
    rcu_periph_clock_enable(I2C_RCU_CLK);

    gpio_af_set(I2C_GPIO_SCL_PORT, I2C_GPIO_AF, I2C_SCL_PIN);
    gpio_af_set(I2C_GPIO_SDA_PORT, I2C_GPIO_AF, I2C_SDA_PIN);

    gpio_mode_set(I2C_GPIO_SCL_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C_SCL_PIN);
    gpio_output_options_set(I2C_GPIO_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, I2C_SCL_PIN);

    gpio_mode_set(I2C_GPIO_SDA_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C_SDA_PIN);
    gpio_output_options_set(I2C_GPIO_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_60MHZ, I2C_SDA_PIN);
}

static void i2c_config() {
    rcu_i2c_clock_config(I2C_RCU_IDX, RCU_I2CSRC_IRC64MDIV);
    i2c_timing_config(I2CX, 0x0, 0x6, 0);
    i2c_master_clock_config(I2CX, 0x26, 0x73);
    i2c_enable(I2CX);
}

/**
 * Public API's
 */

void gd32_i2c_begin() {
	gpio_config();
	i2c_config();
}

void gd32_i2c_set_baudrate(const uint32_t nBaudrate) {
// TODO
}

void gd32_i2c_set_address(const uint8_t nAddress) {
	s_nAddress = nAddress << 1;
}

uint8_t gd32_i2c_write(const char *pBuffer, uint32_t nLength) {
	I2C_STAT(I2CX) |= I2C_STAT_TBE;
	i2c_reload_enable(I2CX);
	i2c_automatic_end_disable(I2CX);
	i2c_master_addressing(I2CX, s_nAddress, I2C_MASTER_TRANSMIT);

	if (i2c_flag_get(I2CX, I2C_FLAG_I2CBSY)) {
		DEBUG_PUTS("I2CBSY");
	}

	auto state = State::I2C_RELOAD;
	auto isFirstReload = true;
	auto isLastReload = false;
	uint32_t nBytesReload;

	while (true) {
		switch (state) {
		case State::I2C_RELOAD: {
			if (nLength > MAX_RELOAD_SIZE) {
				nLength = nLength - MAX_RELOAD_SIZE;
				nBytesReload = MAX_RELOAD_SIZE;
			} else {
				nBytesReload = nLength;
				isLastReload = true;
			}

			if (isFirstReload) {
				isFirstReload = false;

				i2c_transfer_byte_number_config(I2CX, nBytesReload);

				if (isLastReload) {
					isLastReload = false;

					if (nLength <= MAX_RELOAD_SIZE) {
						i2c_reload_disable(I2CX);
						i2c_automatic_end_enable(I2CX);
					}
				}

				i2c_start_on_bus(I2CX);

				auto nTimeout = I2C_TIME_OUT;
				while ((!i2c_flag_get(I2CX, I2C_FLAG_TBE)) && (nTimeout > 0)) {
					nTimeout--;
				}

				if (nTimeout <= 0) {
					state = State::I2C_STOP;
					continue;
				}
			} else {
				auto nTimeout = I2C_TIME_OUT;
				while ((!i2c_flag_get(I2CX, I2C_FLAG_TCR)) && (nTimeout > 0)) {
					nTimeout--;
				}

				if (nTimeout <= 0) {
					state = State::I2C_STOP;
					continue;
				}

				i2c_transfer_byte_number_config(I2CX, nBytesReload);

				if (nLength <= MAX_RELOAD_SIZE) {
					i2c_reload_disable(I2CX);
					i2c_automatic_end_enable(I2CX);
				}
			}

			state = State::I2C_TRANSMIT_DATA;
		}
			break;
		case State::I2C_TRANSMIT_DATA: {
			while (nBytesReload) {
				auto nTimeout = I2C_TIME_OUT;
				while ((!i2c_flag_get(I2CX, I2C_FLAG_TI)) && (nTimeout > 0)) {
					nTimeout--;
				}

				if (nTimeout <= 0) {
					printf(" %u %u\n", (I2C_CTL1(I2CX) >> 16) & 0xFF, nBytesReload);
					state = State::I2C_STOP;
					break;
				} else {
					i2c_data_transmit(I2CX, *pBuffer++);
					nBytesReload--;
				}
			}

			if (state == State::I2C_STOP) {
				continue;
			}

			if (I2C_CTL1(I2CX) & I2C_CTL1_RELOAD) {
				state = State::I2C_RELOAD;
			} else {
				state = State::I2C_STOP;
			}
		}
			break;
		case State::I2C_STOP: {
			auto nTimeout = I2C_TIME_OUT;
			while ((!i2c_flag_get(I2CX, I2C_FLAG_STPDET)) && (nTimeout > 0)) {
				nTimeout--;
			}

			if (nTimeout <= 0) {
				return GD32_I2C_NOK_TOUT;
			}

			i2c_flag_clear(I2CX, I2C_FLAG_STPDET);

			if (i2c_flag_get(I2CX, I2C_FLAG_NACK)) {
				i2c_flag_clear(I2CX, I2C_FLAG_NACK);
				return GD32_I2C_NACK;
			}

			return GD32_I2C_OK;
		}
			break;
		default:
			assert(0);
			break;
		}
	}

	assert(0);
	return GD32_I2C_NOK;
}

uint8_t gd32_i2c_read(char *pBuffer, uint32_t nLength) {
	I2C_STAT(I2CX) |= I2C_STAT_TBE;
	i2c_reload_enable(I2CX);
	i2c_automatic_end_disable(I2CX);
	i2c_master_addressing(I2CX, s_nAddress, I2C_MASTER_RECEIVE);

	if (i2c_flag_get(I2CX, I2C_FLAG_I2CBSY)) {
		DEBUG_PUTS("I2CBSY");
	}

	auto state = State::I2C_RELOAD;
	auto isFirstReload = true;
	auto isLastReload = false;
	uint32_t nBytesReload;

	while (true) {
		switch (state) {
		case State::I2C_RELOAD: {
			if (nLength > MAX_RELOAD_SIZE) {
				nLength = nLength - MAX_RELOAD_SIZE;
				nBytesReload = MAX_RELOAD_SIZE;
			} else {
				nBytesReload = nLength;
				isLastReload = true;
			}

			if (isFirstReload) {
				isFirstReload = false;

				i2c_transfer_byte_number_config(I2CX, nBytesReload);

				if (isLastReload) {
					isLastReload = false;

					if (nLength <= MAX_RELOAD_SIZE) {
						i2c_reload_disable(I2CX);
						i2c_automatic_end_enable(I2CX);
					}
				}

				i2c_start_on_bus(I2CX);

				auto nTimeout = I2C_TIME_OUT;
				while ((!i2c_flag_get(I2CX, I2C_FLAG_TBE)) && (nTimeout > 0)) {
					nTimeout--;
				}

				if (nTimeout <= 0) {
					state = State::I2C_STOP;
					continue;
				}
			} else {
				auto nTimeout = I2C_TIME_OUT;
				while ((!i2c_flag_get(I2CX, I2C_FLAG_TCR)) && (nTimeout > 0)) {
					nTimeout--;
				}

				if (nTimeout <= 0) {
					state = State::I2C_STOP;
					continue;
				}

				i2c_transfer_byte_number_config(I2CX, nBytesReload);

				if (nLength <= MAX_RELOAD_SIZE) {
					i2c_reload_disable(I2CX);
					i2c_automatic_end_enable(I2CX);
				}
			}

			state = State::I2C_TRANSMIT_DATA;
		}
			break;
		case State::I2C_TRANSMIT_DATA: {
			while (nBytesReload) {
				auto nTimeout = I2C_TIME_OUT;
				while ((!i2c_flag_get(I2CX, I2C_FLAG_RBNE)) && (nTimeout > 0)) {
					if (i2c_flag_get(I2CX, I2C_FLAG_NACK)) {
						state = State::I2C_STOP;
						break;
					}
					nTimeout--;
				}

				if (nTimeout <= 0) {
					state = State::I2C_STOP;
					break;
				}

				*pBuffer++ = i2c_data_receive(I2CX);
				nBytesReload--;
			}

			if (state == State::I2C_STOP) {
				continue;
			}

			if (I2C_CTL1(I2CX) & I2C_CTL1_RELOAD) {
				state = State::I2C_RELOAD;
			} else {
				state = State::I2C_STOP;
			}
		}
			break;
		case State::I2C_STOP: {
			auto nTimeout = I2C_TIME_OUT;
			while ((!i2c_flag_get(I2CX, I2C_FLAG_STPDET)) && (nTimeout > 0)) {
				nTimeout--;
			}

			if (nTimeout <= 0) {
				return GD32_I2C_NOK_TOUT;
			}

			i2c_flag_clear(I2CX, I2C_FLAG_STPDET);

			if (i2c_flag_get(I2CX, I2C_FLAG_NACK)) {
				i2c_flag_clear(I2CX, I2C_FLAG_NACK);
				return GD32_I2C_NACK;
			}

			return GD32_I2C_OK;
		}
			break;
		default:
			assert(0);
			break;
		}
	}

	assert(0);
	return GD32_I2C_NOK;
}
