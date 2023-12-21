/**
 * @file storerdmdevice.h
 *
 */
/* Copyright (C) 2019-2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#ifndef STORERDMDEVICE_H_
#define STORERDMDEVICE_H_

#include <cstdint>
#include <cstddef>

#include "rdmdeviceparams.h"

#include "configstore.h"

class StoreRDMDevice {
public:
	static StoreRDMDevice& Get() {
		static StoreRDMDevice instance;
		return instance;
	}

	static void Update(const struct rdm::deviceparams::Params *pParams) {
		Get().IUpdate(pParams);
	}

	static void Copy(struct rdm::deviceparams::Params *pRDMDeviceParams) {
		Get().ICopy(pRDMDeviceParams);
	}

	static void SaveLabel(const char *pLabel, uint8_t nLength) {
		Get().ISaveLabel(pLabel, nLength);
	}

private:
	void IUpdate(const struct rdm::deviceparams::Params *pParams) {
		ConfigStore::Get()->Update(configstore::Store::RDMDEVICE, pParams, sizeof(struct rdm::deviceparams::Params));
	}

	void ICopy(struct rdm::deviceparams::Params *pRDMDeviceParams) {
		ConfigStore::Get()->Copy(configstore::Store::RDMDEVICE, pRDMDeviceParams, sizeof(struct rdm::deviceparams::Params));
	}

	void ISaveLabel(const char *pLabel, uint8_t nLength) {
		ConfigStore::Get()->Update(configstore::Store::RDMDEVICE, offsetof(struct rdm::deviceparams::Params, aDeviceRootLabel), pLabel, nLength, rdm::deviceparams::Mask::LABEL);
		ConfigStore::Get()->Update(configstore::Store::RDMDEVICE, offsetof(struct rdm::deviceparams::Params, nDeviceRootLabelLength), &nLength, sizeof(uint8_t), rdm::deviceparams::Mask::LABEL);
	}
};

#endif /* STORERDMDEVICE_H_ */
