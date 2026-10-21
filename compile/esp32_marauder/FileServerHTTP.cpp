#include "FileServerHTTP.h"
#include "configs.h"

#ifdef HAS_SD

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <ESPmDNS.h>     // mDNS: reach the server at cyd.fileserver.local on any network
#include <SD.h>
#include <FS.h>
#include <Preferences.h>
#include "esp_wifi.h"
#include "esp_bt.h"      // esp_bt_controller_mem_release -- reclaim BT DRAM for the TLS stack
#include "esp_heap_caps.h"   // heap_caps_get_largest_free_block -- OTA fragmentation diagnostic
#include "Display.h"

extern Display display_obj;

// mDNS name for the file server. Must be a SINGLE DNS label (letters/digits/hyphen,
// NO dot -- ESP32's mDNS can't advertise a dotted hostname). The ".local" suffix is
// mandatory, so the address to type is "cyd-fileserver.local" (works for both HTTP
// and FTP, on any network the board joins and on its own AP).
static const char *FS_MDNS_HOST = "cyd-fileserver";

// ---- small local helpers (kept local so this unit stays independent) ----
static String fshBaseName(const String &s) {
  int sl = s.lastIndexOf('/');
  return sl >= 0 ? s.substring(sl + 1) : s;
}
static void fshBtn(int x, int y, int w, int h, uint16_t col, const char *lab, uint8_t font) {
  display_obj.tft.fillRoundRect(x, y, w, h, 3, col);
  display_obj.tft.drawRoundRect(x, y, w, h, 3, TFT_WHITE);
  display_obj.tft.setTextColor(TFT_WHITE, col);
  display_obj.tft.setTextDatum(MC_DATUM);
  display_obj.tft.drawString(lab, x + w / 2, y + h / 2, font);
}
static String fsJoin(const String &dir, const String &name) {
  if (dir.endsWith("/")) return dir + name;
  return dir + "/" + name;
}
static void fsRmRf(const String &path) {           // recursive delete (folders + contents)
  File f = SD.open(path.c_str());
  if (!f) return;
  if (f.isDirectory()) {
    for (File e = f.openNextFile(); e; e = f.openNextFile()) {
      String cp = fsJoin(path, fshBaseName(e.name()));
      bool d = e.isDirectory(); e.close();
      if (d) fsRmRf(cp); else SD.remove(cp.c_str());
    }
    f.close();
    SD.rmdir(path.c_str());
  } else { f.close(); SD.remove(path.c_str()); }
}
// Make any uploaded file name safe for FAT so uploads never fail on the name.
static String fsSafeName(const String &raw) {
  String n = raw;
  int s = n.lastIndexOf('/');  if (s >= 0) n = n.substring(s + 1);
  s = n.lastIndexOf('\\');     if (s >= 0) n = n.substring(s + 1);
  String out = "";
  for (size_t i = 0; i < n.length(); i++) {
    char c = n[i];
    if (c < 0x20 || (uint8_t)c > 0x7E ||
        c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' ||
        c == '"'  || c == '<' || c == '>' || c == '|')
      out += '_';
    else out += c;
  }
  out.trim();
  while (out.endsWith(".")) out.remove(out.length() - 1);
  if (out.length() == 0) out = "upload.bin";
  if (out.length() > 64) {
    int dot = out.lastIndexOf('.');
    String ext = (dot >= 0 && (out.length() - dot) <= 8) ? out.substring(dot) : "";
    out = out.substring(0, 64 - ext.length()) + ext;
  }
  return out;
}

// ================= HTTP (synchronous WebServer, runs in the main loop) =======
static WebServer *g_http = nullptr;
static File g_upFile;
// Serving mode: when g_fsSta is true the server joined an existing Wi-Fi (station mode) and
// g_fsIP is its DHCP address on that network; otherwise it is the self-hosted AP's address.
static bool g_fsSta = false;
static IPAddress g_fsIP;

static String hArg(const char *name, const String &def = "") {
  return g_http->hasArg(name) ? g_http->arg(name) : def;
}
static void hRedirect(const String &loc) {
  g_http->sendHeader("Location", loc);
  g_http->send(303, "text/plain", "");
}
static void hList() {
  String p = hArg("dir", "/");
  if (p.length() == 0) p = "/";
  String html = "<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
                "<title>CYD SD</title><style>body{font-family:sans-serif;margin:12px}"
                "a{text-decoration:none}li{margin:6px 0}.d{color:#b58900;font-weight:bold}"
                ".f{color:#333}form{margin:10px 0}</style></head><body>";
  html += "<h3>" + p + "</h3>";
  if (p != "/") {
    int sl = p.lastIndexOf('/');
    String up = (sl <= 0) ? "/" : p.substring(0, sl);
    html += "<a href='/?dir=" + up + "'>&#8593; up</a><hr>";
  }
  html += "<ul>";
  File dir = SD.open(p.c_str());
  if (dir && dir.isDirectory()) {
    for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
      String nm = fshBaseName(e.name());
      String cp = fsJoin(p, nm);
      if (e.isDirectory()) {
        html += "<li><a class=d href='/?dir=" + cp + "'>[" + nm + "]</a> "
                "<a href='/rm?path=" + cp + "' onclick=\"return confirm('Delete folder " + nm + " and all contents?')\">[del]</a></li>";
      } else {
        html += "<li><span class=f>" + nm + "</span> (" + String((uint32_t)e.size()) + "B) "
                "<a href='/dl?path=" + cp + "'>[get]</a> "
                "<a href='/rm?path=" + cp + "'>[del]</a></li>";
      }
      e.close();
    }
  }
  if (dir) dir.close();
  html += "</ul><hr>";
  html += "<form action='/mkdir'><input type=hidden name=dir value='" + p + "'>"
          "New folder: <input name=name><input type=submit value='Create'></form>";
  html += "<form method='POST' action='/upload?dir=" + p + "' enctype='multipart/form-data'>"
          "Upload here: <input type=file name=f multiple><input type=submit value='Upload'></form>";
  html += "<p style='color:#888'>Tip: enter a folder first, then upload into it.</p>";
  html += "</body></html>";
  g_http->send(200, "text/html", html);
}
static void hDownload() {
  String path = hArg("path", "");
  if (path.length() && SD.exists(path.c_str())) {
    File f = SD.open(path.c_str());
    if (f && !f.isDirectory()) {
      g_http->sendHeader("Content-Disposition", "attachment; filename=\"" + fshBaseName(path) + "\"");
      g_http->streamFile(f, "application/octet-stream");
      f.close();
      return;
    }
    if (f) f.close();
  }
  g_http->send(404, "text/plain", "not found");
}
static void hDelete() {
  String path = hArg("path", "");
  int sl = path.lastIndexOf('/');
  String parent = (sl <= 0) ? "/" : path.substring(0, sl);
  if (path.length() > 1) fsRmRf(path);
  hRedirect("/?dir=" + parent);
}
static void hMkdir() {
  String dir = hArg("dir", "/");
  String name = hArg("name", "");
  if (name.length()) SD.mkdir(fsJoin(dir, name).c_str());
  hRedirect("/?dir=" + dir);
}
// Upload: handleClient() in the main loop streams the POST body here; SD writes
// happen in the main loop (ample stack), so a big firmware .bin uploads fine.
static void hUpload() {
  HTTPUpload &up = g_http->upload();
  if (up.status == UPLOAD_FILE_START) {
    String dir = hArg("dir", "/");
    if (dir.length() == 0) dir = "/";
    String path = fsJoin(dir, fsSafeName(up.filename));
    if (SD.exists(path.c_str())) SD.remove(path.c_str());
    g_upFile = SD.open(path.c_str(), FILE_WRITE);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (g_upFile) g_upFile.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END || up.status == UPLOAD_FILE_ABORTED) {
    if (g_upFile) { g_upFile.flush(); g_upFile.close(); }
  }
}

// ================= Minimal FTP server (passive mode) =========================
static WiFiServer *g_ftpCtrl = nullptr;
static WiFiClient g_ftpCli;
static WiFiServer *g_ftpData = nullptr;
static String g_ftpCwd = "/";
static String g_ftpRnFrom = "";
static int g_ftpPasvPort = 50009;

static String ftpAbs(const String &arg) {
  String p;
  if (arg.length() == 0) return g_ftpCwd;
  if (arg[0] == '/') p = arg;
  else p = (g_ftpCwd == "/" ? "" : g_ftpCwd) + "/" + arg;
  while (p.length() > 1 && p.endsWith("/")) p.remove(p.length() - 1);
  if (p.length() == 0) p = "/";
  return p;
}
static WiFiClient ftpWaitData() {
  uint32_t t = millis();
  while (millis() - t < 6000) {
    WiFiClient dc = g_ftpData->available();
    if (dc) return dc;
    delay(10);
  }
  return WiFiClient();
}
static void ftpCloseData() { if (g_ftpData) { g_ftpData->stop(); delete g_ftpData; g_ftpData = nullptr; } }

static void ftpCmd(String line) {
  int sp = line.indexOf(' ');
  String cmd = (sp < 0 ? line : line.substring(0, sp)); cmd.toUpperCase();
  String arg = (sp < 0 ? "" : line.substring(sp + 1));

  if (cmd == "USER") g_ftpCli.println("331 Password required");
  else if (cmd == "PASS") g_ftpCli.println("230 Login successful");
  else if (cmd == "SYST") g_ftpCli.println("215 UNIX Type: L8");
  else if (cmd == "FEAT") g_ftpCli.println("211 End");
  else if (cmd == "OPTS") g_ftpCli.println("200 OK");
  else if (cmd == "NOOP") g_ftpCli.println("200 OK");
  else if (cmd == "TYPE") g_ftpCli.println("200 Type set");
  else if (cmd == "PWD" || cmd == "XPWD") g_ftpCli.println("257 \"" + g_ftpCwd + "\" is current directory");
  else if (cmd == "QUIT") { g_ftpCli.println("221 Bye"); g_ftpCli.stop(); }
  else if (cmd == "CWD") {
    String np;
    if (arg == "..") { int s = g_ftpCwd.lastIndexOf('/'); np = (s <= 0) ? "/" : g_ftpCwd.substring(0, s); }
    else np = ftpAbs(arg);
    File d = SD.open(np.c_str());
    if (d && d.isDirectory()) { g_ftpCwd = np; g_ftpCli.println("250 OK"); }
    else g_ftpCli.println("550 No such dir");
    if (d) d.close();
  }
  else if (cmd == "CDUP") { int s = g_ftpCwd.lastIndexOf('/'); g_ftpCwd = (s <= 0) ? "/" : g_ftpCwd.substring(0, s); g_ftpCli.println("250 OK"); }
  else if (cmd == "PASV") {
    ftpCloseData();
    g_ftpPasvPort++; if (g_ftpPasvPort > 50020) g_ftpPasvPort = 50009;
    g_ftpData = new WiFiServer(g_ftpPasvPort); g_ftpData->begin();
    IPAddress ip = g_fsIP;              // AP IP or the joined-network DHCP IP
    char b[72];
    snprintf(b, sizeof(b), "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
             ip[0], ip[1], ip[2], ip[3], g_ftpPasvPort / 256, g_ftpPasvPort % 256);
    g_ftpCli.println(b);
  }
  else if (cmd == "LIST" || cmd == "NLST") {
    if (!g_ftpData) { g_ftpCli.println("425 Use PASV first"); return; }
    g_ftpCli.println("150 Here comes the listing");
    WiFiClient dc = ftpWaitData();
    if (dc) {
      File dir = SD.open(g_ftpCwd.c_str());
      if (dir && dir.isDirectory()) {
        for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
          String nm = fshBaseName(e.name());
          if (cmd == "NLST") dc.print(nm + "\r\n");
          else {
            String ln = (e.isDirectory() ? "drwxr-xr-x" : "-rw-r--r--");
            ln += " 1 cyd cyd " + String(e.isDirectory() ? 0 : (uint32_t)e.size()) + " Jan 01 00:00 " + nm;
            dc.print(ln + "\r\n");
          }
          e.close();
        }
      }
      if (dir) dir.close();
      dc.stop();
      g_ftpCli.println("226 Directory send OK");
    } else g_ftpCli.println("425 Can't open data connection");
    ftpCloseData();
  }
  else if (cmd == "RETR") {
    String path = ftpAbs(arg);
    File f = SD.open(path.c_str());
    if (!f || f.isDirectory()) { if (f) f.close(); g_ftpCli.println("550 Not found"); ftpCloseData(); return; }
    g_ftpCli.println("150 Opening data connection");
    WiFiClient dc = ftpWaitData();
    if (dc) {
      static uint8_t buf[1024];
      while (f.available() && dc.connected()) {
        int n = f.read(buf, sizeof(buf));
        if (n > 0) dc.write(buf, n);
        vTaskDelay(1);
      }
      dc.stop();
      g_ftpCli.println("226 Transfer complete");
    } else g_ftpCli.println("425 Can't open data connection");
    f.close();
    ftpCloseData();
  }
  else if (cmd == "STOR" || cmd == "APPE") {
    String path = ftpAbs(arg);
    g_ftpCli.println("150 Ok to send data");
    WiFiClient dc = ftpWaitData();
    if (dc) {
      if (cmd == "STOR" && SD.exists(path.c_str())) SD.remove(path.c_str());
      File f = SD.open(path.c_str(), cmd == "APPE" ? FILE_APPEND : FILE_WRITE);
      static uint8_t buf[1024];
      uint32_t lastData = millis();
      while (true) {
        int n = dc.read(buf, sizeof(buf));
        if (n > 0) { if (f) f.write(buf, n); lastData = millis(); }
        else {
          if (!dc.connected() && !dc.available()) break;
          if (millis() - lastData > 15000) break;
        }
        vTaskDelay(1);                 // yield so the watchdog is fed
      }
      if (f) { f.flush(); f.close(); }
      dc.stop();
      g_ftpCli.println("226 Transfer complete");
    } else g_ftpCli.println("425 Can't open data connection");
    ftpCloseData();
  }
  else if (cmd == "DELE") { String p = ftpAbs(arg); g_ftpCli.println(SD.remove(p.c_str()) ? "250 OK" : "550 Delete failed"); }
  else if (cmd == "MKD" || cmd == "XMKD") { String p = ftpAbs(arg); g_ftpCli.println(SD.mkdir(p.c_str()) ? ("257 \"" + p + "\" created") : "550 Create failed"); }
  else if (cmd == "RMD" || cmd == "XRMD") { String p = ftpAbs(arg); fsRmRf(p); g_ftpCli.println("250 OK"); }
  else if (cmd == "RNFR") { g_ftpRnFrom = ftpAbs(arg); g_ftpCli.println(SD.exists(g_ftpRnFrom.c_str()) ? "350 Ready for RNTO" : "550 Not found"); }
  else if (cmd == "RNTO") { String p = ftpAbs(arg); g_ftpCli.println(SD.rename(g_ftpRnFrom.c_str(), p.c_str()) ? "250 OK" : "550 Rename failed"); }
  else if (cmd == "SIZE") { String p = ftpAbs(arg); File f = SD.open(p.c_str()); if (f && !f.isDirectory()) { g_ftpCli.println("213 " + String((uint32_t)f.size())); } else g_ftpCli.println("550 Not a file"); if (f) f.close(); }
  else g_ftpCli.println("502 Not implemented");
}
static void ftpLoop() {
  if (!g_ftpCtrl) return;
  if (!g_ftpCli || !g_ftpCli.connected()) {
    WiFiClient nc = g_ftpCtrl->available();
    if (nc) { g_ftpCli = nc; g_ftpCwd = "/"; g_ftpCli.println("220 CYD FTP ready"); }
    return;
  }
  if (g_ftpCli.available()) {
    String line = g_ftpCli.readStringUntil('\n');
    line.replace("\r", ""); line.trim();
    if (line.length()) ftpCmd(line);
  }
}

// ================= device-join approval (Allow persists, Deny asks each time) =
static volatile bool g_joinPending = false;
static uint8_t g_joinMac[6];
static uint16_t g_joinAid = 0;
static String g_allowMacs = "";
static String fsMacStr(const uint8_t *m) {
  char b[18];
  snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return String(b);
}
// Approved MACs live as files in "/macs" on the SD card (one file per device,
// named after the MAC with ':' -> '-'), so the allow-list survives even a full
// firmware erase. The SD folder is the single source of truth: to revoke a
// device, delete its file in /macs (e.g. from the web browser).
static String macToFile(const String &mac) { String f = mac; f.replace(":", "-"); return f; }
static String fileToMac(const String &f)   { String m = f;   m.replace("-", ":"); return m; }

static void ensureMacFile(const String &mac) {
  if (!SD.exists("/macs")) SD.mkdir("/macs");
  String path = "/macs/" + macToFile(mac);
  if (!SD.exists(path.c_str())) {
    File f = SD.open(path.c_str(), FILE_WRITE);
    if (f) { f.println(mac); f.close(); }
  }
}
static void fsLoadAllow() {
  g_allowMacs = "";
  File d = SD.open("/macs");
  if (d && d.isDirectory()) {
    for (File e = d.openNextFile(); e; e = d.openNextFile()) {
      if (!e.isDirectory()) {
        String mac = fileToMac(fshBaseName(e.name()));
        if (mac.length() >= 17 && g_allowMacs.indexOf(mac) < 0) g_allowMacs += mac + ";";
      }
      e.close();
    }
  }
  if (d) d.close();
}
static void fsAddAllowed(const String &mac) { ensureMacFile(mac); }
// Wipe all approvals: empty the in-RAM list and delete every file in /macs.
static void fsForgetAll() {
  g_allowMacs = "";
  File d = SD.open("/macs");
  if (d && d.isDirectory()) {
    for (File e = d.openNextFile(); e; e = d.openNextFile()) {
      String nm = fshBaseName(e.name());
      bool isdir = e.isDirectory();
      e.close();
      if (!isdir) SD.remove(("/macs/" + nm).c_str());
    }
  }
  if (d) d.close();
}
static void fsWiFiEvent(WiFiEvent_t ev, WiFiEventInfo_t info) {
  if (ev == ARDUINO_EVENT_WIFI_AP_STACONNECTED) {
    memcpy(g_joinMac, info.wifi_ap_staconnected.mac, 6);
    g_joinAid = info.wifi_ap_staconnected.aid;
    g_joinPending = true;
  }
}
// Scan for Wi-Fi and pick the STRONGEST network for which /wifi_creds/<SSID>.txt exists.
// Each file in that folder is named after the network SSID; its contents are the password
// (first non-empty line). Returns true + the chosen ssid/pass if a saved network is in range.
static bool fsPickNetwork(String &outSsid, String &outPass) {
  if (!SD.exists("/wifi_creds")) return false;
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setFreeFont(NULL); display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawString("Scanning Wi-Fi...", 10, 12, 2);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int found = WiFi.scanNetworks();          // blocking scan
  if (found <= 0) { WiFi.scanDelete(); return false; }
  int bestRssi = -1000; String bestSsid, bestPass;
  File dir = SD.open("/wifi_creds");
  if (dir && dir.isDirectory()) {
    for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
      if (!e.isDirectory()) {
        String fn = fshBaseName(e.name());
        if (fn.endsWith(".txt")) {
          String credSsid = fn.substring(0, fn.length() - 4);
          String pw = "";
          while (e.available()) { pw = e.readStringUntil('\n'); pw.trim(); if (pw.length()) break; }
          for (int i = 0; i < found; i++) {
            if (WiFi.SSID(i) == credSsid && WiFi.RSSI(i) > bestRssi) {
              bestRssi = WiFi.RSSI(i); bestSsid = credSsid; bestPass = pw;
            }
          }
        }
      }
      e.close();
    }
  }
  if (dir) dir.close();
  WiFi.scanDelete();
  if (bestSsid.length()) { outSsid = bestSsid; outPass = bestPass; return true; }
  return false;
}

static void fsDrawInfo(const String &ssid, const String &pass, IPAddress ip) {
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawString("Wi-Fi File Server", 10, 10, 2);
  if (g_fsSta) {
    // Joined an existing network: no AP credentials to show, just which network + the IP.
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("Joined your Wi-Fi:", 10, 44, 1);
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawString(ssid, 10, 62, 2);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("Browser:", 10, 110, 1);
    display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    display_obj.tft.drawString("http://" + ip.toString() + "/", 70, 108, 2);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("FTP:", 10, 140, 1);
    display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    display_obj.tft.drawString("ftp://" + ip.toString() + "/", 70, 138, 2);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("Name (http+ftp, any net):", 10, 166, 1);
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawString("cyd-fileserver.local", 10, 180, 2);
    display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    display_obj.tft.drawString("FTP login: any user / pass", 10, 206, 1);
  } else {
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("1) Join this Wi-Fi:", 10, 44, 1);
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawString("SSID:", 10, 62, 1);
    display_obj.tft.drawString(ssid, 60, 62, 2);
    display_obj.tft.drawString("PASS:", 10, 86, 1);
    display_obj.tft.drawString(pass, 60, 86, 2);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("2) Browser:", 10, 116, 1);
    display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    display_obj.tft.drawString("http://" + ip.toString() + "/", 90, 114, 1);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("   Explorer/FTP client:", 10, 134, 1);
    display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    display_obj.tft.drawString("ftp://" + ip.toString() + "/", 10, 150, 2);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("Name:", 10, 170, 1);
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawString("cyd-fileserver.local", 50, 169, 1);
    display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    display_obj.tft.drawString("FTP login: any user / pass", 10, 184, 1);
    display_obj.tft.drawString("Approve devices as they join.", 10, 198, 1);
  }
  fshBtn(12, 270, 100, 34, TFT_MAROON, "Stop", 2);
  if (!g_fsSta) fshBtn(128, 270, 100, 34, TFT_NAVY, "Forget", 2);   // approvals only in AP mode
}
// Draw the live connected-client count in its own area (updated each loop).
static void fshDrawClients(int n) {
  display_obj.tft.fillRect(10, 214, 220, 16, TFT_BLACK);
  display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  display_obj.tft.drawString("Connected clients: " + String(n), 10, 216, 1);
}

// Show the Allow/Deny popup for a device MAC. Keeps serving HTTP while up.
// On Deny, deauth the station (aid from the connect event).
static void fshPrompt(const String &ssid, const String &pass, IPAddress ip,
                      const String &mac, uint16_t aid) {
  uint16_t tx, ty;
  display_obj.tft.fillRect(10, 90, 220, 150, TFT_BLACK);
  display_obj.tft.drawRect(10, 90, 220, 150, TFT_WHITE);
  display_obj.tft.setTextDatum(MC_DATUM); display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  display_obj.tft.drawString("Device wants to join", 120, 108, 2);
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.drawString(mac, 120, 134, 2);
  fshBtn(24, 170, 90, 40, TFT_DARKGREEN, "Allow", 2);
  fshBtn(126, 170, 90, 40, TFT_MAROON, "Deny", 2);
  bool decided = false;
  while (!decided) {
    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      if (ty >= 168 && ty <= 212) {
        if (tx >= 20 && tx <= 118) { g_allowMacs += mac + ";"; fsAddAllowed(mac); decided = true; }  // Allow
        else if (tx >= 122 && tx <= 220) { esp_wifi_deauth_sta(aid); decided = true; }               // Deny
      }
      while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
    }
    g_http->handleClient();
    delay(20);
  }
  fsDrawInfo(ssid, pass, ip);
}

void runFileServer() {
  String ssid = "CYD_FileServer";
  String pass = "1234567890";

  g_joinPending = false; fsLoadAllow();
  g_fsSta = false;

  WiFi.onEvent(fsWiFiEvent);

  // Prefer joining one of your own networks (strongest /wifi_creds match in range). If that
  // succeeds, the server runs on that network (station mode); otherwise host the AP as before.
  String joinSsid, joinPass;
  if (fsPickNetwork(joinSsid, joinPass)) {
    display_obj.tft.fillScreen(TFT_BLACK);
    display_obj.tft.setFreeFont(NULL); display_obj.tft.setTextDatum(TL_DATUM);
    display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
    display_obj.tft.drawString("Joining " + joinSsid + " ...", 10, 12, 2);
    WiFi.mode(WIFI_STA);
    WiFi.begin(joinSsid.c_str(), joinPass.c_str());
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 12000) delay(200);
    if (WiFi.status() == WL_CONNECTED) { g_fsSta = true; ssid = joinSsid; }
  }

  IPAddress ip;
  if (g_fsSta) {
    ip = WiFi.localIP();                 // our DHCP address on your network
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid.c_str(), pass.c_str(), 1 /*channel*/, 0 /*visible*/, 4 /*max clients*/);
    esp_wifi_set_max_tx_power(78);
    delay(300);
    ip = WiFi.softAPIP();
  }
  g_fsIP = ip;

  g_http = new WebServer(80);
  g_http->on("/", HTTP_GET, hList);
  g_http->on("/dl", HTTP_GET, hDownload);
  g_http->on("/rm", HTTP_GET, hDelete);
  g_http->on("/mkdir", HTTP_GET, hMkdir);
  g_http->on("/upload", HTTP_POST, []() { hRedirect("/?dir=" + hArg("dir", "/")); }, hUpload);
  g_http->begin();

  g_ftpCtrl = new WiFiServer(21);
  g_ftpCtrl->begin();
  g_ftpCwd = "/";

  // Advertise cyd.fileserver.local so the same name reaches the board on any
  // network (and on its own AP), for both the web UI and FTP. Works whether we
  // joined your Wi-Fi (STA) or are hosting the fallback AP.
  MDNS.end();                                  // in case a previous run left it up
  if (MDNS.begin(FS_MDNS_HOST)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ftp",  "tcp", 21);
  }

  fsDrawInfo(ssid, pass, ip);

  if (!g_fsSta) fshDrawClients(0);

  uint16_t tx, ty;
  uint32_t lastScan = 0;
  int lastCount = -1;
  while (true) {
    g_http->handleClient();
    ftpLoop();

    // --- Approval: poll the AP's connected-station list every ~500 ms. This
    //     does NOT rely on the Wi-Fi connect event. Any connected device whose
    //     MAC is not in /macs gets an Allow/Deny prompt, and the live client
    //     count is shown so you can confirm the board sees the connection.
    //     AP mode only -- in station mode clients come via your router. ---
    if (!g_fsSta && millis() - lastScan > 500) {
      lastScan = millis();

      int n = WiFi.softAPgetStationNum();
      if (n != lastCount) { lastCount = n; fshDrawClients(n); }

      wifi_sta_list_t stas;
      if (esp_wifi_ap_get_sta_list(&stas) == ESP_OK) {
        for (int i = 0; i < stas.num; i++) {
          String mac = fsMacStr(stas.sta[i].mac);
          if (g_allowMacs.indexOf(mac) < 0) {
            uint16_t aid = (mac == fsMacStr(g_joinMac)) ? g_joinAid : 0;
            fshPrompt(ssid, pass, ip, mac, aid);
            lastCount = -1;            // force count redraw after the popup
            break;                     // one prompt per scan
          }
        }
      }
    }

    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      if (ty >= 266 && ty <= 308) {
        if (tx >= 8 && tx <= 116) {                 // Stop
          while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
          break;
        } else if (!g_fsSta && tx >= 124 && tx <= 232) {   // Forget: wipe approvals (AP mode only)
          while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
          fsForgetAll();
          display_obj.tft.setTextDatum(MC_DATUM);
          display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
          display_obj.tft.fillRect(10, 240, 220, 20, TFT_BLACK);
          display_obj.tft.drawString("All devices forgotten", 120, 250, 1);
          display_obj.tft.setTextDatum(TL_DATUM);
          lastCount = -1;
        }
      }
    }
    delay(2);
  }

  MDNS.end();
  g_http->stop();
  delete g_http; g_http = nullptr;
  if (g_upFile) g_upFile.close();
  if (g_ftpCli) g_ftpCli.stop();
  ftpCloseData();
  if (g_ftpCtrl) { g_ftpCtrl->stop(); delete g_ftpCtrl; g_ftpCtrl = nullptr; }
  if (g_fsSta) WiFi.disconnect(true); else WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

// ================= Wi-Fi OTA update from the latest GitHub release =================
static const char *WU_REPO = "dorukerel3/CYD_Marauder";   // owner/repo

static void wuSay(int y, const String &s, uint16_t col) {
  display_obj.tft.fillRect(0, y, TFT_WIDTH, 18, TFT_BLACK);
  display_obj.tft.setFreeFont(NULL); display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setTextColor(col, TFT_BLACK);
  display_obj.tft.drawString(s, 8, y, 1);
}

// Find the first asset whose "name" ends in .bin in a releases/latest JSON response.
// (Release titles rarely end in .bin, so the first .bin-named field is the asset we want.)
static String wuFindAssetName(const String &j) {
  int from = 0;
  while (true) {
    int k = j.indexOf("\"name\"", from);
    if (k < 0) break;
    int q1 = j.indexOf('"', j.indexOf(':', k) + 1);
    int q2 = (q1 >= 0) ? j.indexOf('"', q1 + 1) : -1;
    if (q1 < 0 || q2 < 0) break;
    String name = j.substring(q1 + 1, q2);
    if (name.endsWith(".bin")) return name;
    from = q2 + 1;
  }
  return "";
}

// Menu entry point: don't do OTA here (the loaded UI leaves too little contiguous RAM for the
// TLS handshake -> "API error -1 ... b29k"). Flag it and reboot; setup() runs it while RAM is clean.
void requestWifiUpdate() {
  Preferences p;
  if (p.begin("marauder", false)) { p.putBool("otaPend", true); p.putInt("otaPhase", 0); p.end(); }
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setFreeFont(NULL); display_obj.tft.setTextDatum(MC_DATUM);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawString("Rebooting for update...", TFT_WIDTH / 2, TFT_HEIGHT / 2, 2);
  display_obj.tft.setTextDatum(TL_DATUM);
  delay(1200);
  ESP.restart();
}

void runWifiUpdate() {
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setFreeFont(NULL); display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawString("Wi-Fi Update", 10, 8, 2);

  const char *OTA_FILE = "/fw_ota.bin";

  // Which phase are we in? (persisted across the reboots this routine triggers)
  Preferences _p; int _phase = 0; String assetName;
  if (_p.begin("marauder", false)) { _phase = _p.getInt("otaPhase", 0); assetName = _p.getString("otaName", ""); _p.end(); }

  // PHASE 2 - FLASH FROM SD. No Wi-Fi, no TLS -> maximum free RAM. The .bin was
  // already saved to SD in phase 1; here we only write it to the spare app slot
  // and reboot. Downloading (TLS) and flashing (Update) never run together now,
  // which is what was starving RAM and crashing the download near 100%.
  if (_phase == 2) {
    if (_p.begin("marauder", false)) { _p.putInt("otaPhase", 0); _p.remove("otaName"); _p.end(); }
    wuSay(40, "Flashing from SD...", TFT_WHITE);
    File fw = SD.open(OTA_FILE, FILE_READ);
    if (!fw || fw.isDirectory() || fw.size() == 0) {
      if (fw) fw.close();
      wuSay(60, "No downloaded file.", TFT_RED); delay(4000); return;
    }
    size_t total = fw.size();
    size_t appOffset = 0;                       // merged 0x0 image? skip to the app
    if (total > 0x10000) {
      uint8_t sig[2] = {0, 0}; fw.seek(0x8000); fw.read(sig, 2); fw.seek(0);
      if (sig[0] == 0xAA && sig[1] == 0x50) appOffset = 0x10000;
    }
    size_t appSize = total - appOffset;
    wuSay(60, "Writing " + String(appSize / 1024) + " KB...", TFT_WHITE);
    bool ok = false;
    if (appSize > 0 && Update.begin(appSize)) {
      fw.seek(appOffset);
      size_t written = Update.writeStream(fw);
      ok = (written == appSize) && Update.end(true) && Update.isFinished();
      if (!ok) Update.abort();
    }
    fw.close();
    SD.remove(OTA_FILE);                         // don't re-flash on the next boot
    if (ok) { wuSay(80, "Updated! Rebooting...", TFT_GREEN); delay(1500); ESP.restart(); }
    else    { wuSay(80, "Flash failed (err " + String(Update.getError()) + ")", TFT_RED); delay(4000); }
    return;
  }

  // 1) Join Wi-Fi. In phase 0 (discovery) scan and pick the strongest saved network,
  //    then REMEMBER that exact SSID/password so phase 1 (download) rejoins the SAME
  //    network instead of re-scanning (which could otherwise land on a different one).
  String ssid, pass;
  bool haveSaved = false;
  if (_phase != 0) {
    Preferences np;
    if (np.begin("marauder", true)) { ssid = np.getString("otaSsid", ""); pass = np.getString("otaPass", ""); np.end(); }
    haveSaved = ssid.length() > 0;
  }
  if (!haveSaved && !fsPickNetwork(ssid, pass)) {
    wuSay(40, "No saved Wi-Fi in range.", TFT_ORANGE);
    wuSay(60, "Add /wifi_creds/<SSID>.txt", TFT_LIGHTGREY);
    delay(3500); WiFi.mode(WIFI_OFF); return;
  }
  wuSay(40, "Joining " + ssid + " ...", TFT_WHITE);
  WiFi.mode(WIFI_STA); WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(200);
  if (WiFi.status() != WL_CONNECTED) { wuSay(40, "Wi-Fi join failed.", TFT_RED); delay(3000); WiFi.mode(WIFI_OFF); return; }
  wuSay(40, "Connected: " + WiFi.localIP().toString(), TFT_GREEN);

  // 1b) Wait PATIENTLY for DNS. ESP32 DNS is frequently not ready the instant Wi-Fi associates,
  //     which showed up as "API error -1". Resolve api.github.com with up to ~20 tries, 1s apart,
  //     before touching TLS -- and warm the lwip DNS cache so the HTTPS connect resolves instantly.
  IPAddress ghip;
  bool dnsOk = false;
  for (int i = 0; i < 20 && !dnsOk; i++) {
    dnsOk = WiFi.hostByName("api.github.com", ghip) && ghip != IPAddress(0, 0, 0, 0);
    wuSay(55, dnsOk ? ("DNS ok: " + ghip.toString()) : ("Resolving DNS... " + String(i + 1)),
          dnsOk ? TFT_LIGHTGREY : TFT_WHITE);
    if (!dnsOk) delay(1000);
  }
  if (!dnsOk) { wuSay(55, "DNS failed - try another network.", TFT_RED); delay(5000); WiFi.mode(WIFI_OFF); return; }

  // 2) Reclaim the ~64KB Bluetooth controller DRAM. On this RAM-tight board there is only enough
  //    contiguous heap for ONE TLS connection, so we do NOT call api.github.com at all (that first
  //    handshake fragmented the heap and starved the download). Controller must be IDLE: deinit,
  //    then release.
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) esp_bt_controller_disable();
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)  esp_bt_controller_deinit();
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

  // 3) Two-phase so the release asset can have ANY name (not just firmware.bin). Each phase runs
  //    on its own clean boot and uses only ONE TLS session, because this board has contiguous heap
  //    for just one handshake. Phase 0 asks the API for the latest release's first .bin NAME and
  //    stores it; phase 1 downloads /releases/latest/download/<that name> to SD.
  if (_phase == 0) {
    wuSay(60, "Finding firmware...", TFT_WHITE);
    WiFiClientSecure *asec = new WiFiClientSecure();
    if (!asec) { wuSay(60, "Low memory (API).", TFT_RED); delay(4000); WiFi.mode(WIFI_OFF); return; }
    asec->setInsecure(); asec->setHandshakeTimeout(30);
    HTTPClient api; api.setConnectTimeout(20000); api.setTimeout(20000); api.setReuse(false);
    String name = ""; int code = -1;
    if (api.begin(*asec, String("https://api.github.com/repos/") + WU_REPO + "/releases/latest")) {
      api.addHeader("User-Agent", "CYD-Marauder");
      code = api.GET();
      if (code == 200) { String body = api.getString(); name = wuFindAssetName(body); }
    }
    api.end(); delete asec;
    if (!name.length()) {
      wuSay(60, "No .bin in release (" + String(code) + ") f" + String(ESP.getFreeHeap() / 1024)
               + "k b" + String(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024) + "k", TFT_RED);
      delay(6000); WiFi.mode(WIFI_OFF); return;            // otaPend already cleared -> normal boot
    }
    // Arm phase 1 (download the discovered name) and reboot onto a clean heap for the
    // handshake. Save the network we just used so phase 1 rejoins the SAME one.
    if (_p.begin("marauder", false)) {
      _p.putString("otaName", name); _p.putInt("otaPhase", 1); _p.putBool("otaPend", true);
      _p.putString("otaSsid", ssid); _p.putString("otaPass", pass); _p.end();
    }
    wuSay(60, "Found: " + name, TFT_GREEN);
    wuSay(80, "Downloading on reboot...", TFT_WHITE);
    delay(1400); ESP.restart();
  }

  // Phase 1: download the discovered asset BY NAME, straight to SD (we do NOT touch
  //    the flash here). Reset the phase first so a failure returns to a normal boot.
  if (!assetName.length()) assetName = "firmware.bin";     // safety fallback
  if (_p.begin("marauder", false)) { _p.putInt("otaPhase", 0); _p.remove("otaName");
    _p.remove("otaSsid"); _p.remove("otaPass"); _p.end(); }
  String url = String("https://github.com/") + WU_REPO + "/releases/latest/download/" + assetName;
  wuSay(60, "Downloading " + assetName + " ...", TFT_WHITE);

  SD.remove(OTA_FILE);                                   // clear any stale/partial file
  File out = SD.open(OTA_FILE, FILE_WRITE);
  if (!out) { wuSay(80, "SD open failed.", TFT_RED); delay(4000); WiFi.mode(WIFI_OFF); return; }

  WiFiClientSecure *dsec = nullptr;
  HTTPClient dl;
  int code2 = -1;
  for (int attempt = 0; attempt < 4 && code2 != 200; attempt++) {
    if (attempt) { wuSay(80, "Retry dl " + String(attempt) + " ...", TFT_ORANGE); delay(2500); }
    dl.end();
    if (dsec) { delete dsec; dsec = nullptr; }
    dsec = new WiFiClientSecure();
    if (!dsec) { out.close(); SD.remove(OTA_FILE); wuSay(80, "Low memory (download).", TFT_RED); delay(4000); WiFi.mode(WIFI_OFF); return; }
    dsec->setInsecure();
    dsec->setHandshakeTimeout(30);
    dl.setConnectTimeout(20000);
    dl.setTimeout(20000);
    dl.setReuse(false);
    dl.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (dl.begin(*dsec, url)) {
      dl.addHeader("User-Agent", "CYD-Marauder");
      code2 = dl.GET();
    }
    if (code2 > 0 && code2 != 200) break;               // a real HTTP error (e.g. 404) -- retry won't help
  }
  if (code2 != 200) {
    out.close(); SD.remove(OTA_FILE);
    wuSay(80, "Download error " + String(code2) + " f" + String(ESP.getFreeHeap() / 1024)
             + "k b" + String(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024) + "k", TFT_RED);
    dl.end(); if (dsec) delete dsec; delay(6000); WiFi.mode(WIFI_OFF); return;
  }
  int total = dl.getSize();                             // -1 if unknown
  WiFiClient *st = dl.getStreamPtr();
  uint8_t buf[1024];
  uint32_t written = 0;
  bool ok = true;
  uint32_t lastDraw = 0, lastData = millis();
  while (true) {
    if (total >= 0 && (int)written >= total) break;    // got it all
    if (!dl.connected() && !st->available()) {
      if (total < 0) break;                            // unknown length: EOF = done
      if (millis() - lastData > 8000) { ok = false; break; }         // stalled
    }
    int avail = st->available();
    if (avail <= 0) { delay(5); continue; }
    lastData = millis();
    int n = st->readBytes(buf, avail > (int)sizeof(buf) ? (int)sizeof(buf) : avail);
    if (n <= 0) { delay(2); continue; }
    if (out.write(buf, n) != (size_t)n) { ok = false; break; }       // SD write failed / card full
    written += n;
    if (millis() - lastDraw > 400) {
      if (total > 0) wuSay(80, "Downloading " + String((long)(written * 100 / total)) + "%", TFT_WHITE);
      else           wuSay(80, "Downloading " + String(written / 1024) + " KB", TFT_WHITE);
      lastDraw = millis();
    }
    delay(1);                                           // keep the watchdog fed
  }
  out.flush(); out.close();
  dl.end();
  if (dsec) delete dsec;
  WiFi.mode(WIFI_OFF);

  if (ok && written > 0 && (total < 0 || (int)written == total)) {
    // Arm phase 2: flash the saved file on the next boot, with Wi-Fi/TLS gone and
    // the heap clean. otaPend=true so setup() calls us again.
    if (_p.begin("marauder", false)) { _p.putInt("otaPhase", 2); _p.putBool("otaPend", true); _p.end(); }
    wuSay(80, "Downloaded. Flashing on reboot...", TFT_GREEN);
    delay(1600);
    ESP.restart();
  } else {
    SD.remove(OTA_FILE);
    wuSay(80, "Download incomplete - try again.", TFT_RED);
    delay(4000);
  }
}

#else   // no SD
void runFileServer() {}
void runWifiUpdate() {}
void requestWifiUpdate() {}
#endif
