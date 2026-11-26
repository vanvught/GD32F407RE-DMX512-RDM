/**
 * @file e131params.h
 */
/* Copyright (C) 2025-2026 by Arjan van Vught mailto:info@gd32-dmx.org
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

#ifndef JSON_E131PARAMS_H_
#define JSON_E131PARAMS_H_

#include "configurationstore.h"
#include "json/e131paramsconst.h"
#include "json/json_key.h"
#include "json/json_params_base.h"
#include "dmxnode_outputtype.h"
#ifdef DMXNODE_OUTPUT_DMX
#include "dmx.h" // IWYU pragma: keep
#endif // DMXNODE_OUTPUT_DMX

namespace json {
class E131Params : public JsonParamsBase<E131Params> {
   public:
    E131Params();

    E131Params(const E131Params&) = delete;
    E131Params& operator=(const E131Params&) = delete;

    E131Params(E131Params&&) = delete;
    E131Params& operator=(E131Params&&) = delete;

    void Load() { JsonParamsBase::Load(json::E131ParamsConst::kFileName); }
    void Store(std::string_view buffer);
    void Set();

   protected:
    void Dump();

   private:
    static void SetPriority(std::string_view key, std::string_view value);

#ifdef DMX_MAX_PORTS
    static constexpr json::Key kE131PriorityKeys[] = {
        MakeKey(SetPriority, E131ParamsConst::kPriorityPort[0]), //
#if (DMX_MAX_PORTS > 1)
        MakeKey(SetPriority, E131ParamsConst::kPriorityPort[1]), //
#endif                                                           // (DMX_MAX_PORTS > 1)
#if (DMX_MAX_PORTS > 2)
        MakeKey(SetPriority, E131ParamsConst::kPriorityPort[2]), //
#endif                                                           // (DMX_MAX_PORTS > 2)
#if (DMX_MAX_PORTS == 4)
        MakeKey(SetPriority, E131ParamsConst::kPriorityPort[3]), //
#endif                                                           // (DMX_MAX_PORTS == 4)

    };
#endif // DMX_MAX_PORTS

    inline static common::store::DmxNode store_dmxnode;

    friend class JsonParamsBase<E131Params>;
};
} // namespace json

#endif // JSON_E131PARAMS_H_
