#ifndef CONTENT_H_
#define CONTENT_H_

#include <cstdint>
#include "httpd/httpd.h"
#if !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#include "dmx.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#include "index.html.h"
#if !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER) || defined (RDM_RESPONDER))
#include "rdm.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER || defined (RDM_RESPONDER))
#include "styles.css.h"
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
#include "time.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#include "rtc.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#include "index.js.h"
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
#include "time.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)
#if !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER) || defined (RDM_RESPONDER))
#include "rdm.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER || defined (RDM_RESPONDER))
#if !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#include "dmx.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#include "showfile.html.h"
#include "date.js.h"
#include "default.js.h"
#if !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#include "pixeltype.json.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#if defined (NODE_SHOWFILE)
#include "showfile.js.h"
#endif // (NODE_SHOWFILE)
#include "static.js.h"
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#include "rtc.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)

struct FilesContent {
	uint32_t hash;
	const char *file_name;
	const uint8_t *content;
	uint32_t content_length;
	http::ContentTypes content_type;
	bool gzip;
};

inline constexpr struct FilesContent kHttpContent[] = {
#if !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
	{ 449885855,"dmx.js", dmx_js_gz, 592, static_cast<http::ContentTypes>(2), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
	{ 4024653090,"index.html", index_html_gz, 352, static_cast<http::ContentTypes>(0), true },
#if !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER) || defined (RDM_RESPONDER))
	{ 3794898249,"rdm.html", rdm_html_gz, 624, static_cast<http::ContentTypes>(0), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER || defined (RDM_RESPONDER))
	{ 2557875310,"styles.css", styles_css_gz, 237, static_cast<http::ContentTypes>(1), true },
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
	{ 1797555997,"time.js", time_js_gz, 223, static_cast<http::ContentTypes>(2), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
	{ 4194541735,"rtc.html", rtc_html_gz, 463, static_cast<http::ContentTypes>(0), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
	{ 247271700,"index.js", index_js_gz, 614, static_cast<http::ContentTypes>(2), true },
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
	{ 3879424355,"time.html", time_html_gz, 311, static_cast<http::ContentTypes>(0), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)
#if !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER) || defined (RDM_RESPONDER))
	{ 3336539475,"rdm.js", rdm_js_gz, 518, static_cast<http::ContentTypes>(2), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER || defined (RDM_RESPONDER))
#if !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
	{ 1521353325,"dmx.html", dmx_html_gz, 291, static_cast<http::ContentTypes>(0), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
	{ 92830953,"showfile.html", showfile_html_gz, 581, static_cast<http::ContentTypes>(0), true },
	{ 1602327546,"date.js", date_js_gz, 325, static_cast<http::ContentTypes>(2), true },
	{ 135667591,"default.js", default_js_gz, 220, static_cast<http::ContentTypes>(2), true },
#if !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
	{ 2632765249,"pixeltype.json", pixeltype_json_gz, 277, static_cast<http::ContentTypes>(3), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#if defined (NODE_SHOWFILE)
	{ 4266521075,"showfile.js", showfile_js_gz, 611, static_cast<http::ContentTypes>(2), true },
#endif // (NODE_SHOWFILE)
	{ 2932864356,"static.js", static_js_gz, 551, static_cast<http::ContentTypes>(2), true },
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
	{ 2872131065,"rtc.js", rtc_js_gz, 359, static_cast<http::ContentTypes>(2), true },
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
};


#endif /* CONTENT_H_ */
