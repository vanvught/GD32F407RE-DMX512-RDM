/**
 * @file artnetnode.h
 *
 */
/**
 * Art-Net Designed by and Copyright Artistic Licence Holdings Ltd.
 */
/* Copyright (C) 2016-2023 by Arjan van Vught mailto:info@orangepi-dmx.nl
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

#ifndef ARTNETNODE_H_
#define ARTNETNODE_H_

#include <cstdint>
#include <cstring>
#include <cassert>

#if !defined(ARTNET_VERSION)
# error ARTNET_VERSION is not defined
#endif

#if (ARTNET_VERSION >= 4) && defined (ARTNET_HAVE_DMXIN)
# if !defined (E131_HAVE_DMXIN)
#  define E131_HAVE_DMXIN
# endif
#endif

#include "artnet.h"
#include "artnettimecode.h"
#include "artnetrdm.h"
#include "artnetstore.h"
#include "artnetdisplay.h"
#include "artnetdmx.h"
#include "artnettrigger.h"

#if (ARTNET_VERSION >= 4)
# include "e131bridge.h"
#endif

#include "lightset.h"
#include "hardware.h"
#include "network.h"

namespace artnetnode {
#if !defined(ARTNET_PAGE_SIZE)
 static constexpr uint32_t PAGE_SIZE = 4;
#else
 static constexpr uint32_t PAGE_SIZE = ARTNET_PAGE_SIZE;
#endif

static_assert((PAGE_SIZE == 4) || (PAGE_SIZE == 1) , "ARTNET_PAGE_SIZE");

#if !defined(LIGHTSET_PORTS)
# define LIGHTSET_PORTS	0
#endif

#if (LIGHTSET_PORTS == 0)
 static constexpr uint32_t PAGES = 1;		// ISO C++ forbids zero-size array
 static constexpr uint32_t MAX_PORTS = 1;	// ISO C++ forbids zero-size array
#else
 static constexpr uint32_t PAGES = ((LIGHTSET_PORTS + (PAGE_SIZE - 1)) / PAGE_SIZE);
 static constexpr auto MAX_PORTS = PAGE_SIZE * PAGES > LIGHTSET_PORTS ? LIGHTSET_PORTS : PAGE_SIZE * PAGES;
#endif

enum class FailSafe : uint8_t {
	LAST = 0x08, OFF= 0x09, ON = 0x0a, PLAYBACK = 0x0b, RECORD = 0x0c
};

/**
 * Table 3 – NodeReport Codes
 * The NodeReport code defines generic error, advisory and status messages for both Nodes and Controllers.
 * The NodeReport is returned in ArtPollReply.
 */
enum class ReportCode : uint8_t {
	RCDEBUG,
	RCPOWEROK,
	RCPOWERFAIL,
	RCSOCKETWR1,
	RCPARSEFAIL,
	RCUDPFAIL,
	RCSHNAMEOK,
	RCLONAMEOK,
	RCDMXERROR,
	RCDMXUDPFULL,
	RCDMXRXFULL,
	RCSWITCHERR,
	RCCONFIGERR,
	RCDMXSHORT,
	RCFIRMWAREFAIL,
	RCUSERFAIL
};

enum class Status : uint8_t {
	OFF, STANDBY, ON
};

struct State {
	uint32_t ArtPollReplyCount;			///< ArtPollReply : NodeReport : decimal counter that increments every time the Node sends an ArtPollResponse.
	uint32_t DiagSendIPAddress;			///< ArtPoll : Destination IPAddress for the ArtDiag
	uint32_t IPAddressArtPoll;
	uint32_t IPAddressArtDmx;
	uint32_t nArtSyncMillis;			///< Latest ArtSync received time
	ReportCode reportCode;
	Status status;
	bool SendArtPollReplyOnChange;		///< ArtPoll : Flags Bit 1 : 1 = Send ArtPollReply whenever Node conditions change.
	bool SendArtDiagData;				///< ArtPoll : Flags Bit 2 : 1 = Send me diagnostics messages.
	bool IsMultipleControllersReqDiag;	///< ArtPoll : Multiple controllers requesting diagnostics
	bool IsSynchronousMode;				///< ArtSync received
	bool IsMergeMode;
	bool IsChanged;
	bool bDisableMergeTimeout;
	bool bUseTargetPortAddress;
	uint8_t nReceivingDmx;
	uint8_t nEnabledOutputPorts;
	uint8_t nEnabledInputPorts;
	uint8_t DiagPriority;				///< ArtPoll : Field 6 : The lowest priority of diagnostics message that should be sent.
	uint16_t TargetPortAddressTop;		///< ArtPoll
	uint16_t TargetPortAddressBottom;	///< ArtPoll
};

struct Node {
	uint32_t IPAddressBroadcast;
	uint32_t IPAddressTimeCode;
	uint8_t MACAddressLocal[artnet::MAC_SIZE];
	uint8_t NetSwitch[artnetnode::PAGES];		///< Bits 14-8 of the 15 bit Port-Address are encoded into the bottom 7 bits of this field.
	uint8_t SubSwitch[artnetnode::PAGES];		///< Bits 7-4 of the 15 bit Port-Address are encoded into the bottom 4 bits of this field.
	char ShortName[artnet::SHORT_NAME_LENGTH];
	char LongName[artnet::LONG_NAME_LENGTH];
	uint8_t Status1;
	uint8_t Status2;
	uint8_t Status3;
	uint8_t DefaultUidResponder[6];							///< RDMnet & LLRP UID
	bool IsRdmResponder;
	bool bMapUniverse0;										///< Art-Net 4
	uint8_t AcnPriority;									///< Art-Net 4
	struct {
		lightset::PortDir direction;
		artnet::PortProtocol protocol;						///< Art-Net 4
	} Port[artnetnode::MAX_PORTS];
};

struct Source {
	uint32_t nMillis;	///< The latest time of the data received from port
	uint32_t nIp;		///< The IP address for port
};

struct GenericPort {
	uint16_t nPortAddress;			///< One of the 32,768 possible addresses to which a DMX frame can be directed. The Port-Address is a 15 bit number composed of Net+Sub-Net+Universe.
	uint8_t nDefaultAddress;		///< the address set by the hardware
	uint8_t nPollReplyIndex;
};

struct OutputPort {
	GenericPort genericPort;
	Source sourceA;
	Source sourceB;
	uint8_t GoodOutput;
	uint8_t GoodOutputB;
	bool IsTransmitting;
	bool IsDataPending;
};

struct InputPort {
	GenericPort genericPort;
	uint32_t nDestinationIp;
	uint8_t nSequenceNumber;
	uint8_t GoodInput;
};

inline static artnetnode::FailSafe convert_failsafe(const lightset::FailSafe failsafe) {
	const auto fs = static_cast<FailSafe>(static_cast<uint32_t>(failsafe) + static_cast<uint32_t>(FailSafe::LAST));
	DEBUG_PRINTF("failsafe=%u, fs=%u", static_cast<uint32_t>(failsafe), static_cast<uint32_t>(fs));
	return fs;
}

inline static lightset::FailSafe convert_failsafe(const artnetnode::FailSafe failsafe) {
	const auto fs = static_cast<lightset::FailSafe>(static_cast<uint32_t>(failsafe) - static_cast<uint32_t>(FailSafe::LAST));
	DEBUG_PRINTF("failsafe=%u, fs=%u", static_cast<uint32_t>(failsafe), static_cast<uint32_t>(fs));
	return fs;
}

}  // namespace artnetnode

#if (ARTNET_VERSION >= 4)
class ArtNetNode: E131Bridge {
#else
class ArtNetNode {
#endif
public:
	ArtNetNode();
	~ArtNetNode();

	void Start();
	void Stop();

	void Run();

	uint8_t GetVersion() const {
		return artnet::VERSION;
	}

	void SetFailSafe(const artnetnode::FailSafe failsafe);

	artnetnode::FailSafe GetFailSafe() {
		const auto networkloss = (m_Node.Status3 & artnet::Status3::NETWORKLOSS_MASK);
		switch (networkloss) {
			case artnet::Status3::NETWORKLOSS_LAST_STATE:
				return artnetnode::FailSafe::LAST;
				break;
			case artnet::Status3::NETWORKLOSS_OFF_STATE:
				return artnetnode::FailSafe::OFF;
				break;
			case artnet::Status3::NETWORKLOSS_ON_STATE:
				return artnetnode::FailSafe::ON;
				break;
			case artnet::Status3::NETWORKLOSS_PLAYBACK:
				return artnetnode::FailSafe::PLAYBACK;
				break;
			default:
				assert(0);
				__builtin_unreachable();
				break;
		}

		__builtin_unreachable();
		return artnetnode::FailSafe::OFF;
	}

	void SetOutputStyle(const uint32_t nPortIndex, const artnet::OutputStyle outputStyle) {
		assert(nPortIndex < artnetnode::MAX_PORTS);

		if (outputStyle == artnet::OutputStyle::CONTINOUS) {
			m_OutputPort[nPortIndex].GoodOutputB |= artnet::GoodOutputB::STYLE_CONSTANT;
		} else {
			m_OutputPort[nPortIndex].GoodOutputB &= static_cast<uint8_t>(~artnet::GoodOutputB::STYLE_CONSTANT);
		}

		if (m_State.status == artnetnode::Status::ON) {
			if (m_pArtNetStore != nullptr) {
				m_pArtNetStore->SaveOutputStyle(nPortIndex, outputStyle);
			}

			artnet::display_outputstyle(nPortIndex, outputStyle);
		}
	}

	artnet::OutputStyle GetOutputStyle(const uint32_t nPortIndex) const {
		assert(nPortIndex < artnetnode::MAX_PORTS);

		const auto isStyleConstant = (m_OutputPort[nPortIndex].GoodOutputB & artnet::GoodOutputB::STYLE_CONSTANT) == artnet::GoodOutputB::STYLE_CONSTANT;
		return isStyleConstant ? artnet::OutputStyle::CONTINOUS : artnet::OutputStyle::DELTA;
	}

	void SetOutput(LightSet *pLightSet) {
		m_pLightSet = pLightSet;
#if (ARTNET_VERSION >= 4)
		E131Bridge::SetOutput(pLightSet);
#endif
	}

	LightSet *GetOutput() const {
		return m_pLightSet;
	}

	uint32_t GetActiveInputPorts() const {
		return m_State.nEnabledInputPorts;
	}

	uint32_t GetActiveOutputPorts() const {
		return m_State.nEnabledOutputPorts;
	}

	void SetShortName(const char *);

	void GetShortNameDefault(char *);
	const char *GetShortName() const {
		return m_Node.ShortName;
	}

	void SetLongName(const char *);

	void GetLongNameDefault(char *);
	const char *GetLongName() const {
		return m_Node.LongName;
	}

	int SetUniverse(const uint32_t nPortIndex, const lightset::PortDir dir, const uint16_t nUniverse);

	int SetUniverseSwitch(const uint32_t nPortIndex, const lightset::PortDir dir, const uint8_t nAddress);
	bool GetUniverseSwitch(const uint32_t nPortIndex, uint8_t &nAddress,const lightset::PortDir dir) const;

	void SetNetSwitch(uint8_t nAddress, uint32_t nPage);

	uint8_t GetNetSwitch(const uint32_t nPage) const {
		assert(nPage < artnetnode::PAGES);
		return m_Node.NetSwitch[nPage];
	}

	void SetSubnetSwitch(uint8_t nAddress, uint32_t nPage);

	uint8_t GetSubnetSwitch(const uint32_t nPage) const {
		assert(nPage < artnetnode::PAGES);
		return m_Node.SubSwitch[nPage];
	}

	bool GetPortAddress(uint32_t nPortIndex, uint16_t& nAddress) const;
	bool GetPortAddress(uint32_t nPortIndex, uint16_t& nAddress, lightset::PortDir dir) const;

	bool GetOutputPort(const uint16_t nUniverse, uint32_t& nPortIndex) {
		for (nPortIndex = 0; nPortIndex < artnetnode::MAX_PORTS; nPortIndex++) {
			if (m_Node.Port[nPortIndex].direction != lightset::PortDir::OUTPUT) {
				continue;
			}
			if ((m_Node.Port[nPortIndex].protocol == artnet::PortProtocol::ARTNET) && (nUniverse == m_OutputPort[nPortIndex].genericPort.nPortAddress)) {
				return true;
			}
		}
		return false;
	}

	void SetMergeMode(const uint32_t nPortIndex, const lightset::MergeMode mergeMode);

	lightset::MergeMode GetMergeMode(const uint32_t nPortIndex) const {
		assert(nPortIndex < artnetnode::MAX_PORTS);
		if ((m_OutputPort[nPortIndex].GoodOutput & artnet::GoodOutput::MERGE_MODE_LTP) == artnet::GoodOutput::MERGE_MODE_LTP) {
			return lightset::MergeMode::LTP;
		}
		return lightset::MergeMode::HTP;
	}

	void SetRmd(const uint32_t nPortIndex, const bool bEnable);
	bool GetRdm(const uint32_t nPortIndex) const {
		assert(nPortIndex < artnetnode::MAX_PORTS);
		return !((m_OutputPort[nPortIndex].GoodOutputB & artnet::GoodOutputB::RDM_DISABLED) == artnet::GoodOutputB::RDM_DISABLED);
	}

	void SetDisableMergeTimeout(bool bDisable) {
		m_State.bDisableMergeTimeout = bDisable;
#if (ARTNET_VERSION >= 4)
		E131Bridge::SetDisableMergeTimeout(bDisable);
#endif
	}

	bool GetDisableMergeTimeout() const {
		return m_State.bDisableMergeTimeout;
	}

#if defined ( ARTNET_ENABLE_SENDDIAG )
	void SendDiag(const char *, artnet::PriorityCodes);
#endif

	void SendTimeCode(const struct TArtNetTimeCode *);

	void SetTimeCodeHandler(ArtNetTimeCode *pArtNetTimeCode) {
		m_pArtNetTimeCode = pArtNetTimeCode;
	}

	void SetTimeCodeIp(uint32_t nDestinationIp);

	void SetRdmHandler(ArtNetRdm *, bool isResponder = false);

	void SetArtNetStore(ArtNetStore *pArtNetStore) {
		m_pArtNetStore = pArtNetStore;
	}

	void SetArtNetTrigger(ArtNetTrigger *pArtNetTrigger) {
		m_pArtNetTrigger = pArtNetTrigger;
	}

	void SetDestinationIp(const uint32_t nPortIndex, const uint32_t nDestinationIp) {
		if (nPortIndex < artnetnode::MAX_PORTS) {
			if (Network::Get()->IsValidIp(nDestinationIp)) {
				m_InputPort[nPortIndex].nDestinationIp = nDestinationIp;
			} else {
				m_InputPort[nPortIndex].nDestinationIp = m_Node.IPAddressBroadcast;
			}

			DEBUG_PRINTF("m_nDestinationIp=" IPSTR, IP2STR(m_InputPort[nPortIndex].nDestinationIp));
		}
	}

	uint32_t GetDestinationIp(const uint32_t nPortIndex) const {
		if (nPortIndex < artnetnode::MAX_PORTS) {
			return m_InputPort[nPortIndex].nDestinationIp;
		}

		return 0;
	}

	/**
	 * LLRP
	 */
	void SetRdmUID(const uint8_t *pUid, bool bSupportsLLRP = false) {
		memcpy(m_Node.DefaultUidResponder, pUid, sizeof(m_Node.DefaultUidResponder));

		if (bSupportsLLRP) {
			m_Node.Status3 |= artnet::Status3::SUPPORTS_LLRP;
		} else {
			m_Node.Status3 &= static_cast<uint8_t>(~artnet::Status3::SUPPORTS_LLRP);
		}
	}

	void Print();

	static ArtNetNode* Get() {
		return s_pThis;
	}

#if (ARTNET_VERSION >= 4)
private:
	void SetUniverse4(const uint32_t nPortIndex, const lightset::PortDir portDir);
	void SetLedBlinkMode4(hardware::ledblink::Mode mode);
	void HandleAddress4(const uint8_t nCommand, const uint32_t nPortIndex);
	uint8_t GetGoodOutput4(const uint32_t nPortIndex);

public:
	/**
	 * Art-Net 4
	 */
	void SetMapUniverse0(const bool bMapUniverse0 = false) {
		m_Node.bMapUniverse0 = bMapUniverse0;
	}
	bool IsMapUniverse0() const {
		return m_Node.bMapUniverse0;
	}

	void SetPriority4(const uint32_t nPriority) {
		m_Node.AcnPriority = static_cast<uint8_t>(nPriority);

		for (uint32_t nPortIndex = 0; nPortIndex < e131bridge::MAX_PORTS; nPortIndex++) {
			E131Bridge::SetPriority(nPortIndex, static_cast<uint8_t>(nPriority));
		}
	}

	void SetPortProtocol4(const uint32_t nPortIndex, const artnet::PortProtocol portProtocol);
	artnet::PortProtocol GetPortProtocol4(const uint32_t nPortIndex) const {
		assert(nPortIndex < artnetnode::MAX_PORTS);
		return m_Node.Port[nPortIndex].protocol;
	}

	/**
	 * sACN E1.131
	 */
	void SetPriority4(uint32_t nPortIndex, uint8_t nPriority) {
		E131Bridge::SetPriority(nPortIndex, nPriority);
	}
	uint8_t GetPriority4(uint32_t nPortIndex) const {
		return E131Bridge::GetPriority(nPortIndex);
	}

	bool GetUniverse4(uint32_t nPortIndex, uint16_t &nUniverse, lightset::PortDir portDir) const {
		return E131Bridge::GetUniverse(nPortIndex, nUniverse, portDir);
	}

	uint32_t GetActiveOutputPorts4() const {
		return E131Bridge::GetActiveOutputPorts();
	}

	uint32_t GetActiveInputPorts4() const {
		return E131Bridge::GetActiveInputPorts();
	}
#endif

private:
	void FillPollReply();
#if defined ( ARTNET_ENABLE_SENDDIAG )
	void FillDiagData(void);
#endif

	void GetType(const uint32_t nBytesReceived, enum TOpCodes& opCode);

	void HandlePoll();
	void HandleDmx();
	void HandleSync();
	void HandleAddress();
	void HandleTimeCode();
	void HandleTimeSync();
	void HandleTodControl();
	void HandleTodData();
	void HandleTodRequest();
	void HandleRdm();
	void HandleIpProg();
#if defined (ARTNET_HAVE_DMXIN)
	void HandleDmxIn();
#endif
	void HandleRdmIn();
	void HandleTrigger();

	uint16_t MakePortAddress(const uint16_t nUniverse, const uint32_t nPage);

	bool GetPortIndexInput(const uint32_t nPage, const uint32_t nPollReplyIndex , uint32_t& nPortIndex);
	bool GetPortIndexOutput(const uint32_t nPage, const uint32_t nPollReplyIndex, uint32_t& nPortIndex);

	void UpdateMergeStatus(uint32_t nPortIndex);
	void CheckMergeTimeouts(uint32_t nPortIndex);

	void ProcessPollRelply(uint32_t nPortIndex, uint32_t& NumPortsInput, uint32_t& NumPortsOutput);
	void SendPollRelply(bool);
	void SendTod(uint32_t nPortIndex);
	void SendTodRequest(uint32_t nPortIndex);

	void SetNetworkDataLossCondition();

	void FailSafeRecord();
	void FailSafePlayback();

private:
	int32_t m_nHandle { -1 };
	uint8_t *m_pReceiveBuffer { nullptr };
	uint32_t m_nIpAddressFrom;

	artnetnode::Node m_Node;
	artnetnode::State m_State;
	artnetnode::OutputPort m_OutputPort[artnetnode::MAX_PORTS];
	artnetnode::InputPort m_InputPort[artnetnode::MAX_PORTS];

	TArtPollReply m_PollReply;
#if defined (ARTNET_HAVE_DMXIN)
	TArtDmx m_ArtDmx;
#endif
#if defined (RDM_CONTROLLER) || defined (RDM_RESPONDER)
	union UArtTodPacket {
		struct TArtTodData ArtTodData;
		struct TArtTodRequest ArtTodRequest;
		struct TArtRdm ArtRdm;
	};
	UArtTodPacket m_ArtTodPacket;
#endif
#if defined (ARTNET_HAVE_TIMECODE)
	TArtTimeCode m_ArtTimeCode;
#endif
#if defined (ARTNET_ENABLE_SENDDIAG)
	TArtDiagData m_DiagData;
#endif

	uint32_t m_nCurrentPacketMillis { 0 };
	uint32_t m_nPreviousPacketMillis { 0 };

	LightSet *m_pLightSet { nullptr };

	ArtNetTimeCode *m_pArtNetTimeCode { nullptr };
	ArtNetRdm *m_pArtNetRdm { nullptr };
	ArtNetTrigger *m_pArtNetTrigger { nullptr };
	ArtNetStore *m_pArtNetStore { nullptr };

	static ArtNetNode *s_pThis;
};

#endif /* ARTNETNODE_H_ */
