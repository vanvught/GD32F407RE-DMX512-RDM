/**
 * @file showfileparams.h
 */
/* Copyright (C) 2025 by Arjan van Vught mailto:info@gd32-dmx.org */

#ifndef JSON_SHOWFILEPARAMS_H_
#define JSON_SHOWFILEPARAMS_H_

#include "configurationstore.h"
#include "json/json_key.h"
#include "json/showfileparamsconst.h"
#include "json/oscparamsconst.h"
#include "json/json_params_base.h"

namespace json {
class ShowFileParams : public JsonParamsBase<ShowFileParams> {
   public:
    ShowFileParams();

    ShowFileParams(const ShowFileParams&) = delete;
    ShowFileParams& operator=(const ShowFileParams&) = delete;

    ShowFileParams(ShowFileParams&&) = delete;
    ShowFileParams& operator=(ShowFileParams&&) = delete;

    void Load() { JsonParamsBase::Load(json::ShowFileParamsConst::kFileName); }
    void Store(std::string_view buffer);
    void Set();

   protected:
    void Dump();

   private:
    static void SetShow(std::string_view value);
    static void SetOptionAutoPlay(std::string_view value);
    static void SetOptionLoop(std::string_view value);
    static void SetIncomingPort(std::string_view value);
    static void SetOutgoingPort(std::string_view value);

    static constexpr json::Key kShowFileKeys[] = {
        MakeKey(SetShow, ShowFileParamsConst::kShow),                     //
        MakeKey(SetOptionAutoPlay, ShowFileParamsConst::kOptionAutoPlay), //
        MakeKey(SetOptionLoop, ShowFileParamsConst::kOptionLoop),         //
#if defined(CONFIG_SHOWFILE_ENABLE_OSC)                                   //
        MakeKey(SetIncomingPort, OscParamsConst::kIncomingPort),          //
        MakeKey(SetOutgoingPort, OscParamsConst::kOutgoingPort),
#endif
    };

    inline static common::store::ShowFile store_showfile;

    friend class JsonParamsBase<ShowFileParams>;
};
} // namespace json

#endif // JSON_SHOWFILEPARAMS_H_
