/**
 * @file dmx.h
 *
 */
/* Copyright (C) 2021-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_DMX_H_
#define GD32_DMX_H_

#include <cstdint>

#include "dmxconst.h"
#include "dmx/dmx_config.h"
#include "dmxstatistics.h"

struct Statistics
{
    uint32_t nSlotsInPacket;
};

struct Data
{
    uint8_t Data[dmx::buffer::SIZE];
    struct Statistics Statistics;
};

class Dmx
{
   public:
    Dmx();

    void SetPortDirection(uint32_t port_index, dmx::PortDirection port_direction, bool enable_data = false);
    dmx::PortDirection GetPortDirection(uint32_t port_index) const { return m_dmxPortDirection[port_index]; }

    void ClearData(uint32_t port_index);

    volatile dmx::TotalStatistics& GetTotalStatistics(uint32_t port_index);

    // RDM Send

    void RdmSendRaw(uint32_t port_index, const uint8_t* data, uint32_t length);
    void RdmSendDiscoveryRespondMessage(uint32_t port_index, const uint8_t* data, uint32_t length);

    // RDM Receive

    const uint8_t* RdmReceive(uint32_t port_index);
    const uint8_t* RdmReceiveTimeOut(uint32_t port_index, uint16_t time_out);

    // DMX Send

    void SetDmxBreakTime(uint32_t break_time);
    uint32_t GetDmxBreakTime() const;

    void SetDmxMabTime(uint32_t mab_time);
    uint32_t GetDmxMabTime() const;

    void SetDmxPeriodTime(uint32_t period_time);
    uint32_t GetDmxPeriodTime() const { return m_nDmxTransmitPeriod; }

    void SetDmxSlots(uint16_t slots = dmx::kChannelsMax);
    uint16_t GetDmxSlots() const { return m_nDmxTransmitSlots; }

    template <dmx::SendStyle dmxSendStyle> void SetSendData(uint32_t port_index, const uint8_t* data, uint32_t length);
    template <dmx::SendStyle dmxSendStyle> void SetSendDataWithoutSC(uint32_t port_index, const uint8_t* data, uint32_t length);

    void Sync();

    void SetOutputStyle(uint32_t port_index, dmx::OutputStyle output_style);
    dmx::OutputStyle GetOutputStyle(uint32_t port_index) const;

    void Blackout();
    void FullOn();

    // DMX Receive

    const uint8_t* GetDmxAvailable(uint32_t port_index);
    const uint8_t* GetDmxChanged(uint32_t port_index);
    const uint8_t* GetDmxCurrentData(uint32_t port_index);

    uint32_t GetDmxUpdatesPerSecond(uint32_t port_index);

    static Dmx* Get() { return s_this; }

   private:
    void StartData(uint32_t port_index);
    void StopData(uint32_t port_index);

    template <uint32_t portIndex, bool hasStartCode, dmx::SendStyle dmxSendStyle> void SetSendDataInternal(const uint8_t* pData, uint32_t length);

    void StartOutput(uint32_t port_index);
    void StartDmxOutput(uint32_t port_index);

   private:
    uint32_t m_nDmxTransmitPeriod{dmx::transmit::kPeriodDefault};
    uint32_t m_nDmxTransmitPeriodRequested{dmx::transmit::kPeriodDefault};
    uint32_t m_nDmxTransmissionLength[dmx::config::max::PORTS];
    uint32_t m_nDmxTransmitSlots{dmx::kChannelsMax};
    dmx::PortDirection m_dmxPortDirection[dmx::config::max::PORTS];
    bool m_bHasContinuosOutput{false};

    inline static Dmx* s_this;
};

#define DMX_HANDLE_SEND_CASE(i, SC, STYLE) \
    case i:                                \
        return SetSendDataInternal<i, SC, STYLE>(pData, length)

template <dmx::SendStyle dmxSendStyle> inline void Dmx::SetSendData(uint32_t port_index, const uint8_t* pData, uint32_t length)
{
    switch (port_index)
    {
        DMX_HANDLE_SEND_CASE(0, true, dmxSendStyle);
#if DMX_MAX_PORTS >= 2
        DMX_HANDLE_SEND_CASE(1, true, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 3
        DMX_HANDLE_SEND_CASE(2, true, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 4
        DMX_HANDLE_SEND_CASE(3, true, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 5
        DMX_HANDLE_SEND_CASE(4, true, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 6
        DMX_HANDLE_SEND_CASE(5, true, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 7
        DMX_HANDLE_SEND_CASE(6, true, dmxSendStyle);
#endif
#if DMX_MAX_PORTS == 8
        DMX_HANDLE_SEND_CASE(7, true, dmxSendStyle);
#endif
        default:
            return;
    }
}

template <dmx::SendStyle dmxSendStyle> inline void Dmx::SetSendDataWithoutSC(uint32_t port_index, const uint8_t* pData, uint32_t length)
{
    switch (port_index)
    {
        DMX_HANDLE_SEND_CASE(0, false, dmxSendStyle);
#if DMX_MAX_PORTS >= 2
        DMX_HANDLE_SEND_CASE(1, false, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 3
        DMX_HANDLE_SEND_CASE(2, false, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 4
        DMX_HANDLE_SEND_CASE(3, false, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 5
        DMX_HANDLE_SEND_CASE(4, false, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 6
        DMX_HANDLE_SEND_CASE(5, false, dmxSendStyle);
#endif
#if DMX_MAX_PORTS >= 7
        DMX_HANDLE_SEND_CASE(6, false, dmxSendStyle);
#endif
#if DMX_MAX_PORTS == 8
        DMX_HANDLE_SEND_CASE(7, false, dmxSendStyle);
#endif
        default:
            return;
    }
}

#undef DMX_HANDLE_SEND_CASE

#endif  // GD32_DMX_H_
