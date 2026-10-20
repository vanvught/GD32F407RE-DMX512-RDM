/**
 * @file globalparams.h
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org */

#ifndef JSON_GLOBALPARAMS_H_
#define JSON_GLOBALPARAMS_H_

#include "configurationstore.h"
#include "json/globalparamsconst.h"
#include "json/json_key.h"
#include "json/json_params_base.h"

namespace json {
class GlobalParams : public JsonParamsBase<GlobalParams> {
   public:
    GlobalParams();

    GlobalParams(const GlobalParams&) = delete;
    GlobalParams& operator=(const GlobalParams&) = delete;

    GlobalParams(GlobalParams&&) = delete;
    GlobalParams& operator=(GlobalParams&&) = delete;

    void Load() { JsonParamsBase::Load(json::GlobalParamsConst::kFileName); }
    void Store(std::string_view buffer);
    void Set();

   protected:
    void Dump();

   private:
    static void SetUtcOffset(std::string_view value);

    static constexpr json::Key kGlobalKeys[] = {
        json::MakeKey(SetUtcOffset, GlobalParamsConst::kUtcOffset),
    };

    inline static common::store::Global store_global;

    friend class JsonParamsBase<GlobalParams>;
};
} // namespace json

#endif // JSON_GLOBALPARAMS_H_
