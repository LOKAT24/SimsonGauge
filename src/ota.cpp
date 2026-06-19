// Over-the-air firmware update over a WiFi SoftAP.
//
// Flow: ota_request() raises a flag; ota_loop() (called from loop()) brings up
// the SoftAP + HTTP server on first request, then services upload requests.
// A phone connects to the AP, opens http://192.168.4.1, picks a .bin and the
// Update library flashes the inactive app partition and reboots.

#include "ota.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

#define OTA_AP_SSID  "MotoGauge-OTA"
#define OTA_AP_PASS  "motogauge"        // must be >= 8 chars
#define OTA_URL_STR  "http://192.168.4.1"

static WebServer      server(80);
static volatile bool  s_requested = false;
static bool           s_started   = false;

const char * ota_ssid(void) { return OTA_AP_SSID; }
const char * ota_pass(void) { return OTA_AP_PASS; }
const char * ota_url(void)  { return OTA_URL_STR; }
bool ota_is_active(void)    { return s_started; }
void ota_request(void)      { s_requested = true; }

static const char UPLOAD_PAGE[] PROGMEM =
    "<!doctype html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>MotoGauge OTA</title></head>"
    "<body style='font-family:sans-serif;text-align:center;margin-top:40px'>"
    "<h2>MotoGauge OTA</h2>"
    "<input type='file' id='f' accept='.bin'><br><br>"
    "<button onclick='up()' style='padding:10px 20px'>Wgraj firmware</button>"
    "<div style='margin-top:24px'>"
    "<progress id='p' value='0' max='100' style='width:80%;height:26px'></progress>"
    "<div id='s' style='margin-top:8px;font-size:20px'>0%</div></div>"
    "<script>"
    "function up(){"
    "var f=document.getElementById('f').files[0];"
    "if(!f){alert('Wybierz plik .bin');return;}"
    "var x=new XMLHttpRequest();x.open('POST','/update',true);"
    "x.upload.onprogress=function(e){if(e.lengthComputable){"
    "var pc=Math.round(e.loaded/e.total*100);"
    "document.getElementById('p').value=pc;"
    "document.getElementById('s').innerText=pc+'%';}};"
    "x.onload=function(){document.getElementById('s').innerHTML=x.responseText||'Done';};"
    "x.onerror=function(){document.getElementById('s').innerText='Blad polaczenia';};"
    "var fd=new FormData();fd.append('firmware',f);x.send(fd);"
    "}"
    "</script></body></html>";

static void handle_root(void)
{
    server.send_P(200, "text/html", UPLOAD_PAGE);
}

static void handle_update_finish(void)
{
    bool ok = !Update.hasError();
    server.send(200, "text/html",
                ok ? "<h3>OK - restart...</h3>" : "<h3>Blad aktualizacji</h3>");
    delay(800);
    if (ok) {
        ESP.restart();
    }
}

static void handle_update_upload(void)
{
    HTTPUpload &up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
        Update.begin(UPDATE_SIZE_UNKNOWN);
    } else if (up.status == UPLOAD_FILE_WRITE) {
        Update.write(up.buf, up.currentSize);
    } else if (up.status == UPLOAD_FILE_END) {
        Update.end(true);
    }
}

static void ota_start(void)
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(OTA_AP_SSID, OTA_AP_PASS);
    server.on("/", HTTP_GET, handle_root);
    server.on("/update", HTTP_POST, handle_update_finish, handle_update_upload);
    server.begin();
    s_started = true;
}

void ota_loop(void)
{
    if (s_requested && !s_started) {
        ota_start();
    }
    if (s_started) {
        server.handleClient();
    }
}
