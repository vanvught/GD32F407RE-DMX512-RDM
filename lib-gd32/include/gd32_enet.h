/**
 * @file gd32_enet.h
 *
 */
/* Copyright (C) 2024-2025 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef GD32_ENET_H_
#define GD32_ENET_H_

#include "gd32.h"

/**
 * @brief Retrieves specific descriptor information for Ethernet DMA operations.
 *
 * This templated function extracts various details from an Ethernet descriptor structure
 * based on the specified descriptor state enum. The implementation differs depending on
 * the target platform (GD32F10X, GD32F20X, GD32F4XX, GD32H7XX).
 *
 * @tparam info_get Descriptor state enum indicating the type of information to retrieve.
 * @param[in] desc Pointer to the Ethernet descriptor structure.
 * @return The requested descriptor information as a 32-bit unsigned integer. If the request
 *         is invalid, the function returns `0xFFFFFFFF`.
 *
 * @note For `RXDESC_FRAME_LENGTH`, the function adjusts the length by excluding the CRC
 *       (if applicable) or adding it back depending on the frame type and configuration.
 *
 * @platform Specific implementations for:
 * - **GD32F10X/GD32F20X**: No additional adjustments for frame CRC.
 * - **GD32F4XX**: Adjusts for CRC if forwarding frame configuration is enabled.
 * - **GD32H7XX**: Similar to GD32F4XX but considers the Ethernet peripheral base address.
 */
#if defined(GD32F10X) || defined(GD32F20X)
template <enet_descstate_enum info_get> uint32_t Gd32EnetDescInformationGet(const enet_descriptors_struct* desc)
{
    uint32_t reval = 0xFFFFFFFFU;

    switch (info_get)
    {
        case RXDESC_BUFFER_1_SIZE:
            reval = GET_RDES1_RB1S(desc->control_buffer_size);
            break;
        case RXDESC_BUFFER_2_SIZE:
            reval = GET_RDES1_RB2S(desc->control_buffer_size);
            break;
        case RXDESC_FRAME_LENGTH:
            reval = GET_RDES0_FRML(desc->status);
            if (reval > 4U)
            {
                reval = reval - 4U;
            }
            else
            {
                reval = 0U;
            }
            break;
        case RXDESC_BUFFER_1_ADDR:
            reval = desc->buffer1_addr;
            break;
        case TXDESC_BUFFER_1_ADDR:
            reval = desc->buffer1_addr;
            break;
        case TXDESC_COLLISION_COUNT:
            reval = GET_TDES0_COCNT(desc->status);
            break;
        default:
            break;
    }
    return reval;
}
#elif defined(GD32F4XX)
template <enet_descstate_enum info_get> uint32_t Gd32EnetDescInformationGet(const enet_descriptors_struct* desc)
{
    uint32_t reval = 0xFFFFFFFFU;

    switch (info_get)
    {
        case RXDESC_BUFFER_1_SIZE:
            reval = GET_RDES1_RB1S(desc->control_buffer_size);
            break;
        case RXDESC_BUFFER_2_SIZE:
            reval = GET_RDES1_RB2S(desc->control_buffer_size);
            break;
        case RXDESC_FRAME_LENGTH:
            reval = GET_RDES0_FRML(desc->status);
            if (reval > 4U)
            {
                reval = reval - 4U;
                /* if is a type frame, and CRC is not included in forwarding frame */
                if ((RESET != (ENET_MAC_CFG & ENET_MAC_CFG_TFCD)) && (RESET != (desc->status & ENET_RDES0_FRMT)))
                {
                    reval = reval + 4U;
                }
            }
            else
            {
                reval = 0U;
            }
            break;
        case RXDESC_BUFFER_1_ADDR:
            reval = desc->buffer1_addr;
            break;
        case TXDESC_BUFFER_1_ADDR:
            reval = desc->buffer1_addr;
            break;
        case TXDESC_COLLISION_COUNT:
            reval = GET_TDES0_COCNT(desc->status);
            break;
        default:
            break;
    }
    return reval;
}
#elif defined(GD32H7XX)
template <enet_descstate_enum info_get> uint32_t Gd32EnetDescInformationGet(const enet_descriptors_struct* desc)
{
    uint32_t reval = 0xFFFFFFFFU;

    switch (info_get)
    {
        case RXDESC_BUFFER_1_SIZE:
            reval = GET_RDES1_RB1S(desc->control_buffer_size);
            break;
        case RXDESC_BUFFER_2_SIZE:
            reval = GET_RDES1_RB2S(desc->control_buffer_size);
            break;
        case RXDESC_FRAME_LENGTH:
            reval = GET_RDES0_FRML(desc->status);
            if (reval > 4U)
            {
                reval = reval - 4U;
                /* if is a type frame, and CRC is not included in forwarding frame */
                if ((RESET != (ENET_MAC_CFG(ENETx) & ENET_MAC_CFG_TFCD)) && (RESET != (desc->status & ENET_RDES0_FRMT)))
                {
                    reval = reval + 4U;
                }
            }
            else
            {
                reval = 0U;
            }
            break;
        case RXDESC_BUFFER_1_ADDR:
            reval = desc->buffer1_addr;
            break;
        case TXDESC_BUFFER_1_ADDR:
            reval = desc->buffer1_addr;
            break;
        case TXDESC_COLLISION_COUNT:
            reval = GET_TDES0_COCNT(desc->status);
            break;
        default:
            break;
    }
    return reval;
}
#else
#error
#endif

/**
 * @brief Clears DMA TX transmission flags and resumes DMA TX operation.
 *
 * This function handles transmission buffer unavailable (TBU) and transmission underflow (TU) flags
 * for the Ethernet DMA. It clears these flags and resumes the DMA transmission. The implementation
 * varies based on whether the target platform is GD32H7XX or not.
 */
#if defined(GD32H7XX)
inline void Gd32EnetClearDmaTxFlagsAndResume()
{
    const auto dma_tbu_flag = (ENET_DMA_STAT(ENETx) & ENET_DMA_STAT_TBU);
    const auto dma_tu_flag = (ENET_DMA_STAT(ENETx) & ENET_DMA_STAT_TU);

    if ((0 != dma_tbu_flag) || (0 != dma_tu_flag))
    {
        ENET_DMA_STAT(ENETx) = (dma_tbu_flag | dma_tu_flag); ///< Clear TBU and TU flags
        ENET_DMA_TPEN(ENETx) = 0;                            ///< Resume DMA transmission
    }
}
#else
inline void Gd32EnetClearDmaTxFlagsAndResume()
{
    const auto kDmaTbuFlag = (ENET_DMA_STAT & ENET_DMA_STAT_TBU);
    const auto kDmaTuFlag = (ENET_DMA_STAT & ENET_DMA_STAT_TU);

    if ((0 != kDmaTbuFlag) || (0 != kDmaTuFlag))
    {
        ENET_DMA_STAT = (kDmaTbuFlag | kDmaTuFlag); ///< Clear TBU and TU flags
        ENET_DMA_TPEN = 0;                          ///< Resume DMA transmission
    }
}
#endif

/**
 * @brief Handles the Rx buffer unavailable (RBU) condition.
 *
 * This function checks if the Rx buffer unavailable (RBU) flag is set in the DMA status register.
 * If the flag is set, it clears the RBU flag and resumes the DMA reception by updating the
 * corresponding control register. The implementation varies based on whether the target platform
 * is GD32H7XX or not.
 *
 * @note This function ensures that the DMA reception process continues without interruptions
 * caused by the Rx buffer unavailable condition.
 */
#if defined(GD32H7XX)
inline void Gd32EnetHandleRxBufferUnavailable()
{
    if (0 != (ENET_DMA_STAT(ENETx) & ENET_DMA_STAT_RBU))
    {
        ENET_DMA_STAT(ENETx) = ENET_DMA_STAT_RBU; ///< Clear RBU flag
        ENET_DMA_RPEN(ENETx) = 0;                 ///< Resume DMA reception
    }
}
#else
inline void Gd32EnetHandleRxBufferUnavailable()
{
    if (0 != (ENET_DMA_STAT & ENET_DMA_STAT_RBU))
    {
        ENET_DMA_STAT = ENET_DMA_STAT_RBU; ///< Clear RBU flag
        ENET_DMA_RPEN = 0;                 ///< Resume DMA reception
    }
}
#endif

#if defined(GD32H7XX)
inline void Gd32EnetResetHash()
{
    ENET_MAC_HLH(ENETx) = 0;
    ENET_MAC_HLL(ENETx) = 0;
}
#else
inline void Gd32EnetResetHash()
{
    ENET_MAC_HLH = 0;
    ENET_MAC_HLL = 0;
}
#endif

#if defined(GD32H7XX)
template <uint32_t feature> inline void Gd32EnetFilterFeatureDisable()
{
    auto value = ENET_MAC_FRMF(ENETx);
    value &= ~feature;
    ENET_MAC_FRMF(ENETx) = value;
}
#else
template <uint32_t feature> inline void Gd32EnetFilterFeatureDisable()
{
    auto value = ENET_MAC_FRMF;
    value &= ~feature;
    ENET_MAC_FRMF = value;
}
#endif

#if defined(GD32H7XX)
template <uint32_t feature> inline void Gd32EenetFilterFeatureEnable()
{
    auto value = ENET_MAC_FRMF(ENETx);
    value |= feature;
    ENET_MAC_FRMF(ENETx) = value;
}
#else
template <uint32_t feature> inline void Gd32EenetFilterFeatureEnable()
{
    auto value = ENET_MAC_FRMF;
    value |= feature;
    ENET_MAC_FRMF = value;
}
#endif

#if defined(GD32H7XX)
inline void Gd32EnetFilterSetHash(uint32_t hash)
{
    if (hash > 31)
    {
        ENET_MAC_HLH(ENETx) |= (1U << (hash - 32));
    }
    else
    {
        ENET_MAC_HLL(ENETx) |= (1U << hash);
    }
}
#else
inline void Gd32EnetFilterSetHash(uint32_t hash)
{
    if (hash > 31)
    {
        ENET_MAC_HLH |= (1U << (hash - 32));
    }
    else
    {
        ENET_MAC_HLL |= (1U << hash);
    }
}
#endif

#endif // GD32_ENET_H_
