/**
 * @file http.h
 *
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

#ifndef HTTP_HTTP_H_
#define HTTP_HTTP_H_

#include <algorithm>
#include <array>
#include <string_view>

namespace http {
enum class Status {
    kOk = 200,
    kNotModified = 304,
    kBadRequest = 400,
    kNotFound = 404,
    kRequestTimeout = 408,
    kRequestEntityTooLarge = 413,
    kRequestUriTooLong = 414,
    kInternalServerError = 500,
    kMethodNotImplemented = 501,
    kVersionNotSupported = 505,
    kUnknownError = 520, //
};

struct StatusMapping {
    http::Status status;
    std::string_view message;
};

constexpr std::array kStatusMessages{
    StatusMapping{.status = Status::kOk, .message = "OK"},
    StatusMapping{.status = Status::kNotModified, .message = "Not Modified"},
    StatusMapping{.status = Status::kBadRequest, .message = "Bad Request"},
    StatusMapping{.status = Status::kNotFound, .message = "Not Found"},
    StatusMapping{.status = Status::kRequestTimeout, .message = "Request Timeout"},
    StatusMapping{.status = Status::kRequestEntityTooLarge, .message = "Payload Too Large"},
    StatusMapping{.status = Status::kRequestUriTooLong, .message = "URI Too Long"},
    StatusMapping{.status = Status::kInternalServerError, .message = "Internal Server Error"},
    StatusMapping{.status = Status::kMethodNotImplemented, .message = "Not Implemented"},
    StatusMapping{.status = Status::kVersionNotSupported, .message = "HTTP Version Not Supported"},
};

constexpr std::string_view GetStatusMessage(Status status) noexcept {
    const auto* const kIt = std::ranges::find(kStatusMessages, status, &StatusMapping::status);
    return kIt != kStatusMessages.end() ? kIt->message : std::string_view{};
}

static_assert(GetStatusMessage(Status::kNotFound) == "Not Found");
static_assert(GetStatusMessage(Status::kBadRequest) == "Bad Request");
static_assert(GetStatusMessage(Status::kUnknownError).empty());

enum class RequestMethod {
    kGet,
    kPost,
    kDelete,
    kUnknown, //
};

enum class ContentTypes {
    kTextHtml,
    kTextCss,
    kTextJs,
    kApplicationJson,
    kApplicationOctetStream,
    kNotDefined, //
};

inline constexpr char kRequestMethod[][8] = {
    "GET",
    "POST",
    "DELETE",
    "UNKNOWN", //
};

inline constexpr char kContentType[][26] = {
    "text/html",
    "text/css",
    "text/javascript",
    "application/json",
    "application/octet-stream", //
};
} // namespace http

#endif // HTTP_HTTP_H_
