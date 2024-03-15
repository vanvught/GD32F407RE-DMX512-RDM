/**
 * @file rdmdeviceresponder.cpp
 *
 */
/* Copyright (C) 2018-2021 by Arjan van Vught mailto:info@orangepi-dmx.nl
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
#include <cstring>
#include <cstdio>
#include <cassert>

#include "rdmdeviceresponder.h"
#include "rdmdevice.h"

#include "rdmsensors.h"
#include "rdmsubdevices.h"

#include "rdmsoftwareversion.h"
#include "rdmpersonality.h"

#include "lightset.h"

#include "hardware.h"

#include "rdm_e120.h"

#include "debug.h"

static constexpr char LANGUAGE[2] = { 'e', 'n' };

RDMDeviceResponder *RDMDeviceResponder::s_pThis;

RDMDeviceResponder::RDMDeviceResponder(RDMPersonality **pRDMPersonalities, uint32_t nPersonalityCount) :
	m_pRDMPersonalities(pRDMPersonalities)
{
	DEBUG_ENTRY

	assert(s_pThis == nullptr);
	s_pThis = this;

	m_aLanguage[0] = LANGUAGE[0];
	m_aLanguage[1] = LANGUAGE[1];

	memset(&m_DeviceInfo, 0, sizeof (struct rdm::device::responder::DeviceInfo));
	memset(&m_SubDeviceInfo, 0, sizeof (struct rdm::device::responder::DeviceInfo));

	m_DeviceInfo.personality_count = static_cast<uint8_t>(nPersonalityCount);

	m_pSoftwareVersion = const_cast<char*>(RDMSoftwareVersion::GetVersion());
	m_nSoftwareVersionLength = static_cast<uint8_t>(RDMSoftwareVersion::GetVersionLength());

	const auto *pLightSet = m_pRDMPersonalities[rdm::device::responder::DEFAULT_CURRENT_PERSONALITY - 1]->GetLightSet();

	if (pLightSet == nullptr) {
		m_nDmxStartAddressFactoryDefault = lightset::dmx::ADDRESS_INVALID;
	}

	DEBUG_EXIT
}

void RDMDeviceResponder::Init() {
	DEBUG_ENTRY

	RDMDevice::Init();

	const auto nSoftwareVersionId = RDMSoftwareVersion::GetVersionId();
	const auto nDeviceModel = Hardware::Get()->GetBoardId();
	const auto nProductCategory = RDMDevice::GetProductCategory();
	const auto nSubDevices = m_RDMSubDevices.GetCount();

	m_DeviceInfo.protocol_major = (E120_PROTOCOL_VERSION >> 8);
	m_DeviceInfo.protocol_minor = static_cast<uint8_t>(E120_PROTOCOL_VERSION);
	m_DeviceInfo.device_model[0] = static_cast<uint8_t>(nDeviceModel >> 8);
	m_DeviceInfo.device_model[1] = static_cast<uint8_t>(nDeviceModel);
	m_DeviceInfo.product_category[0] =static_cast<uint8_t>( nProductCategory >> 8);
	m_DeviceInfo.product_category[1] = static_cast<uint8_t>(nProductCategory);
	m_DeviceInfo.software_version[0] = static_cast<uint8_t>(nSoftwareVersionId >> 24);
	m_DeviceInfo.software_version[1] = static_cast<uint8_t>(nSoftwareVersionId >> 16);
	m_DeviceInfo.software_version[2] = static_cast<uint8_t>(nSoftwareVersionId >> 8);
	m_DeviceInfo.software_version[3] = static_cast<uint8_t>(nSoftwareVersionId);

	auto *pLightSet = m_pRDMPersonalities[rdm::device::responder::DEFAULT_CURRENT_PERSONALITY - 1]->GetLightSet();

	if (pLightSet == nullptr) {
		m_DeviceInfo.dmx_footprint[0] = 0;
		m_DeviceInfo.dmx_footprint[1] = 0;
		m_DeviceInfo.dmx_start_address[0] = static_cast<uint8_t>(m_nDmxStartAddressFactoryDefault >> 8);
		m_DeviceInfo.dmx_start_address[1] = static_cast<uint8_t>(m_nDmxStartAddressFactoryDefault);
	} else {
		m_DeviceInfo.dmx_footprint[0] = static_cast<uint8_t>(pLightSet->GetDmxFootprint() >> 8);
		m_DeviceInfo.dmx_footprint[1] = static_cast<uint8_t>(pLightSet->GetDmxFootprint());
		m_DeviceInfo.dmx_start_address[0] = static_cast<uint8_t>(pLightSet->GetDmxStartAddress() >> 8);
		m_DeviceInfo.dmx_start_address[1] = static_cast<uint8_t>(pLightSet->GetDmxStartAddress());
	}

	m_DeviceInfo.current_personality = rdm::device::responder::DEFAULT_CURRENT_PERSONALITY;
	m_DeviceInfo.sub_device_count[0] = static_cast<uint8_t>(nSubDevices >> 8);
	m_DeviceInfo.sub_device_count[1] = static_cast<uint8_t>(nSubDevices);
	m_DeviceInfo.sensor_count = m_RDMSensors.GetCount();

	memcpy(&m_SubDeviceInfo, &m_DeviceInfo, sizeof(struct rdm::device::responder::DeviceInfo));

	m_nCheckSum = CalculateChecksum();

	DEBUG_EXIT
}

void RDMDeviceResponder::Print() {
	RDMDevice::Print();

	const rdm::device::responder::DeviceInfo *pDeviceInfo = GetDeviceInfo();
	const auto *pPersonality = m_pRDMPersonalities[rdm::device::responder::DEFAULT_CURRENT_PERSONALITY - 1];
	const char *pPersonalityDescription = pPersonality->GetDescription();
	const uint8_t nPersonalityDescriptionLength =pPersonality->GetDescriptionLength();

	puts("RDM Responder configuration");
	printf(" Protocol Version %d.%d\n", pDeviceInfo->protocol_major, pDeviceInfo->protocol_minor);
	printf(" DMX Address      : %d\n", (pDeviceInfo->dmx_start_address[0] << 8) + pDeviceInfo->dmx_start_address[1]);
	printf(" DMX Footprint    : %d\n", (pDeviceInfo->dmx_footprint[0] << 8) + pDeviceInfo->dmx_footprint[1]);
	printf(" Personality %d of %d [%.*s]\n", pDeviceInfo->current_personality, pDeviceInfo->personality_count, nPersonalityDescriptionLength, pPersonalityDescription);
	printf(" Sub Devices      : %d\n", (pDeviceInfo->sub_device_count[0] << 8) + pDeviceInfo->sub_device_count[1]);
	printf(" Sensors          : %d\n", pDeviceInfo->sensor_count);
}

void RDMDeviceResponder::PersonalityUpdate([[maybe_unused]]  LightSet *pLightSet) {
}

void RDMDeviceResponder::DmxStartAddressUpdate() {
}

