/**
 * @file gd32_i2c.cpp
 *
 */
/* Copyright (C) 2021-2022 by Arjan van Vught mailto:info@gd32-dmx.org
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

static constexpr int32_t TIMEOUT = 0xfff;

static uint8_t s_nAddress;

static int32_t _sendstart(void) {
	auto nTimeout = TIMEOUT;

	/* wait until I2C bus is idle */
	while (i2c_flag_get(I2C_PERIPH, I2C_FLAG_I2CBSY)) {
		if (--nTimeout <= 0) {
			return -GD32_I2C_NOK_TOUT;
		}
	}

	nTimeout = TIMEOUT;

	/* send a start condition to I2C bus */
	i2c_start_on_bus(I2C_PERIPH);

	/* wait until SBSEND bit is set */
	while (!i2c_flag_get(I2C_PERIPH, I2C_FLAG_SBSEND)) {
		if (--nTimeout <= 0) {
			return -GD32_I2C_NOK_TOUT;
		}
	}

	return GD32_I2C_OK;
}

static int32_t _sendslaveaddr(void) {
	auto nTimeout = TIMEOUT;

	/* send slave address to I2C bus */
	i2c_master_addressing(I2C_PERIPH, s_nAddress, I2C_TRANSMITTER);

	/* wait until ADDSEND bit is set */
	while (!i2c_flag_get(I2C_PERIPH, I2C_FLAG_ADDSEND)) {
		if (--nTimeout <= 0) {
			return -GD32_I2C_NOK_TOUT;
		}
	}

	/* clear the ADDSEND bit */
	i2c_flag_clear(I2C_PERIPH, I2C_FLAG_ADDSEND);

	nTimeout = TIMEOUT;

	/* wait until the transmit data buffer is empty */
	while (SET != i2c_flag_get(I2C_PERIPH, I2C_FLAG_TBE)) {
		if (--nTimeout <= 0) {
			return -GD32_I2C_NOK_TOUT;
		}
	}

	return GD32_I2C_OK;
}

static int32_t _stop(void) {
	auto nTimeout = TIMEOUT;

    /* send a stop condition to I2C bus */
    i2c_stop_on_bus(I2C_PERIPH);

    /* wait until the stop condition is finished */
    while(I2C_CTL0(I2C_PERIPH)&0x0200) {
		if (--nTimeout <= 0) {
			return -GD32_I2C_NOK_TOUT;
		}
    }

	return GD32_I2C_OK;
}

static int32_t _senddata(uint8_t *pData, uint32_t nCount) {
	int32_t nTimeout;

	for (auto i = 0; i < nCount; i++) {
		i2c_data_transmit(I2C_PERIPH, *pData);

		/* point to the next byte to be written */
		pData++;

		nTimeout = TIMEOUT;

		/* wait until BTC bit is set */
		while (!i2c_flag_get(I2C_PERIPH, I2C_FLAG_BTC)) {
			if (--nTimeout <= 0) {
				return -GD32_I2C_NOK_TOUT;
			}
		}
	}

	return GD32_I2C_OK;
}

static int _write(char *pBuffer, int nLength) {
	if (_sendstart() != GD32_I2C_OK) {
		_stop();
		return -1;
	}

	if (_sendslaveaddr() != GD32_I2C_OK) {
		_stop();
		return -1;
	}

	if (_senddata((uint8_t*) pBuffer, (uint32_t) nLength) != GD32_I2C_OK) {
		_stop();
		return -1;
	}

	_stop();

	return 0;
}

/*
 * Public API's
 */

void gd32_i2c_begin(void) {
	rcu_periph_clock_enable(I2C_RCU_CLK);
	rcu_periph_clock_enable(I2C_GPIO_SCL_CLK);
	rcu_periph_clock_enable(I2C_GPIO_SDA_CLK);

#if !defined (GD32F4XX)
	gpio_init(I2C_GPIO_SCL_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C_SCL_PIN);
	gpio_init(I2C_GPIO_SDA_PORT, GPIO_MODE_AF_OD, GPIO_OSPEED_50MHZ, I2C_SDA_PIN);

# if defined (I2C_REMAP)
	if (I2C_REMAP == GPIO_I2C0_REMAP) {
		gpio_pin_remap_config(GPIO_I2C0_REMAP, ENABLE);
	}
# endif
#else
    gpio_af_set(I2C_GPIO_SCL_PORT, GPIO_AF_4, I2C_SCL_PIN);
    gpio_af_set(I2C_GPIO_SCL_PORT, GPIO_AF_4, I2C_SDA_PIN);

	gpio_mode_set(I2C_GPIO_SCL_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C_SCL_PIN);
	gpio_output_options_set(I2C_GPIO_SCL_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, I2C_SCL_PIN);
	gpio_mode_set(I2C_GPIO_SDA_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, I2C_SDA_PIN);
	gpio_output_options_set(I2C_GPIO_SDA_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, I2C_SDA_PIN);
#endif

	i2c_clock_config(I2C_PERIPH, GD32_I2C_FULL_SPEED, I2C_DTCY_2);
	i2c_enable(I2C_PERIPH);
	i2c_ack_config(I2C_PERIPH, I2C_ACK_ENABLE);
}

void gd32_i2c_set_baudrate(uint32_t nBaudrate) {
	i2c_clock_config(I2C_PERIPH, nBaudrate, I2C_DTCY_2);
}

void gd32_i2c_set_address(uint8_t nAddress) {
	s_nAddress = nAddress << 1;
}

uint8_t gd32_i2c_write(const char *pBuffer, uint32_t nLength) {
	const auto ret = _write((char *)pBuffer, (int) nLength);

	return (uint8_t)-ret;;
}

uint8_t gd32_i2c_read(char *pBuffer, uint32_t nLength) {
	auto nTimeout = TIMEOUT;

	/* wait until I2C bus is idle */
	while (i2c_flag_get(I2C_PERIPH, I2C_FLAG_I2CBSY)) {
		if (--nTimeout <= 0) {
			_stop();
			return GD32_I2C_NOK_TOUT;
		}
	}

	if (2 == nLength) {
		i2c_ackpos_config(I2C_PERIPH, I2C_ACKPOS_NEXT);
	}

	/* send a start condition to I2C bus */
	i2c_start_on_bus(I2C_PERIPH);

	nTimeout = TIMEOUT;

	/* wait until SBSEND bit is set */
	while (!i2c_flag_get(I2C_PERIPH, I2C_FLAG_SBSEND)) {
		if (--nTimeout <= 0) {
			_stop();
			return GD32_I2C_NOK_TOUT;
		}
	}

	/* send slave address to I2C bus */
	i2c_master_addressing(I2C_PERIPH, s_nAddress, I2C_RECEIVER);

	if (nLength < 3) {
		/* disable acknowledge */
		i2c_ack_config(I2C_PERIPH, I2C_ACK_DISABLE);
	}

	nTimeout = TIMEOUT;

	/* wait until ADDSEND bit is set */
	while (!i2c_flag_get(I2C_PERIPH, I2C_FLAG_ADDSEND)) {
		if (--nTimeout <= 0) {
			_stop();
			return GD32_I2C_NOK_TOUT;
		}
	}

	/* clear the ADDSEND bit */
	i2c_flag_clear(I2C_PERIPH, I2C_FLAG_ADDSEND);

	if (1 == nLength) {
		/* send a stop condition to I2C bus */
		i2c_stop_on_bus(I2C_PERIPH);
	}

	auto nTimeoutLoop = TIMEOUT;

	/* while there is data to be read */
	while (nLength) {
		if (3 == nLength) {
			nTimeout = TIMEOUT;
			/* wait until BTC bit is set */
			while (!i2c_flag_get(I2C_PERIPH, I2C_FLAG_BTC)) {
				if (--nTimeout <= 0) {
					_stop();
					return GD32_I2C_NOK_TOUT;
				}
			}

			/* disable acknowledge */
			i2c_ack_config(I2C_PERIPH, I2C_ACK_DISABLE);
		}

		if (2 == nLength) {
			nTimeout = TIMEOUT;

			/* wait until BTC bit is set */
			while (!i2c_flag_get(I2C_PERIPH, I2C_FLAG_BTC)) {
				if (--nTimeout <= 0) {
					_stop();
					return GD32_I2C_NOK_TOUT;
				}
			}

			/* send a stop condition to I2C bus */
			i2c_stop_on_bus(I2C_PERIPH);
		}

		/* wait until the RBNE bit is set and clear it */
		if (i2c_flag_get(I2C_PERIPH, I2C_FLAG_RBNE)) {
			*pBuffer = i2c_data_receive(I2C_PERIPH);
			pBuffer++;
			nLength--;
			nTimeoutLoop = TIMEOUT;
		}

		if (--nTimeoutLoop <= 0) {
			_stop();
			return GD32_I2C_NOK_TOUT;
		}
	}

	nTimeout = TIMEOUT;

	/* wait until the stop condition is finished */
	while (I2C_CTL0(I2C_PERIPH) & 0x0200) {
		if (--nTimeout <= 0) {
			return GD32_I2C_NOK_TOUT;
		}
	}

	/* enable acknowledge */
	i2c_ack_config(I2C_PERIPH, I2C_ACK_ENABLE);

	i2c_ackpos_config(I2C_PERIPH, I2C_ACKPOS_CURRENT);

	return GD32_I2C_OK;
}
