#if defined(NODE_ARTNET) || defined(NODE_ARTNET_MULTI)
#include "config_artnet.js.h"
#endif // defined(NODE_ARTNET) || defined(NODE_ARTNET_MULTI)
#include "layout.js.h"
#include "status_index.html.h"
#include "index.html.h"
#if defined (CONFIG_HTTPD_ENABLE_UPLOAD)
#include "upload_index.js.h"
#endif // (CONFIG_HTTPD_ENABLE_UPLOAD)
#if defined (NODE_LTC_SMPTE)
#include "config_etc.js.h"
#endif // (NODE_LTC_SMPTE)
#if defined (NODE_OSC_SERVER)
#include "config_oscserver.js.h"
#endif // (NODE_OSC_SERVER)
#include "styles.css.h"
#include "status_index.js.h"
#if defined (NODE_LTC_SMPTE)
#include "config_gps.js.h"
#endif // (NODE_LTC_SMPTE)
#if defined (DMXNODE_PORTS)
#include "config_dmxnode.js.h"
#endif // (DMXNODE_PORTS)
#if !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#include "config_dmxpixel.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#include "index.js.h"
#if !defined (CONFIG_HTTP_HTML_NO_DMX_PCA9685) && defined(OUTPUT_DMX_PCA9685)
#include "config_dmxpca9685.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX_PCA9685) && defined(OUTPUT_DMX_PCA9685)
#include "config_oscclient.js.h"
#if !defined (CONFIG_HTTP_HTML_NO_DMX) && defined(OUTPUT_DMX_MONITOR)
#include "config_dmxmonitor.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX) && defined(OUTPUT_DMX_MONITOR)
#if defined (NODE_LTC_SMPTE)
#include "config_ltc.js.h"
#endif // (NODE_LTC_SMPTE)
#if defined (NODE_LTC_SMPTE)
#include "config_tcnet.js.h"
#endif // (NODE_LTC_SMPTE)
#if !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#include "rtc_index.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RTC) && !defined (DISABLE_RTC)
#if !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#include "status_pixel.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#if defined (NODE_SHOWFILE)
#include "status_showfile.js.h"
#endif // (NODE_SHOWFILE)
#if !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#include "status_dmx.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#if !defined (CONFIG_HTTP_HTML_NO_TIME)
#include "time_index.html.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_TIME)
#include "date.js.h"
#include "common.js.h"
#if defined(NODE_E131) || defined(NODE_E131_MULTI) || (ARTNET_VERSION == 4)
#include "config_e131.js.h"
#endif // defined(NODE_E131) || defined(NODE_E131_MULTI) || (ARTNET_VERSION == 4)
#if defined (NODE_LTC_SMPTE)
#include "config_ltcdisplays.js.h"
#endif // (NODE_LTC_SMPTE)
#if !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER) || defined (RDM_RESPONDER))
#include "config_rdmdevice.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_RDM) && (defined (RDM_CONTROLLER || defined (RDM_RESPONDER))
#if defined (NODE_SHOWFILE)
#include "config_showfile.js.h"
#endif // (NODE_SHOWFILE)
#if !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#include "config_dmxsend.js.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_DMX) && (defined(OUTPUT_DMX_SEND) || defined(OUTPUT_DMX_SEND_MULTI))
#if !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#include "pixeltype.json.h"
#endif // !defined (CONFIG_HTTP_HTML_NO_PIXEL) && (defined(OUTPUT_DMX_PIXEL) || defined(OUTPUT_DMX_PIXEL_MULTI))
#if defined (CONFIG_HTTPD_ENABLE_UPLOAD)
#include "upload_index.html.h"
#endif // (CONFIG_HTTPD_ENABLE_UPLOAD)
#if defined (DISPLAY_UDF)
#include "config_display.js.h"
#endif // (DISPLAY_UDF)
