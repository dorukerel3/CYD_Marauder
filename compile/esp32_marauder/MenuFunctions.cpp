#include "MenuFunctions.h"
#include "lang_var.h"
//#include "icons.h"
#include "configs.h"
#include "TouchDrvGT911.hpp"
#include <Preferences.h>
#include <SD.h>
#include <math.h>
#include <new>          // std::nothrow -- PNG decoder (re)allocation must never abort on OOM
// PNGdec bundles zlib, whose inflate_mode enum uses names Marauder also #defines.
// Undef the clashing macros so zlib's headers parse, then include PNGdec last.
#undef BAD
#undef GOOD
#undef WAITING
#undef HEAD
#undef NAME
#undef COMMENT
#undef TYPE
#undef DIST
#undef MATCH
#undef LEN
#undef DONE
#undef MEM
#undef SYNC
#undef CHECK
#undef LENGTH
#include <PNGdec.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_wdt.h"
#include "FileServerHTTP.h"

TouchDrvGT911 touch;



extern const unsigned char menu_icons[][66];
PROGMEM lv_obj_t * slider_label;
PROGMEM lv_obj_t * ta1;
PROGMEM lv_obj_t * ta2;
PROGMEM lv_obj_t * save_name;

// ---- UI language system (menu labels only) ----
// 0=English, 1=Japanese (romaji), 2=German, 3=French, 4=Italian, 5=Portuguese, 6=Turkish
uint8_t g_language = 0;
struct UiTr { const char* en; const char* v[6]; };
static const UiTr UI_TR[] = {
  {"Device",             {"Debaisu","Geraet","Appareil","Dispositivo","Dispositivo","Cihaz"}},
  {"Reboot",             {"Saikidou","Neustart","Redemarrer","Riavvia","Reiniciar","Yeniden Baslat"}},
  {"Settings",           {"Settei","Einstellungen","Reglages","Impostazioni","Ajustes","Ayarlar"}},
  {"Language",           {"Gengo","Sprache","Langue","Lingua","Idioma","Dil"}},
  {"Device Info",        {"Jouhou","Geraeteinfo","Infos","Info Disp","Info Disp","Cihaz Bilgisi"}},
  {"Update Firmware",    {"Koushin","Aktualisieren","Mise a jour","Aggiorna","Atualizar","Guncelle"}},
  {"Attacks",            {"Kougeki","Angriffe","Attaques","Attacchi","Ataques","Saldirilar"}},
  {"Sniffers",           {"Sunifa","Schnueffler","Ecouteurs","Sniffer","Farejadores","Dinleyiciler"}},
  {"General",            {"Ippan","Allgemein","General","Generale","Geral","Genel"}},
  {"WiFi Sniffers",      {"WiFi Sunifa","WLAN Schnueffler","Ecouteurs WiFi","Sniffer WiFi","Farejadores WiFi","WiFi Dinleyiciler"}},
  {"WiFi Attacks",       {"WiFi Kougeki","WLAN Angriffe","Attaques WiFi","Attacchi WiFi","Ataques WiFi","WiFi Saldirilar"}},
  {"WiFi General",       {"WiFi Ippan","WLAN Allgemein","WiFi General","WiFi Generale","WiFi Geral","WiFi Genel"}},
  {"Bluetooth Sniffers", {"BT Sunifa","BT Schnueffler","Ecouteurs BT","Sniffer BT","Farejadores BT","BT Dinleyiciler"}},
  {"Bluetooth General",  {"BT Ippan","BT Allgemein","BT General","BT Generale","BT Geral","BT Genel"}},
  {"Wardriving",         {"Wodoraibingu","Wardriving","Wardriving","Wardriving","Wardriving","Wardriving"}},
  {"Generate SSIDs",     {"SSID Sakusei","SSID Erzeugen","Generer SSID","Genera SSID","Gerar SSID","SSID Uret"}},
  {"Clear SSIDs",        {"SSID Shoukyo","SSID Loeschen","Effacer SSID","Cancella SSID","Limpar SSID","SSID Temizle"}},
  {"Clear APs",          {"AP Shoukyo","AP Loeschen","Effacer AP","Cancella AP","Limpar AP","AP Temizle"}},
  {"Clear Stations",     {"Sta Shoukyo","Sta Loeschen","Effacer Sta","Cancella Sta","Limpar Sta","Sta Temizle"}},
  {"Select Method",      {"Houhou","Methode","Methode","Metodo","Metodo","Yontem"}},
  {"SSID List",          {"SSID Ichiran","SSID Liste","Liste SSID","Lista SSID","Lista SSID","SSID Listesi"}},
  {"Add SSIDs",          {"SSID Tsuika","SSID Hinzufuegen","Ajouter SSID","Aggiungi SSID","Adicionar SSID","SSID Ekle"}},
  {"General Apps",       {"Ippan Apuri","Allgemeine Apps","Apps General","App Generali","Apps Gerais","Genel Uygulamalar"}},
  {"Confirm Update",     {"Koushin Kakunin","Update Bestaetigen","Confirmer MaJ","Conferma Agg","Confirmar Atu","Guncelleme Onayla"}},
  {"Web Update",         {"Web Koushin","Web Update","MaJ Web","Agg Web","Atu Web","Web Guncelle"}},
  {"SD Update",          {"SD Koushin","SD Update","MaJ SD","Agg SD","Atu SD","SD Guncelle"}},
  {"Draw",               {"Egaku","Zeichnen","Dessiner","Disegna","Desenhar","Ciz"}},
  {"Scan APs",           {"AP Sukyan","AP Scannen","Scanner AP","Scansiona AP","Escanear AP","AP Tara"}},
  {"Join WiFi",          {"WiFi Setsuzoku","WLAN Verbinden","Rejoindre WiFi","Unisci WiFi","Conectar WiFi","WiFi Katil"}},
  {"Select APs",         {"AP Sentaku","AP Auswaehlen","Choisir AP","Seleziona AP","Selecionar AP","AP Sec"}},
  {"Select Stations",    {"Station Sentaku","Stationen Waehlen","Choisir Stations","Seleziona Stazioni","Selecionar Estacoes","Istasyon Sec"}},
  {"Packet Monitor",     {"Paketto Kanshi","Paket Monitor","Moniteur Paquet","Monitor Pacchetti","Monitor Pacotes","Paket Izleme"}},
  {"Raw Capture",        {"Nama Kyapucha","Rohaufnahme","Capture Brute","Cattura Raw","Captura Raw","Ham Yakalama"}},
  {"Station Sniff",      {"Station Sunifu","Station Schnueffeln","Sniff Station","Sniff Stazione","Sniff Estacao","Istasyon Dinle"}},
  {"Detect Card Skimmers",{"Sukima Kenshutsu","Skimmer Erkennen","Detecter Skimmers","Rileva Skimmer","Detectar Skimmers","Skimmer Algila"}},
  {"Detect Pwnagotchi",  {"Pwnagotchi Kenshutsu","Pwnagotchi Erkennen","Detecter Pwnagotchi","Rileva Pwnagotchi","Detectar Pwnagotchi","Pwnagotchi Algila"}},
  {"Detect Espressif",   {"Espressif Kenshutsu","Espressif Erkennen","Detecter Espressif","Rileva Espressif","Detectar Espressif","Espressif Algila"}},
  {"Shutdown WiFi",      {"WiFi Shuuryou","WLAN Aus","Arreter WiFi","Spegni WiFi","Desligar WiFi","WiFi Kapat"}},
  {"Shutdown BLE",       {"BLE Shuuryou","BLE Aus","Arreter BLE","Spegni BLE","Desligar BLE","BLE Kapat"}},
  {"Bluetooth Sniff",    {"BT Sunifu","BT Schnueffeln","Sniff BT","Sniff BT","Sniff BT","BT Dinle"}},
  {"Bluetooth Analyzer", {"BT Kaiseki","BT Analyse","Analyseur BT","Analizzatore BT","Analisador BT","BT Analiz"}},
  {"Bluetooth Attacks",  {"BT Kougeki","BT Angriffe","Attaques BT","Attacchi BT","Ataques BT","BT Saldirilar"}},
  {"BT Wardrive Continuous",{"BT Wardrive Renzoku","BT Wardrive Dauernd","BT Wardrive Continu","BT Wardrive Continuo","BT Wardrive Continuo","BT Wardrive Surekli"}},
  {"Airtag Sniff",       {"Airtag Sunifu","Airtag Schnueffeln","Sniff Airtag","Sniff Airtag","Sniff Airtag","Airtag Dinle"}},
  {"Flipper Sniff",      {"Flipper Sunifu","Flipper Schnueffeln","Sniff Flipper","Sniff Flipper","Sniff Flipper","Flipper Dinle"}},
  {"Station Wardrive",   {"Station Wodoraibu","Station Wardrive","Wardrive Station","Wardrive Stazione","Wardrive Estacao","Istasyon Wardrive"}},
  {"WiFi",               {"WiFi","WLAN","WiFi","WiFi","WiFi","WiFi"}},
  {"Bluetooth",          {"Bluetuusu","Bluetooth","Bluetooth","Bluetooth","Bluetooth","Bluetooth"}},
  {"Maps",               {"Mappu","Karten","Cartes","Mappe","Mapas","Haritalar"}},
  {"SD Folders",         {"SD Foruda","SD Ordner","Dossiers SD","Cartelle SD","Pastas SD","SD Klasorleri"}},
  {"Color Theme",        {"Karaa Teema","Farbthema","Theme Couleur","Tema Colore","Tema de Cor","Renk Temasi"}},
  {"Save/Load Files",    {"Fairu Hozon/Yomi","Dateien Sp/Laden","Fichiers Sauv/Char","Salva/Carica File","Salvar/Carregar","Dosya Kaydet/Yukle"}},
  {"Bluetooth Sniffer",  {"BT Sunifa","BT Schnueffler","Ecouteur BT","Sniffer BT","Farejador BT","BT Dinleyici"}},
  {"Probe Request Sniff",{"Probe Sunifu","Probe Schnueffeln","Sniff Probe","Sniff Probe","Sniff Probe","Probe Dinle"}},
  {"Beacon Sniff",       {"Beacon Sunifu","Beacon Schnueffeln","Sniff Beacon","Sniff Beacon","Sniff Beacon","Beacon Dinle"}},
  {"Deauth Sniff",       {"Deauth Sunifu","Deauth Schnueffeln","Sniff Deauth","Sniff Deauth","Sniff Deauth","Deauth Dinle"}},
  {"EAPOL/PMKID Scan",   {"EAPOL Sukyan","EAPOL Scan","Scan EAPOL","Scansione EAPOL","Escanear EAPOL","EAPOL Tara"}},
  {"Beacon Spam List",   {"Beacon Supamu Risuto","Beacon Spam Liste","Spam Beacon Liste","Spam Beacon Lista","Spam Beacon Lista","Beacon Spam Liste"}},
  {"Beacon Spam Random", {"Beacon Supamu Randamu","Beacon Spam Zufall","Spam Beacon Aleatoire","Spam Beacon Casuale","Spam Beacon Aleatorio","Beacon Spam Rastgele"}},
  {"Rick Roll Beacon",   {"Rick Roll Beacon","Rick Roll Beacon","Rick Roll Beacon","Rick Roll Beacon","Rick Roll Beacon","Rick Roll Beacon"}},
  {"Probe Req Flood",    {"Probe Furaddo","Probe Flut","Flood Probe","Flood Probe","Flood Probe","Probe Taskini"}},
  {"Deauth Flood",       {"Deauth Furaddo","Deauth Flut","Flood Deauth","Flood Deauth","Flood Deauth","Deauth Taskini"}},
  {"AP Clone Spam",      {"AP Kuroon Supamu","AP Klon Spam","Spam Clone AP","Spam Clone AP","Spam Clone AP","AP Klon Spam"}},
  {"Deauth Targeted",    {"Deauth Taagetto","Deauth Gezielt","Deauth Cible","Deauth Mirato","Deauth Alvo","Deauth Hedefli"}},
  {"BLE Spam All",       {"BLE Supamu Zenbu","BLE Spam Alle","Spam BLE Tout","Spam BLE Tutti","Spam BLE Todos","BLE Spam Tumu"}},
  {"BT Wardrive",        {"BT Wodoraibu","BT Wardrive","BT Wardrive","BT Wardrive","BT Wardrive","BT Wardrive"}},
  {"Channel Analyzer",   {"Chaneru Kaiseki","Kanal Analyse","Analyseur Canal","Analizzatore Canale","Analisador Canal","Kanal Analiz"}},
  {"Clone AP MAC",       {"AP MAC Kuroon","AP MAC Klonen","Cloner MAC AP","Clona MAC AP","Clonar MAC AP","AP MAC Klonla"}},
  {"Clone STA MAC",      {"STA MAC Kuroon","STA MAC Klonen","Cloner MAC STA","Clona MAC STA","Clonar MAC STA","STA MAC Klonla"}},
  {"Delete SD Files",    {"SD Fairu Sakujo","SD Dateien Loeschen","Suppr Fichiers SD","Elimina File SD","Excluir Arq SD","SD Dosya Sil"}},
  {"Evil Portal",        {"Evil Portal","Evil Portal","Evil Portal","Evil Portal","Evil Portal","Evil Portal"}},
  {"Flipper BLE Spam",   {"Flipper BLE Supamu","Flipper BLE Spam","Spam BLE Flipper","Spam BLE Flipper","Spam BLE Flipper","Flipper BLE Spam"}},
  {"GPS Data",           {"GPS Deeta","GPS Daten","Donnees GPS","Dati GPS","Dados GPS","GPS Verisi"}},
  {"GPS Settings",       {"GPS Settei","GPS Einstell.","Reglages GPS","Impostazioni GPS","Config GPS","GPS Ayarlari"}},
  {"Fuel Gauge",         {"Nenryoukei","Ladeanzeige","Jauge Batterie","Indicatore Batt","Medidor Bateria","Yakit Gostergesi"}},
  {"UTC Offset",         {"UTC Ofuset","UTC-Versatz","Decalage UTC","Fuso UTC","Fuso UTC","UTC Farki"}},
  {"Generate AP MAC",    {"AP MAC Sakusei","AP MAC Erzeugen","Generer MAC AP","Genera MAC AP","Gerar MAC AP","AP MAC Uret"}},
  {"Generate STA MAC",   {"STA MAC Sakusei","STA MAC Erzeugen","Generer MAC STA","Genera MAC STA","Gerar MAC STA","STA MAC Uret"}},
  {"Google BLE Spam",    {"Google BLE Spam","Google BLE Spam","Google BLE Spam","Google BLE Spam","Google BLE Spam","Google BLE Spam"}},
  {"Load APs",           {"AP Yomikomi","AP Laden","Charger AP","Carica AP","Carregar AP","AP Yukle"}},
  {"Load Airtags",       {"Airtag Yomikomi","Airtags Laden","Charger Airtags","Carica Airtag","Carregar Airtags","Airtag Yukle"}},
  {"Load SSIDs",         {"SSID Yomikomi","SSID Laden","Charger SSID","Carica SSID","Carregar SSID","SSID Yukle"}},
  {"NMEA Stream",        {"NMEA Sutoriimu","NMEA Stream","Flux NMEA","Flusso NMEA","Fluxo NMEA","NMEA Akisi"}},
  {"Packet Count",       {"Paketto Kazu","Paketzahl","Compte Paquets","Conteggio Pacchetti","Contagem Pacotes","Paket Sayisi"}},
  {"Samsung BLE Spam",   {"Samsung BLE Spam","Samsung BLE Spam","Samsung BLE Spam","Samsung BLE Spam","Samsung BLE Spam","Samsung BLE Spam"}},
  {"Save APs",           {"AP Hozon","AP Speichern","Sauver AP","Salva AP","Salvar AP","AP Kaydet"}},
  {"Save Airtags",       {"Airtag Hozon","Airtags Speichern","Sauver Airtags","Salva Airtag","Salvar Airtags","Airtag Kaydet"}},
  {"Save SSIDs",         {"SSID Hozon","SSID Speichern","Sauver SSID","Salva SSID","Salvar SSID","SSID Kaydet"}},
  {"Scan All",           {"Subete Sukyan","Alle Scannen","Tout Scanner","Scansiona Tutto","Escanear Tudo","Hepsini Tara"}},
  {"Select EP HTML File",{"EP HTML Sentaku","EP HTML Waehlen","Choisir HTML EP","Seleziona HTML EP","Selecionar HTML EP","EP HTML Sec"}},
  {"Set MACs",           {"MAC Settei","MACs Setzen","Definir MAC","Imposta MAC","Definir MAC","MAC Ayarla"}},
  {"Signal Monitor",     {"Shingou Kanshi","Signal Monitor","Moniteur Signal","Monitor Segnale","Monitor de Sinal","Sinyal Izleme"}},
  {"Sour Apple",         {"Sour Apple","Sour Apple","Sour Apple","Sour Apple","Sour Apple","Sour Apple"}},
  {"Spoof Airtag",       {"Airtag Narideri","Airtag Faelschen","Usurper Airtag","Falsifica Airtag","Falsificar Airtag","Airtag Taklit"}},
  {"Swiftpair Spam",     {"Swiftpair Spam","Swiftpair Spam","Swiftpair Spam","Swiftpair Spam","Swiftpair Spam","Swiftpair Spam"}},
  {"View AP Info",       {"AP Jouhou","AP Info","Voir Info AP","Vedi Info AP","Ver Info AP","AP Bilgisi"}},
  {"Wardrive",           {"Wodoraibu","Wardrive","Wardrive","Wardrive","Wardrive","Wardrive"}},
  {"Back",               {"Modoru","Zurueck","Retour","Indietro","Voltar","Geri"}},
  {"Yes",                {"Hai","Ja","Oui","Si","Sim","Evet"}},
};
static String tr(const String& s) {
  if (g_language == 0) return s;
  String key = s; key.trim();
  for (uint16_t i = 0; i < sizeof(UI_TR) / sizeof(UI_TR[0]); i++)
    if (key == UI_TR[i].en) return String(UI_TR[i].v[g_language - 1]);
  return s;
}
// UI colour theme (also used by drawLangCheck below); persisted via save/loadTheme()
static uint16_t g_lineColor = TFT_DARKGREY;   // button border line colour
static uint16_t g_textColor = 0;              // 0 = use each item's own colour
static uint16_t g_bgColor   = TFT_BLACK;      // menu background colour
// Language menu entries, in g_language index order (0=English .. 6=Turkish)
static const char *LANG_NAMES[7] = {"English", "Japanese", "German", "French", "Italian", "Portuguese", "Turkish"};
static bool isActiveLang(const String& name) {
  String k = name; k.trim();
  return (g_language < 7) && (k == String(LANG_NAMES[g_language]));
}
// check mark on the right of a menu row (centre y) for the active language,
// drawn in the current theme colour (text colour, or the line colour if text is Auto)
static void drawLangCheck(int centerY) {
  uint16_t col = g_textColor ? g_textColor : g_lineColor;
  int x = TFT_WIDTH - 26, y = centerY;
  display_obj.tft.drawLine(x, y, x + 6, y + 7, col);
  display_obj.tft.drawLine(x + 6, y + 7, x + 16, y - 8, col);
  display_obj.tft.drawLine(x, y + 1, x + 6, y + 8, col);
  display_obj.tft.drawLine(x + 6, y + 8, x + 16, y - 7, col);
}
// 2-letter language code shown in the icon slot (left) of each language menu row
static String langCode(const String& name) {
  String k = name; k.trim();
  if (k == "English") return "EN";
  if (k == "Japanese") return "JP";
  if (k == "German") return "DE";
  if (k == "French") return "FR";
  if (k == "Italian") return "IT";
  if (k == "Portuguese") return "PT";
  if (k == "Turkish") return "TR";
  return "";
}
static void drawLangCode(const String& name, int centerY) {
  String c = langCode(name);
  if (c == "") return;
  uint16_t col = g_textColor ? g_textColor : TFT_WHITE;
  // Match the menu's label typeface (MENU_FONT) so the 2-letter code is the
  // same font/size as the language name next to it, not the smaller GLCD font.
  display_obj.tft.setFreeFont(MENU_FONT);
  display_obj.tft.setTextDatum(MC_DATUM);
  display_obj.tft.setTextColor(col, TFT_BLACK);
  display_obj.tft.drawString(c, 11, centerY);
  display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setFreeFont(NULL);
}
// Color Theme menu icon: outer ring = line colour, inner fill = text colour (live preview)
static void drawThemeSwatch(int centerY) {
  uint16_t fill = g_textColor ? g_textColor : TFT_WHITE;   // text colour (white when "Auto")
  uint16_t ring = g_lineColor;                              // border/line colour
  int cx = 11, cy = centerY;
  display_obj.tft.fillCircle(cx, cy, 8, fill);
  display_obj.tft.drawCircle(cx, cy, 9, ring);
  display_obj.tft.drawCircle(cx, cy, 10, ring);             // 2px outer ring
}
static void saveLanguage(uint8_t n) {
  g_language = n;
  Preferences _p;
  if (_p.begin("marauder", false)) { _p.putUChar("lang", n); _p.end(); }
}
static void loadLanguage() {
  Preferences _p;
  if (_p.begin("marauder", true)) { g_language = _p.getUChar("lang", 0); _p.end(); }
}

// ---- UI colour theme persistence (g_lineColor/g_textColor declared above) ----
static void saveTheme() {
  Preferences _p;
  if (_p.begin("marauder", false)) {
    _p.putUShort("line", g_lineColor);
    _p.putUShort("text", g_textColor);
    _p.putUShort("bg", g_bgColor);
    _p.end();
  }
  display_obj.bg_color = g_bgColor;   // apply immediately to all menu clears
}
static void loadTheme() {
  Preferences _p;
  if (_p.begin("marauder", true)) {
    g_lineColor = _p.getUShort("line", TFT_DARKGREY);
    g_textColor = _p.getUShort("text", 0);
    g_bgColor   = _p.getUShort("bg", TFT_BLACK);
    _p.end();
  }
  display_obj.bg_color = g_bgColor;
}

// ---- GPS wiring config (pins + baud), persisted in NVS; read by
//      GpsInterface::begin() at boot and by the "GPS Settings" screen ----
#ifndef GPS_TX
  #define GPS_TX 35
#endif
#ifndef GPS_RX
  #define GPS_RX 17
#endif
// Candidate GPIOs. RX (ESP input) may use input-only pins (34-39);
// TX (ESP output) must be an output-capable pin.
static const int GPS_RX_CANDS[] = {35, 34, 39, 36, 22, 27, 21, 16, 17, 4, 13};
static const int GPS_TX_CANDS[] = {17, 22, 27, 21, 16, 4, 13, 1};
static const uint32_t GPS_BAUD_CANDS[] = {4800, 9600, 19200, 38400, 57600, 115200};
static const uint32_t GPS_HZ_CANDS[] = {1, 2, 5, 10};   // position/NMEA output rate
static int g_gpsEspRx = GPS_TX;   // ESP pin receiving GPS module TX (Serial2 rxPin)
static int g_gpsEspTx = 21;       // ESP pin sending to GPS module RX (Serial2 txPin); default IO21
static uint32_t g_gpsBaud = 9600;
static uint32_t g_gpsHz = 1;      // GPS update rate in Hz

static void loadGpsCfg() {
  Preferences _p;
  if (_p.begin("marauder", true)) {
    g_gpsEspRx = _p.getInt("gRx", GPS_TX);
    g_gpsEspTx = _p.getInt("gTx", 21);
    g_gpsBaud  = _p.getUInt("gBaud", 9600);
    g_gpsHz    = _p.getUInt("gHz", 1);
    _p.end();
  }
}
static void saveGpsCfg() {
  Preferences _p;
  if (_p.begin("marauder", false)) {
    _p.putInt("gRx", g_gpsEspRx);
    _p.putInt("gTx", g_gpsEspTx);
    _p.putUInt("gBaud", g_gpsBaud);
    _p.putUInt("gHz", g_gpsHz);
    _p.end();
  }
}

// Local-time UTC offset in minutes (Device > UTC Offset). Default +180 = Turkey.
static int g_utcOffsetMin = 180;
static void loadUtcCfg() {
  Preferences _p;
  if (_p.begin("marauder", true)) { g_utcOffsetMin = _p.getInt("utc", 180); _p.end(); }
}
static void saveUtcCfg() {
  Preferences _p;
  if (_p.begin("marauder", false)) { _p.putInt("utc", g_utcOffsetMin); _p.end(); }
}
// Format a minutes offset as "UTC+3:00" / "UTC-5:30" / "UTC+0:00".
static String fmtUtc(int off) {
  char s = (off < 0) ? '-' : '+';
  int a = (off < 0) ? -off : off;
  char buf[12];
  snprintf(buf, sizeof(buf), "UTC%c%d:%02d", s, a / 60, a % 60);
  return String(buf);
}
// Step current value to the next/previous entry in a candidate list (wrapping).
static int gpsStep(const int *arr, int n, int cur, int dir) {
  int idx = 0;
  for (int i = 0; i < n; i++) if (arr[i] == cur) { idx = i; break; }
  idx = (idx + dir + n) % n;
  return arr[idx];
}
static uint32_t gpsStepU(const uint32_t *arr, int n, uint32_t cur, int dir) {
  int idx = 0;
  for (int i = 0; i < n; i++) if (arr[i] == cur) { idx = i; break; }
  idx = (idx + dir + n) % n;
  return arr[idx];
}

// Draw a menu button label (translated at draw time)
static void drawMenuLabel(uint8_t slot, uint16_t topY, uint16_t color, const String& enName, bool pressed) {
  display_obj.tft.setFreeFont(MENU_FONT);           // every row: keep the label in the menu font
  display_obj.key[slot].drawButton(pressed, tr(enName));   // (drawLangCode below resets the font)
  drawLangCode(enName, topY);                       // EN/JP/DE... in the icon slot
  if (isActiveLang(enName)) drawLangCheck(topY);    // tick the active language
}

// Sort a menu's items alphabetically (case-insensitive) by name.
static void sortMenuAZ(Menu *menu) {
  if (!menu || !menu->list) return;
  int n = menu->list->size();
  for (int i = 0; i < n - 1; i++) {
    int m = i;
    String mi = menu->list->get(i).name; mi.toLowerCase();
    for (int j = i + 1; j < n; j++) {
      String nj = menu->list->get(j).name; nj.toLowerCase();
      if (nj < mi) { m = j; mi = nj; }
    }
    if (m != i) {
      MenuNode a = menu->list->get(i);
      MenuNode b = menu->list->get(m);
      menu->list->set(i, b);
      menu->list->set(m, a);
    }
  }
}

// ---- Offline map viewer: OSM raster tiles (OsmAnd tile storage) on the SD card ----
// The PNG decoder (~40KB inflate window) is RESIDENT: created once when the map opens (when
// RAM is clean so it's guaranteed to allocate) and freed when the map closes. It is NEVER
// freed/recreated around routing -- that juggling fragmented the heap and caused the blank
// map / low-memory / panic churn. The search just uses whatever heap is left beside it.
static PNG *g_png = nullptr;
static int g_tileScreenX = 0, g_tileScreenY = 0;
static int mapPngDraw(PNGDRAW *pDraw) {
  uint16_t line[256];
  g_png->getLineAsRGB565(pDraw, line, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
  int y = g_tileScreenY + pDraw->y;
  if (y < 0 || y >= TFT_HEIGHT) return 1;
  int srcStart = 0, drawX = g_tileScreenX, w = pDraw->iWidth;
  if (drawX < 0) { srcStart = -drawX; drawX = 0; }
  int drawW = w - srcStart;
  if (drawX + drawW > TFT_WIDTH) drawW = TFT_WIDTH - drawX;
  if (drawW > 0) display_obj.tft.pushImage(drawX, y, drawW, 1, line + srcStart);
  return 1;
}
// --- PNG tile decode, STREAMED from SD -------------------------------------------
// We used to malloc the whole compressed tile into RAM and cap it at 60 KB. On this
// ~50 KB-free board the big, detailed CITY tiles (the ones that matter most) failed
// to allocate or exceeded the cap and rendered blank. Decoding straight from the SD
// file via PNGdec's read/seek callbacks removes both limits: only the decoder's own
// resident buffers are needed, never a copy of the whole file.
// The File lives on the heap only for the duration of one tile decode (its pointer
// travels through PNGFILE.fHandle), so there's no persistent static SD handle eating
// the board's nearly-full static DRAM.
static void *mapPngOpen(const char *fn, int32_t *size) {
  File *f = new (std::nothrow) File(SD.open(fn, FILE_READ));
  if (!f) return NULL;
  if (!*f) { delete f; return NULL; }
  *size = (int32_t)f->size();
  return (void *)f;
}
static void mapPngClose(void *h) { File *f = (File *)h; if (f) { if (*f) f->close(); delete f; } }
static int32_t mapPngRead(PNGFILE *pf, uint8_t *buf, int32_t len) {
  File *f = (File *)pf->fHandle;
  return (f && *f) ? (int32_t)f->read(buf, len) : 0;
}
static int32_t mapPngSeek(PNGFILE *pf, int32_t pos) {
  File *f = (File *)pf->fHandle;
  return (f && *f && f->seek(pos)) ? pos : 0;
}
static void mapDrawTile(uint32_t z, uint32_t x, uint32_t y, int sx, int sy) {
  char path[48];
  snprintf(path, sizeof(path), "/maps/%lu/%lu/%lupng.tile", (unsigned long)z, (unsigned long)x, (unsigned long)y);
  if (!SD.exists(path)) return;                   // missing tile -> leave blank
  g_tileScreenX = sx; g_tileScreenY = sy;
  if (!g_png) g_png = new (std::nothrow) PNG();   // recover if a route's reclaim left it freed
  if (g_png && g_png->open(path, mapPngOpen, mapPngClose, mapPngRead, mapPngSeek, mapPngDraw) == PNG_SUCCESS) {
    g_png->decode(NULL, 0);
    g_png->close();
  }
}
// Find the first available tile on the SD (so the map opens somewhere with data, no GPS needed)
static String baseName(const String &s) {
  int sl = s.lastIndexOf('/');
  return sl >= 0 ? s.substring(sl + 1) : s;
}
// Find the first tile (x/y) inside a given zoom folder path. Returns false if the folder has none.
static bool firstTileInZoom(const String &zpath, uint32_t &ox, uint32_t &oy) {
  File zdir = SD.open(zpath.c_str());
  if (!zdir) return false;
  for (File xe = zdir.openNextFile(); xe; xe = zdir.openNextFile()) {
    if (xe.isDirectory()) {
      String xn = baseName(xe.name());
      uint32_t xv = strtoul(xn.c_str(), NULL, 10);
      File xdir = SD.open((zpath + "/" + xn).c_str());
      if (xdir) {
        for (File ye = xdir.openNextFile(); ye; ye = xdir.openNextFile()) {
          bool isd = ye.isDirectory();
          uint32_t yv = strtoul(baseName(ye.name()).c_str(), NULL, 10);
          ye.close();
          if (!isd && xv > 0 && yv > 0) {
            ox = xv; oy = yv;
            xdir.close(); zdir.close();
            return true;
          }
        }
        xdir.close();
      }
    }
    xe.close();
  }
  zdir.close();
  return false;
}
// Scan /maps for numeric zoom folders and return the LOWEST zoom that actually holds a tile
// (so the map opens at the most zoomed-out level available), plus its first tile.
static bool findMapBase(uint32_t &oz, uint32_t &ox, uint32_t &oy) {
  File root = SD.open("/maps");
  if (!root || !root.isDirectory()) { if (root) root.close(); return false; }
  bool found = false;
  for (File ze = root.openNextFile(); ze; ze = root.openNextFile()) {
    if (ze.isDirectory()) {
      String zn = baseName(ze.name());
      uint32_t zv = strtoul(zn.c_str(), NULL, 10);
      if (zv >= 1 && zv <= 20 && zn == String(zv) && (!found || zv < oz)) {  // a lower zoom folder
        uint32_t tx, ty;
        if (firstTileInZoom("/maps/" + zn, tx, ty)) {
          oz = zv; ox = tx; oy = ty; found = true;
        }
      }
    }
    ze.close();
  }
  root.close();
  return found;
}

// Does a specific tile exist on the SD?
static bool tileExists(uint32_t z, uint32_t x, uint32_t y) {
  char path[48];
  snprintf(path, sizeof(path), "/maps/%lu/%lu/%lupng.tile",
           (unsigned long)z, (unsigned long)x, (unsigned long)y);
  return SD.exists(path);
}
// Next existing zoom folder in /maps in a direction (+1 = deeper, -1 = shallower).
// Skips missing levels; returns `cur` if there's none further that way.
static uint32_t adjacentZoom(uint32_t cur, int dir) {
  File root = SD.open("/maps");
  if (!root || !root.isDirectory()) { if (root) root.close(); return cur; }
  uint32_t best = cur; bool found = false;
  for (File e = root.openNextFile(); e; e = root.openNextFile()) {
    if (e.isDirectory()) {
      String nm = baseName(e.name());
      uint32_t zv = strtoul(nm.c_str(), NULL, 10);
      if (zv >= 1 && zv <= 20 && nm == String(zv)) {
        if (dir > 0 && zv > cur && (!found || zv < best)) { best = zv; found = true; }
        if (dir < 0 && zv < cur && (!found || zv > best)) { best = zv; found = true; }
      }
    }
    e.close();
  }
  root.close();
  return found ? best : cur;
}
// Find a tile at zoom z that has neighbours on all 4 sides, so panning works in
// every direction from the start (falls back to any tile if none is fully interior).
static bool interiorTileAtZoom(uint32_t z, uint32_t &ox, uint32_t &oy) {
  char zp[24];
  snprintf(zp, sizeof(zp), "/maps/%lu", (unsigned long)z);
  File zdir = SD.open(zp);
  if (!zdir || !zdir.isDirectory()) { if (zdir) zdir.close(); return false; }
  uint32_t fx = 0, fy = 0; bool anyTile = false;
  int budget = 90;   // cap SD work so a huge zoom tree can't stall the UI
  for (File xe = zdir.openNextFile(); xe && budget > 0; xe = zdir.openNextFile()) {
    if (xe.isDirectory()) {
      String xn = baseName(xe.name());
      uint32_t xv = strtoul(xn.c_str(), NULL, 10);
      File xdir = SD.open((String(zp) + "/" + xn).c_str());
      if (xdir) {
        for (File ye = xdir.openNextFile(); ye && budget > 0; ye = xdir.openNextFile()) {
          bool isd = ye.isDirectory();
          uint32_t yv = strtoul(baseName(ye.name()).c_str(), NULL, 10);
          ye.close();
          if (!isd && xv > 0 && yv > 0) {
            if (!anyTile) { fx = xv; fy = yv; anyTile = true; }
            budget--;   // each candidate costs up to 4 neighbour lookups
            if (tileExists(z, xv, yv - 1) && tileExists(z, xv, yv + 1) &&
                tileExists(z, xv - 1, yv) && tileExists(z, xv + 1, yv)) {
              ox = xv; oy = yv; xdir.close(); zdir.close(); return true;
            }
          }
        }
        xdir.close();
      }
    }
    xe.close();
  }
  zdir.close();
  if (anyTile) { ox = fx; oy = fy; return true; }   // fallback: first tile seen
  return false;
}
// Pick a tile near the MIDDLE of a zoom level's coverage (cheap: directory listings
// only, no per-tile existence checks) so panning has tiles in every direction.
static bool centralTileAtZoom(uint32_t z, uint32_t &ox, uint32_t &oy) {
  char zp[24];
  snprintf(zp, sizeof(zp), "/maps/%lu", (unsigned long)z);
  // pass 1: count numeric x-folders
  File d = SD.open(zp);
  if (!d || !d.isDirectory()) { if (d) d.close(); return false; }
  int nx = 0;
  for (File e = d.openNextFile(); e; e = d.openNextFile()) {
    if (e.isDirectory() && strtoul(baseName(e.name()).c_str(), NULL, 10) > 0) nx++;
    e.close();
  }
  d.close();
  if (nx == 0) return false;
  // pass 2: grab the middle x-folder name
  d = SD.open(zp);
  int target = nx / 2, idx = 0; String midx = "";
  for (File e = d.openNextFile(); e; e = d.openNextFile()) {
    if (e.isDirectory() && strtoul(baseName(e.name()).c_str(), NULL, 10) > 0) {
      if (idx == target) { midx = baseName(e.name()); e.close(); break; }
      idx++;
    }
    e.close();
  }
  d.close();
  if (midx == "") return false;
  uint32_t xv = strtoul(midx.c_str(), NULL, 10);
  String xpath = String(zp) + "/" + midx;
  // count y-files in that folder, then pick the middle one
  File xd = SD.open(xpath.c_str());
  if (!xd) return false;
  int ny = 0;
  for (File e = xd.openNextFile(); e; e = xd.openNextFile()) { if (!e.isDirectory()) ny++; e.close(); }
  xd.close();
  if (ny == 0) return false;
  xd = SD.open(xpath.c_str());
  int yt = ny / 2, yi = 0; uint32_t yv = 0;
  for (File e = xd.openNextFile(); e; e = xd.openNextFile()) {
    if (!e.isDirectory()) {
      if (yi == yt) { yv = strtoul(baseName(e.name()).c_str(), NULL, 10); e.close(); break; }
      yi++;
    }
    e.close();
  }
  xd.close();
  if (yv == 0) return false;
  ox = xv; oy = yv;
  return true;
}

// project an e7 lat/lon to screen using the current view (Z, centre tile cxf/cyf)
static void projE7(int32_t late7, int32_t lone7, uint32_t Z, double cxf, double cyf,
                   int cx, int cy, int &sx, int &sy) {
  double n = (double)(1UL << Z);
  double lat = late7 / 1e7, lon = lone7 / 1e7;
  double txf = (lon + 180.0) / 360.0 * n;
  double tyf = (1.0 - asinh(tan(lat * M_PI / 180.0)) / M_PI) / 2.0 * n;
  sx = cx + (int)lround((txf - cxf) * 256.0);
  sy = cy + (int)lround((tyf - cyf) * 256.0);
}
// Draw the live GPS position marker (cyan dot + white ring) for the current view.
static void drawGpsMarker(uint32_t Z, double cxf, double cyf, int cx, int cy) {
  if (!gps_obj.getFixStatus()) return;
  double la = gps_obj.getLat().toDouble(), lo = gps_obj.getLon().toDouble();
  if (la == 0.0 && lo == 0.0) return;
  int mx, my;
  projE7((int32_t)lround(la * 1e7), (int32_t)lround(lo * 1e7), Z, cxf, cyf, cx, cy, mx, my);
  display_obj.tft.fillCircle(mx, my, 4, TFT_CYAN);
  display_obj.tft.drawCircle(mx, my, 6, TFT_WHITE);
  display_obj.tft.drawCircle(mx, my, 4, TFT_BLACK);
}

// ---- Map UI layout (portrait 240x320). Big rectangular hit zones, no thin shapes. ----
static bool g_showCoords = false;   // live GPS coordinate overlay toggle
static bool g_followGps = false;    // when true + GPS fix: keep the map centred on us

// Saved places ("favourites") kept in a tiny text file on the SD: one line "latE7 lonE7".
static void favSave(int32_t latE, int32_t lonE) {
  if (!SD.exists("/maps")) SD.mkdir("/maps");
  File f = SD.open("/maps/favs.txt", FILE_APPEND);
  if (f) { f.print(latE); f.print(' '); f.println(lonE); f.close(); }
}
static int favLoad(int32_t *lat, int32_t *lon, int maxN) {   // returns how many were read
  File f = SD.open("/maps/favs.txt", FILE_READ);
  if (!f) return 0;
  int n = 0;
  while (f.available() && n < maxN) {
    String line = f.readStringUntil('\n'); line.trim();
    int sp = line.indexOf(' ');
    if (sp > 0) { lat[n] = (int32_t)line.substring(0, sp).toInt();
                  lon[n] = (int32_t)line.substring(sp + 1).toInt(); n++; }
  }
  f.close();
  return n;
}
// Full-screen list of saved places. Returns true (and the chosen lat/lon) on a tap, or
// false if the user backs out. Also lets the user delete all with a "Clear" button.
static bool showFavList(int32_t &outLat, int32_t &outLon) {
  const int MAXF = 9;
  int32_t lat[MAXF], lon[MAXF];
  int n = favLoad(lat, lon, MAXF);
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextDatum(MC_DATUM);
  display_obj.tft.fillRoundRect(4, 4, 40, 28, 3, TFT_RED); display_obj.tft.drawRoundRect(4, 4, 40, 28, 3, TFT_WHITE);
  display_obj.tft.setTextColor(TFT_WHITE, TFT_RED); display_obj.tft.drawString("X", 24, 18, 2);
  display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  display_obj.tft.drawString(n ? "Saved places (tap to go):" : "No saved places yet", 52, 12, 1);
  for (int i = 0; i < n; i++) {
    int y = 40 + i * 28;
    display_obj.tft.drawRect(2, y, 236, 26, TFT_DARKGREY);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    String row = String(i + 1) + ":  " + String(lat[i] / 1e7, 4) + ",  " + String(lon[i] / 1e7, 4);
    display_obj.tft.drawString(row, 8, y + 9, 1);
  }
  while (true) {
    uint16_t tx, ty;
    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
      if (ty >= 2 && ty <= 36 && tx <= 50) return false;           // X = back
      for (int i = 0; i < n; i++) {
        int y = 40 + i * 28;
        if (ty >= y && ty <= y + 26) { outLat = lat[i]; outLon = lon[i]; return true; }
      }
    }
    delay(30);
  }
}

// Live GPS coordinate panel (top strip). Reads the GPS each call so it updates live.
static void drawCoordsPanel() {
  display_obj.tft.fillRect(0, 50, TFT_WIDTH, 26, TFT_BLACK);
  display_obj.tft.drawRect(0, 50, TFT_WIDTH, 26, TFT_DARKCYAN);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setTextDatum(TL_DATUM);
  if (gps_obj.getFixStatus()) {
    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawString("Lat " + gps_obj.getLat(), 4, 54, 1);
    display_obj.tft.drawString("Lon " + gps_obj.getLon(), 4, 64, 1);
  } else {
    display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    display_obj.tft.drawString("GPS: no fix", 4, 54, 1);
    display_obj.tft.drawString("(not connected / no signal)", 4, 64, 1);
  }
}

// Button rectangles {x, y, w, h}; hit test uses the same rects (grown slightly).
static void mapBtn(int x, int y, int w, int h, uint16_t col, const char *lab, uint8_t font) {
  display_obj.tft.fillRoundRect(x, y, w, h, 3, col);
  display_obj.tft.drawRoundRect(x, y, w, h, 3, TFT_WHITE);
  display_obj.tft.setTextColor(TFT_WHITE, col);
  display_obj.tft.setTextDatum(MC_DATUM);
  display_obj.tft.drawString(lab, x + w / 2, y + h / 2, font);
}
static void drawMapControls() {
  display_obj.tft.setFreeFont(NULL);
  // button row at y18, directly under the info bar (zoom/status) that occupies y0-16:
  // exit / zoom out / zoom in on the left, GPS coords toggle on the right.
  mapBtn(4, 18, 30, 28, TFT_RED, "X", 2);
  mapBtn(48, 18, 30, 28, TFT_DARKGREY, "-", 2);
  mapBtn(90, 18, 30, 28, TFT_DARKGREY, "+", 2);
  mapBtn(124, 18, 38, 28, TFT_DARKGREEN, "Sav", 1);   // save current centre as a place
  mapBtn(164, 18, 38, 28, TFT_PURPLE, "Fav", 1);      // open saved-places list
  mapBtn(206, 18, 30, 28, (g_showCoords || g_followGps) ? TFT_DARKCYAN : TFT_NAVY, "GPS", 1);
  // D-pad: four big rectangles with triangle glyphs (up/down stacked, left/right sides)
  uint16_t dc = TFT_NAVY;
  display_obj.tft.fillRoundRect(92, 196, 56, 28, 3, dc); display_obj.tft.drawRoundRect(92, 196, 56, 28, 3, TFT_WHITE);
  display_obj.tft.fillTriangle(120, 201, 110, 217, 130, 217, TFT_WHITE);          // up
  display_obj.tft.fillRoundRect(92, 254, 56, 28, 3, dc); display_obj.tft.drawRoundRect(92, 254, 56, 28, 3, TFT_WHITE);
  display_obj.tft.fillTriangle(110, 261, 130, 261, 120, 277, TFT_WHITE);          // down
  display_obj.tft.fillRoundRect(32, 225, 56, 28, 3, dc); display_obj.tft.drawRoundRect(32, 225, 56, 28, 3, TFT_WHITE);
  display_obj.tft.fillTriangle(46, 239, 62, 231, 62, 247, TFT_WHITE);            // left
  display_obj.tft.fillRoundRect(150, 225, 56, 28, 3, dc); display_obj.tft.drawRoundRect(150, 225, 56, 28, 3, TFT_WHITE);
  display_obj.tft.fillTriangle(192, 239, 176, 231, 176, 247, TFT_WHITE);         // right
  display_obj.tft.setTextDatum(TL_DATUM);
}

static int mapHitTest(int x, int y) {
  if (y >= 16 && y <= 48) {
    if (x >= 2   && x <= 36)  return 1;    // exit
    if (x >= 46  && x <= 80)  return 3;    // zoom out (-)
    if (x >= 88  && x <= 122) return 2;    // zoom in  (+)
    if (x >= 122 && x <= 162) return 13;   // Save place
    if (x >= 164 && x <= 204) return 14;   // Favourites list
    if (x >= 204 && x <= 238) return 12;   // GPS coords/follow toggle (top-right)
  }
  if (x >= 90  && x <= 150 && y >= 194 && y <= 226) return 4;  // up
  if (x >= 90  && x <= 150 && y >= 252 && y <= 284) return 5;  // down
  if (x >= 30  && x <= 90  && y >= 223 && y <= 255) return 6;  // left
  if (x >= 150 && x <= 208 && y >= 223 && y <= 255) return 7;  // right
  return 0;
}

static void showMap() {
  uint32_t Z = 15;
  double cxf = 0, cyf = 0;
  bool have = false;
  g_png = new (std::nothrow) PNG();   // resident decoder for the whole map session (allocated while RAM is clean)
  display_obj.tft.setSwapBytes(true);
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  display_obj.tft.drawCentreString("Loading map...", TFT_WIDTH / 2, TFT_HEIGHT / 2, 2);
  // Start at the lowest zoom available, on the first tile found (fast).
  uint32_t fz, fx, fy;
  if (findMapBase(fz, fx, fy)) { Z = fz; cxf = fx + 0.5; cyf = fy + 0.5; have = true; }
  // If we have a GPS fix, re-center on our position at that same zoom level.
  if (gps_obj.getFixStatus()) {
    double lat = gps_obj.getLat().toDouble(), lon = gps_obj.getLon().toDouble();
    if (lat != 0.0 || lon != 0.0) {
      uint32_t n = 1UL << Z;
      cxf = (lon + 180.0) / 360.0 * n;
      cyf = (1.0 - asinh(tan(lat * M_PI / 180.0)) / M_PI) / 2.0 * n;
      have = true;
    }
  }
  bool redraw = true;
  String status = "";
  uint32_t lastCoord = 0, lastFollow = 0;
  while (true) {
    if (redraw) {
      int cx = TFT_WIDTH / 2, cy = TFT_HEIGHT / 2;
      display_obj.tft.fillScreen(TFT_BLACK);
      if (have) {
        uint32_t xt = (uint32_t)cxf, yt = (uint32_t)cyf;
        int px = (int)((cxf - xt) * 256.0), py = (int)((cyf - yt) * 256.0);
        for (int dy = -1; dy <= 1; dy++)
          for (int dx = -1; dx <= 1; dx++) {
            int sx = cx - px + dx * 256, sy = cy - py + dy * 256;
            if (sx < TFT_WIDTH && sy < TFT_HEIGHT && sx + 256 > 0 && sy + 256 > 0)
              mapDrawTile(Z, xt + dx, yt + dy, sx, sy);
          }
        drawGpsMarker(Z, cxf, cyf, cx, cy);
        // centre crosshair (marks the map centre / where a saved place drops)
        display_obj.tft.drawFastHLine(cx - 8, cy, 16, TFT_WHITE);
        display_obj.tft.drawFastVLine(cx, cy - 8, 16, TFT_WHITE);
        display_obj.tft.fillCircle(cx, cy, 2, TFT_RED);
      } else {
        display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
        display_obj.tft.drawCentreString("No tiles found on SD (/maps)", TFT_WIDTH / 2, TFT_HEIGHT / 2, 2);
      }
      // Info bar across the very top (y0-16): zoom level + route status. Drawn BEFORE
      // the controls; the button row sits directly under it at y18. setFreeFont(NULL)
      // first: on the first frame the large menu font is still active, and font "1"
      // becomes that free font unless we reset to the built-in GLCD font here.
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.fillRect(0, 0, TFT_WIDTH, 16, TFT_BLACK);
      display_obj.tft.setTextDatum(TL_DATUM);
      display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      display_obj.tft.drawString((String) "z" + Z, 4, 4, 1);
      if (status.length()) {
        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
        display_obj.tft.drawString(status, 34, 4, 1);
      }
      drawMapControls();
      if (g_showCoords) { drawCoordsPanel(); lastCoord = millis(); }
      redraw = false;
    }
    uint16_t tx, ty;
    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      int a = mapHitTest(tx, ty);
      while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
      double step = 0.5;
      // current map-centre as e7 lat/lon (where pins drop)
      double nn = (double)(1UL << Z);
      double clon = cxf / nn * 360.0 - 180.0;
      double clat = atan(sinh(M_PI * (1 - 2 * cyf / nn))) * 180.0 / M_PI;
      int32_t cLatE = (int32_t)lround(clat * 1e7), cLonE = (int32_t)lround(clon * 1e7);
      // without a GPS fix, if the new zoom's centre tile is blank, jump to the
      // first available tile at that zoom so you always land on real data.
      bool noGps = !gps_obj.getFixStatus();
      if (a == 1) break;
      else if (a == 2 || a == 3) {           // zoom: jump only between existing zoom folders
        uint32_t nz = adjacentZoom(Z, a == 2 ? +1 : -1);
        if (nz != Z) {
          double f = pow(2.0, (double)nz - (double)Z);   // rescale for the zoom-level gap
          cxf *= f; cyf *= f; Z = nz;
          if (noGps && !tileExists(Z, (uint32_t)cxf, (uint32_t)cyf)) {
            uint32_t nx, ny; if (firstTileInZoom("/maps/" + String(Z), nx, ny)) { cxf = nx + 0.5; cyf = ny + 0.5; } }
          redraw = true;
        }
      }
      else if (a == 4) { cyf -= step; g_followGps = false; redraw = true; }   // manual pan
      else if (a == 5) { cyf += step; g_followGps = false; redraw = true; }   // stops GPS-follow
      else if (a == 6) { cxf -= step; g_followGps = false; redraw = true; }
      else if (a == 7) { cxf += step; g_followGps = false; redraw = true; }
      else if (a == 12) {
        // GPS button: show coords + (if we have a fix) centre on us and start following.
        g_showCoords = !g_showCoords;
        g_followGps = false;
        if (g_showCoords && gps_obj.getFixStatus()) {
          double la = gps_obj.getLat().toDouble(), lo = gps_obj.getLon().toDouble();
          if (la != 0.0 || lo != 0.0) {
            cxf = (lo + 180.0) / 360.0 * nn;
            cyf = (1.0 - asinh(tan(la * M_PI / 180.0)) / M_PI) / 2.0 * nn;
            g_followGps = true; status = "Following GPS";
          }
        }
        redraw = true;
      }
      else if (a == 13) {                       // Save current map centre as a place
        favSave(cLatE, cLonE); status = "Place saved"; redraw = true;
      }
      else if (a == 14) {                       // Favourites: pick a saved place to jump to
        int32_t fl, fo;
        bool picked = showFavList(fl, fo);
        if (picked) {
          cxf = (fo / 1e7 + 180.0) / 360.0 * nn;
          cyf = (1.0 - asinh(tan(fl / 1e7 * M_PI / 180.0)) / M_PI) / 2.0 * nn;
          g_followGps = false; status = "Jumped to place";
        }
        redraw = true;                          // the list took over the screen; full redraw
      }
    }
    // live-refresh the coordinate panel ~1x/sec while it's shown
    if (g_showCoords && millis() - lastCoord > 1000) { drawCoordsPanel(); lastCoord = millis(); }
    // GPS-follow: keep the map centred on our live position every few seconds
    if (g_followGps && gps_obj.getFixStatus() && millis() - lastFollow > 4000) {
      double la = gps_obj.getLat().toDouble(), lo = gps_obj.getLon().toDouble();
      if (la != 0.0 || lo != 0.0) {
        double nf = (double)(1UL << Z);
        cxf = (lo + 180.0) / 360.0 * nf;
        cyf = (1.0 - asinh(tan(la * M_PI / 180.0)) / M_PI) / 2.0 * nf;
        redraw = true;
      }
      lastFollow = millis();
    }
    delay(50);
  }
  if (g_png) { delete g_png; g_png = nullptr; }
  display_obj.tft.setSwapBytes(false);
}

// SD Wi-Fi file server lives in FileServerHTTP.cpp (runFileServer) to keep
// <WebServer.h> isolated from ESPAsyncWebServer (which EvilPortal pulls in).

// ================= SD card file browser =================
#define SDB_PER 8
static void drawSDBrowser(const String &path, String names[], bool dirs[], uint32_t sizes[],
                          int count, int pageStart, bool hasMore) {
  display_obj.tft.fillScreen(TFT_BLACK);
  display_obj.tft.setFreeFont(NULL);
  mapBtn(4, 4, 40, 28, TFT_NAVY, "<", 2);            // up / back
  mapBtn(150, 4, 86, 28, TFT_DARKGREEN, "File Srv", 1);  // start Wi-Fi file server
  display_obj.tft.setTextDatum(TL_DATUM);
  display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String title = path.length() ? path : "/";
  if (title.length() > 17) title = "..." + title.substring(title.length() - 14);
  display_obj.tft.drawString(title, 50, 12, 1);
  for (int i = 0; i < count; i++) {
    int y = 38 + i * 28;
    display_obj.tft.drawRect(2, y, 236, 26, TFT_DARKGREY);
    String nm = names[i];
    if (dirs[i]) nm = "[" + nm + "]";
    if (nm.length() > 30) nm = nm.substring(0, 29) + "~";
    display_obj.tft.setTextDatum(TL_DATUM);
    display_obj.tft.setTextColor(dirs[i] ? TFT_YELLOW : TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString(nm, 6, y + 9, 1);
    if (!dirs[i]) {
      String sz = sizes[i] >= 1048576 ? String(sizes[i] / 1048576) + "M"
                : sizes[i] >= 1024 ? String(sizes[i] / 1024) + "K"
                : String(sizes[i]) + "B";
      display_obj.tft.setTextDatum(TR_DATUM);
      display_obj.tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      display_obj.tft.drawString(sz, 232, y + 9, 1);
    }
  }
  if (count == 0) {
    display_obj.tft.setTextDatum(MC_DATUM);
    display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    display_obj.tft.drawString("(empty)", TFT_WIDTH / 2, 150, 2);
  }
  mapBtn(4, 288, 74, 26, pageStart > 0 ? TFT_NAVY : TFT_DARKGREY, "Prev", 1);
  mapBtn(82, 288, 76, 26, TFT_MAROON, "Exit", 1);
  mapBtn(162, 288, 74, 26, hasMore ? TFT_NAVY : TFT_DARKGREY, "Next", 1);
  display_obj.tft.setTextDatum(TL_DATUM);
}

static int sdBrowserHit(int x, int y, int count) {
  if (x >= 2 && x <= 46 && y >= 2 && y <= 34) return 1;          // up / back
  if (x >= 148 && x <= 238 && y >= 2 && y <= 34) return 5;       // File Server
  if (y >= 286 && y <= 318) {
    if (x >= 2  && x <= 80)  return 2;   // prev
    if (x >= 81 && x <= 160) return 4;   // exit
    if (x >= 161 && x <= 238) return 3;  // next
  }
  if (y >= 38 && y < 38 + count * 28) {
    int i = (y - 38) / 28;
    if (i >= 0 && i < count) return 100 + i;
  }
  return 0;
}

static void showSDBrowser() {
  String path = "/";
  int pageStart = 0;
  String names[SDB_PER]; bool dirs[SDB_PER]; uint32_t sizes[SDB_PER];
  while (true) {
    // load the current page of directory entries
    int count = 0; bool hasMore = false; int idx = 0;
    File dir = SD.open(path.length() ? path.c_str() : "/");
    if (dir && dir.isDirectory()) {
      for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
        if (idx >= pageStart && count < SDB_PER) {
          names[count] = baseName(e.name());
          dirs[count] = e.isDirectory();
          sizes[count] = e.isDirectory() ? 0 : (uint32_t)e.size();
          count++;
        } else if (idx >= pageStart + SDB_PER) { hasMore = true; e.close(); break; }
        idx++;
        e.close();
      }
    }
    if (dir) dir.close();
    drawSDBrowser(path, names, dirs, sizes, count, pageStart, hasMore);

    bool refresh = false;
    uint16_t tx, ty;
    while (!refresh) {
      if (display_obj.tft.getTouch(&tx, &ty, 350)) {
        int a = sdBrowserHit(tx, ty, count);
        while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
        if (a == 4) return;                                   // Exit
        else if (a == 5) { runFileServer(); refresh = true; }    // start Wi-Fi file server, then redraw
        else if (a == 1) {                                    // Up / back
          if (path == "/" || path.length() == 0) return;      // at root -> leave
          int sl = path.lastIndexOf('/');
          path = (sl <= 0) ? "/" : path.substring(0, sl);
          pageStart = 0; refresh = true;
        } else if (a == 2) { if (pageStart > 0) { pageStart -= SDB_PER; if (pageStart < 0) pageStart = 0; refresh = true; } }
        else if (a == 3) { if (hasMore) { pageStart += SDB_PER; refresh = true; } }
        else if (a >= 100) {
          int i = a - 100;
          if (i < count && dirs[i]) {                         // enter folder
            path = (path == "/" ? "" : path) + "/" + names[i];
            pageStart = 0; refresh = true;
          }
        }
      }
      delay(30);
    }
  }
}

// ================= Colour theme picker (button line + text colour) =================
static void showColorTheme() {
  struct Pal { const char *n; uint16_t c; };
  static const Pal PAL[] = {
    {"White", TFT_WHITE}, {"Grey", TFT_DARKGREY}, {"Red", TFT_RED}, {"Orange", TFT_ORANGE}, {"Yellow", TFT_YELLOW},
    {"Green", TFT_GREEN}, {"Cyan", TFT_CYAN}, {"Blue", TFT_BLUE}, {"Pink", TFT_MAGENTA}, {"Purple", TFT_PURPLE}
  };
  const int NP = 10;
  int target = 0;              // 0 = lines, 1 = text, 2 = background
  bool redraw = true;
  while (true) {
    if (redraw) {
      display_obj.tft.fillScreen(TFT_BLACK);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextDatum(TL_DATUM);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.drawString("Color Theme", 8, 6, 2);
      // which attribute you're editing (Lines / Text / Bg)
      mapBtn(4,   32, 74, 28, target == 0 ? TFT_BLUE : TFT_NAVY, "Lines", 2);
      mapBtn(84,  32, 74, 28, target == 1 ? TFT_BLUE : TFT_NAVY, "Text",  2);
      mapBtn(164, 32, 72, 28, target == 2 ? TFT_BLUE : TFT_NAVY, "Bg",    2);
      // colour swatches (5 x 2)
      uint16_t cur = (target == 0) ? g_lineColor : (target == 1) ? g_textColor : g_bgColor;
      for (int i = 0; i < NP; i++) {
        int c = i % 5, r = i / 5;
        int x = 6 + c * 47, y = 72 + r * 50;
        display_obj.tft.fillRoundRect(x, y, 42, 42, 4, PAL[i].c);
        bool sel = (PAL[i].c == cur);
        display_obj.tft.drawRoundRect(x, y, 42, 42, 4, sel ? TFT_WHITE : TFT_DARKGREY);
        if (sel) display_obj.tft.drawRoundRect(x + 1, y + 1, 40, 40, 4, TFT_WHITE);
      }
      // quick reset: Text:Auto (item colour) or Bg:Black depending on target
      if (target == 1)
        mapBtn(6, 176, 104, 26, (g_textColor == 0) ? TFT_DARKGREEN : TFT_NAVY, "Text:Auto", 1);
      else if (target == 2)
        mapBtn(6, 176, 104, 26, (g_bgColor == TFT_BLACK) ? TFT_DARKGREEN : TFT_NAVY, "Bg:Black", 1);
      // live sample: filled with the background, outlined in the line colour
      display_obj.tft.fillRoundRect(120, 174, 114, 30, 3, g_bgColor);
      display_obj.tft.drawRoundRect(120, 174, 114, 30, 3, g_lineColor);
      display_obj.tft.setTextDatum(MC_DATUM);
      display_obj.tft.setTextColor(g_textColor ? g_textColor : TFT_YELLOW, g_bgColor);
      display_obj.tft.drawString("Sample", 177, 189, 2);
      display_obj.tft.setTextDatum(TL_DATUM);
      mapBtn(60, 262, 120, 36, TFT_MAROON, "Save & Back", 2);
      redraw = false;
    }
    uint16_t tx, ty;
    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      int ax = tx, ay = ty;
      while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
      if (ay >= 32 && ay <= 60) {
        if (ax >= 4 && ax <= 78) { target = 0; redraw = true; }
        else if (ax >= 84 && ax <= 158) { target = 1; redraw = true; }
        else if (ax >= 164 && ax <= 236) { target = 2; redraw = true; }
      } else if (ay >= 72 && ay < 172) {
        int r = (ay - 72) / 50, c = (ax - 6) / 47, i = r * 5 + c;
        if (c >= 0 && c < 5 && r >= 0 && r < 2 && i < NP) {
          if (target == 0) g_lineColor = PAL[i].c;
          else if (target == 1) g_textColor = PAL[i].c;
          else g_bgColor = PAL[i].c;
          redraw = true;
        }
      } else if (ay >= 176 && ay <= 202 && ax >= 6 && ax <= 110) {
        if (target == 1) { g_textColor = 0; redraw = true; }
        else if (target == 2) { g_bgColor = TFT_BLACK; redraw = true; }
      } else if (ay >= 262 && ay <= 298 && ax >= 60 && ax <= 180) {
        saveTheme();
        return;
      }
    }
    delay(30);
  }
}

// ---- GPS Settings: pick the ESP GPIOs wired to the GPS module and the baud ----
static void showGpsSettings() {
  loadGpsCfg();
  const int NR = sizeof(GPS_RX_CANDS) / sizeof(GPS_RX_CANDS[0]);
  const int NT = sizeof(GPS_TX_CANDS) / sizeof(GPS_TX_CANDS[0]);
  const int NB = sizeof(GPS_BAUD_CANDS) / sizeof(GPS_BAUD_CANDS[0]);
  const int NH = sizeof(GPS_HZ_CANDS) / sizeof(GPS_HZ_CANDS[0]);
  bool redraw = true;
  // row control geometry (shared by all four rows)
  const int LX = 6, LW = 40, VX = 52, VW = 136, RX = 194, RW = 40, BH = 28;
  const int y1 = 44, y2 = 102, y3 = 160, y4 = 218;   // control-row Y: RX / TX / Baud / Rate

  while (true) {
    if (redraw) {
      display_obj.tft.fillScreen(TFT_BLACK);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextDatum(TL_DATUM);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.drawString("GPS Settings", 8, 6, 2);

      struct Row { const char *lab; int y; String val; };
      Row rows[4] = {
        { "GPS TX",    y1, String(g_gpsEspRx) },
        { "GPS RX",    y2, String(g_gpsEspTx) },
        { "Baud rate", y3, String(g_gpsBaud) },
        { "Rate (Hz)", y4, String(g_gpsHz) },
      };
      for (int i = 0; i < 4; i++) {
        // Label: force top-left alignment every time (mapBtn leaves the datum
        // centred, which would push the next label off the left edge).
        display_obj.tft.setTextDatum(TL_DATUM);
        display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
        display_obj.tft.drawString(rows[i].lab, LX, rows[i].y - 18, 2);
        mapBtn(LX, rows[i].y, LW, BH, TFT_NAVY, "<", 2);
        display_obj.tft.drawRoundRect(VX, rows[i].y, VW, BH, 3, TFT_DARKGREY);
        display_obj.tft.setTextDatum(MC_DATUM);
        display_obj.tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
        display_obj.tft.drawString(rows[i].val, VX + VW / 2, rows[i].y + BH / 2, 2);
        display_obj.tft.setTextDatum(TL_DATUM);
        mapBtn(RX, rows[i].y, RW, BH, TFT_NAVY, ">", 2);
      }
      display_obj.tft.setTextDatum(TL_DATUM);
      display_obj.tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
      display_obj.tft.drawString("Save re-inits GPS (pins/baud/rate).", 8, 250, 1);
      mapBtn(16, 268, 100, 36, TFT_MAROON, "Save", 2);
      mapBtn(128, 268, 100, 36, TFT_DARKGREY, "Back", 2);
      redraw = false;
    }

    uint16_t tx, ty;
    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      int ax = tx, ay = ty;
      while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);

      // which row?
      int row = -1;
      if (ay >= y1 && ay <= y1 + BH) row = 0;
      else if (ay >= y2 && ay <= y2 + BH) row = 1;
      else if (ay >= y3 && ay <= y3 + BH) row = 2;
      else if (ay >= y4 && ay <= y4 + BH) row = 3;

      if (row >= 0) {
        int dir = 0;
        if (ax >= LX && ax <= LX + LW) dir = -1;
        else if (ax >= RX && ax <= RX + RW) dir = +1;
        if (dir != 0) {
          if (row == 0) g_gpsEspRx = gpsStep(GPS_RX_CANDS, NR, g_gpsEspRx, dir);
          else if (row == 1) g_gpsEspTx = gpsStep(GPS_TX_CANDS, NT, g_gpsEspTx, dir);
          else if (row == 2) g_gpsBaud = gpsStepU(GPS_BAUD_CANDS, NB, g_gpsBaud, dir);
          else g_gpsHz = gpsStepU(GPS_HZ_CANDS, NH, g_gpsHz, dir);
          redraw = true;
        }
      } else if (ay >= 268 && ay <= 304) {
        if (ax >= 16 && ax <= 116) {            // Save
          saveGpsCfg();
          display_obj.tft.fillScreen(TFT_BLACK);
          display_obj.tft.setTextDatum(MC_DATUM);
          display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
          display_obj.tft.drawString("Saved. Applying...", 120, 150, 2);
          display_obj.tft.setTextDatum(TL_DATUM);
          #ifdef HAS_GPS
            gps_obj.begin();   // re-open Serial2 with the new pins/baud/rate
          #endif
          return;
        } else if (ax >= 128 && ax <= 228) {    // Back (discard)
          return;
        }
      }
    }
    delay(30);
  }
}

// ---- Fuel Gauge: show expected pins and whether a MAX1704x is on the bus ----
static void showFuelGauge() {
  #ifndef I2C_SDA
    #define I2C_SDA 22
  #endif
  #ifndef I2C_SCL
    #define I2C_SCL 27
  #endif
  bool redraw = true;
  while (true) {
    if (redraw) {
      // Live-probe + initialise the MAX1704x fuel gauge (Adafruit MAX17048 or a
      // MAX17043) right now, so a hot-plugged gauge reads without a reboot.
      bool connected = battery_obj.reinit();

      display_obj.tft.fillScreen(TFT_BLACK);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextDatum(TL_DATUM);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.drawString("Fuel Gauge", 8, 6, 2);

      display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
      display_obj.tft.drawString("Expected wiring:", 8, 42, 2);
      display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      display_obj.tft.drawString("SDA (data)  ->  IO22", 16, 70, 2);
      display_obj.tft.drawString("SCL (clock) ->  IO27", 16, 94, 2);
      display_obj.tft.drawString("Chip: MAX17048 @ 0x36", 16, 118, 1);

      // Status banner
      display_obj.tft.drawRoundRect(8, 150, 224, 54, 4, connected ? TFT_GREEN : TFT_RED);
      display_obj.tft.setTextDatum(MC_DATUM);
      if (connected) {
        display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
        display_obj.tft.drawString("Fuel gauge CONNECTED", 120, 166, 2);
        int8_t lvl = battery_obj.getBatteryLevel();
        float v = battery_obj.getVoltage();
        String line = "";
        if (lvl >= 0) line += String(lvl) + "%";
        if (v > 0)    line += (line.length() ? "   " : "") + String(v, 2) + " V";
        if (line.length() == 0) line = "reading...";
        display_obj.tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
        display_obj.tft.drawString(line, 120, 190, 2);
      } else {
        display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
        display_obj.tft.drawString("Not detected", 120, 168, 2);
        display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        display_obj.tft.drawString("Connect it to IO22 / IO27", 120, 190, 1);
      }
      display_obj.tft.setTextDatum(TL_DATUM);

      mapBtn(16, 268, 100, 36, TFT_NAVY, "Refresh", 1);
      mapBtn(128, 268, 100, 36, TFT_DARKGREY, "Back", 2);
      redraw = false;
    }

    uint16_t tx, ty;
    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      int ax = tx, ay = ty;
      while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
      if (ay >= 268 && ay <= 304) {
        if (ax >= 16 && ax <= 116) { redraw = true; }      // Refresh: re-probe
        else if (ax >= 128 && ax <= 228) { return; }       // Back
      }
    }
    delay(30);
  }
}

// ---- UTC Offset: set the local-time offset used for the GPS clock ----
static void showUtcSetting() {
  loadUtcCfg();
  const int STEP = 30;        // 30-minute steps (covers half-hour zones)
  const int MINO = -720;      // UTC-12:00
  const int MAXO = 840;       // UTC+14:00
  bool redraw = true;
  while (true) {
    if (redraw) {
      display_obj.tft.fillScreen(TFT_BLACK);
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextDatum(TL_DATUM);
      display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
      display_obj.tft.drawString("UTC Offset", 8, 6, 2);

      display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      display_obj.tft.drawString("Local time = GPS (UTC) + offset", 8, 40, 1);

      // value with - / + steppers
      mapBtn(8, 90, 48, 44, TFT_NAVY, "-", 2);
      display_obj.tft.drawRoundRect(64, 90, 112, 44, 4, TFT_DARKGREY);
      display_obj.tft.setTextDatum(MC_DATUM);
      display_obj.tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      display_obj.tft.drawString(fmtUtc(g_utcOffsetMin), 120, 112, 2);
      display_obj.tft.setTextDatum(TL_DATUM);
      mapBtn(184, 90, 48, 44, TFT_NAVY, "+", 2);

      // live local-time preview (needs a GPS fix)
      String t = gps_obj.getTime(g_utcOffsetMin);
      display_obj.tft.setTextDatum(MC_DATUM);
      display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
      display_obj.tft.drawString(t.length() ? ("Now: " + t) : "Now: (waiting for GPS fix)",
                                 120, 160, 2);
      display_obj.tft.setTextDatum(TL_DATUM);

      mapBtn(16, 262, 100, 40, TFT_MAROON, "Save", 2);
      mapBtn(128, 262, 100, 40, TFT_DARKGREY, "Back", 2);
      redraw = false;
    }
    uint16_t tx, ty;
    if (display_obj.tft.getTouch(&tx, &ty, 350)) {
      int ax = tx, ay = ty;
      while (display_obj.tft.getTouch(&tx, &ty, 350)) delay(10);
      if (ay >= 90 && ay <= 134) {
        if (ax >= 8 && ax <= 56) { g_utcOffsetMin -= STEP; if (g_utcOffsetMin < MINO) g_utcOffsetMin = MINO; redraw = true; }
        else if (ax >= 184 && ax <= 232) { g_utcOffsetMin += STEP; if (g_utcOffsetMin > MAXO) g_utcOffsetMin = MAXO; redraw = true; }
      } else if (ay >= 262 && ay <= 302) {
        if (ax >= 16 && ax <= 116) { saveUtcCfg(); return; }   // Save
        else if (ax >= 128 && ax <= 228) { loadUtcCfg(); return; }  // Back (discard)
      }
    }
    delay(30);
  }
}

MenuFunctions::MenuFunctions()
{
}

// LVGL Stuff
/* Interrupt driven periodic handler */
#if defined(HAS_ST7789) || defined(HAS_ILI9341) || defined(HAS_ST7796)
    #if defined(CYD_32CAP) || defined(CYD_35CAP)
      uint8_t MenuFunctions::updateTouch(int16_t *x, int16_t *y, uint16_t threshold) {
        static bool was_pressed = false;
        if (!display_obj.headless_mode) {
          int16_t t_x[5] = {0,0,0,0,0}, t_y[5] = {0,0,0,0,0};
          uint8_t result = touch.getPoint(t_x, t_y, touch.getSupportTouchPoint());
          bool pressed = result > 0;

          if (pressed && !was_pressed) {
            for (int i = 0; i < result; i++) {
              x[i] = t_x[i];
              y[i] = t_y[i];
            }
            
            int16_t tmp_x[5] = {0,0,0,0,0}, tmp_y[5] = {0,0,0,0,0};
            for (int i = 0; i < THROW_AWAY_TOUCH_COUNT; i++) {
              touch.getPoint(tmp_x, tmp_y, touch.getSupportTouchPoint());
              delay(1);
            }
            was_pressed = true;
            return result;
          } else if (!pressed) {
            was_pressed = false;
          }
          return 0;
        } else {
          Serial.println("headless mode");
          return !display_obj.headless_mode;
        }
      }
    #elif defined(CYD_24CAP) || defined(CYD_22CAP)
    uint8_t MenuFunctions::updateTouch(uint16_t *x, uint16_t *y, uint16_t threshold) {
      if (!display_obj.headless_mode)
        return display_obj.tft.getTouchBBC(x, y, threshold);
      else
        return !display_obj.headless_mode;
    }
    #else
      // Resistive touch
      uint8_t MenuFunctions::updateTouch(uint16_t *x, uint16_t *y, uint16_t threshold) {
        if (!display_obj.headless_mode)
          return display_obj.tft.getTouch(x, y, threshold);
        else
          return !display_obj.headless_mode;
      }
    #endif
  



  void MenuFunctions::lv_tick_handler()
  {
    lv_tick_inc(LVGL_TICK_PERIOD);
  }
  
  /* Display flushing */
  void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
  {
    extern Display display_obj;
    uint16_t c;
  
    display_obj.tft.startWrite();
    display_obj.tft.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1));
    for (int y = area->y1; y <= area->y2; y++) {
      for (int x = area->x1; x <= area->x2; x++) {
        c = color_p->full;
        display_obj.tft.writeColor(c, 1);
        color_p++;
      }
    }
    display_obj.tft.endWrite();
    lv_disp_flush_ready(disp);
  }
  
  bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    extern Display display_obj;
    static bool was_pressed = false;
    bool touched = false;
  
    #if defined(CYD_32CAP) || defined(CYD_35CAP)    
      int16_t touchX = 0, touchY = 0;
    #else
      uint16_t touchX, touchY;
    #endif

  #if defined(CYD_32CAP) || defined(CYD_35CAP)
      touch.setMaxCoordinates(SCREEN_HEIGHT, SCREEN_WIDTH);
      touch.setSwapXY(true);
      touch.setMirrorXY(true, true);
      int16_t touchX_array[5] = {0, 0, 0, 0, 0}, touchY_array[5] = {0, 0, 0, 0, 0};
      int16_t points = touch.getPoint(touchX_array, touchY_array, touch.getSupportTouchPoint());
      touched = (points > 0);
      if (touched) {
          touchX = touchX_array[0];
          touchY = touchY_array[0];
          if (!was_pressed) {
              for (int i = 0; i < THROW_AWAY_TOUCH_COUNT; i++) {
                  int16_t tmp_x[5] = {0, 0, 0, 0, 0}, tmp_y[5] = {0, 0, 0, 0, 0};
                  touch.getPoint(tmp_x, tmp_y, touch.getSupportTouchPoint());
                  delay(1);
              }
          }
      }
      was_pressed = touched;
  #elif defined(CYD_24CAP)
      touched = display_obj.tft.getTouchBBC(&touchX, &touchY, 600);
  #else
      touched = display_obj.tft.getTouch(&touchX, &touchY, 350);
  #endif

  // Handle touch state and coordinates
  if (!touched) {
      data->state = LV_INDEV_STATE_REL;
      return false;
  }
  if (touchX >= WIDTH_1 || touchY >= HEIGHT_1) {
      Serial.println("Touch outside expected parameters:");
      Serial.print("x: ");
      Serial.print(touchX);
      Serial.print(" y: ");
      Serial.print(touchY);
      data->state = LV_INDEV_STATE_REL;
      return false;
  }

  data->state = LV_INDEV_STATE_PR;
  data->point.x = touchX;
  data->point.y = touchY;
  return false;
}
    

  void MenuFunctions::initLVGL() {
    tick.attach_ms(LVGL_TICK_PERIOD, lv_tick_handler);
    lv_init();
  
    lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);
  
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = WIDTH_1;
    disp_drv.ver_res = HEIGHT_1;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);
  
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);             
    indev_drv.type = LV_INDEV_TYPE_POINTER;    
    indev_drv.read_cb = my_touchpad_read;      
    lv_indev_drv_register(&indev_drv);

  //#if defined(CYD_32CAP) || defined(CYD_35CAP)
  //  touch.setMaxCoordinates(SCREEN_WIDTH, SCREEN_HEIGHT);
  //  touch.setSwapXY(false);
  //  touch.setMirrorXY(false, false);
  //#endif
      //display_obj.exit_draw = true;         
  }
  
  
  void MenuFunctions::deinitLVGL() {
      //Serial.println(F("Deinit LVGL"));
      #if defined(CYD_32CAP) || defined(CYD_35CAP)
        touch.setMaxCoordinates(SCREEN_WIDTH, SCREEN_HEIGHT);
        touch.setSwapXY(false);
        touch.setMirrorXY(false, false);
      #endif
      //display_obj.exit_draw = true;
  }
  
  
  // Event handler for settings drop down menus
  void setting_dropdown_cb(lv_obj_t * obj, lv_event_t event) {

  }

  // GFX Function to build a list showing all Stations scanned
  void MenuFunctions::addStationGFX(){
    extern LinkedList<Station>* stations;
    extern LinkedList<AccessPoint>* access_points;
    extern WiFiScan wifi_scan_obj;
  
    lv_obj_t * list1 = lv_list_create(lv_scr_act(), NULL);
    lv_obj_set_size(list1, 320, 240);
    lv_obj_set_width(list1, LV_HOR_RES);
    lv_obj_align(list1, NULL, LV_ALIGN_CENTER, 0, 0);
  
    lv_obj_t * list_btn;
  
    lv_obj_t * label;
  
    list_btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, text09);
    lv_obj_set_event_cb(list_btn, station_list_cb);

    char addr[] = "00:00:00:00:00:00";
    for (int x = 0; x < access_points->size(); x++) {
      AccessPoint cur_ap = access_points->get(x);

      // Add non clickable button for AP
      String full_label = "AP: " + cur_ap.essid;
      char buf[full_label.length() + 1] = {};
      full_label.toCharArray(buf, full_label.length() + 1);
      list_btn = lv_list_add_btn(list1, NULL, buf);
      lv_btn_set_checkable(list_btn, false);
      
      int cur_ap_sta_len = access_points->get(x).stations->size();
      for (int y = 0; y < cur_ap_sta_len; y++) {
        Station cur_sta = stations->get(cur_ap.stations->get(y));
        // Convert uint8_t MAC to char array
        wifi_scan_obj.getMAC(addr, cur_sta.mac, 0);
        
        list_btn = lv_list_add_btn(list1, LV_SYMBOL_WIFI, addr);
        lv_btn_set_checkable(list_btn, true);
        lv_obj_set_event_cb(list_btn, station_list_cb);
    
        if (cur_sta.selected)
          lv_btn_toggle(list_btn);
      }
    }
  }

  // Function to work with list of Stations
  void station_list_cb(lv_obj_t * btn, lv_event_t event) {
    extern LinkedList<Station>* stations;
    extern MenuFunctions menu_function_obj;
    extern WiFiScan wifi_scan_obj;
  
    String btn_text = lv_list_get_btn_text(btn);
    String display_string = "";
    char addr[] = "00:00:00:00:00:00";
    
    if (event == LV_EVENT_CLICKED) {
      if (btn_text != text09) {
      }
      else {
        Serial.println("Exiting...");
        lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(btn)));
  
        for (int i = 0; i < stations->size(); i++) {
          if (stations->get(i).selected) {
            wifi_scan_obj.getMAC(addr, stations->get(i).mac, 0);
            Serial.print("Selected: ");
            Serial.println(addr);
          }
        }
  
        printf("LV_EVENT_CANCEL\n");
        menu_function_obj.deinitLVGL();
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        display_obj.exit_draw = true; // set everything back to normal
      }
    }
    
    if (event == LV_EVENT_VALUE_CHANGED) {     
      if (lv_btn_get_state(btn) == LV_BTN_STATE_CHECKED_RELEASED) {
        for (int i = 0; i < stations->size(); i++) {
          wifi_scan_obj.getMAC(addr, stations->get(i).mac, 0); 
          if (strcmp(addr, btn_text.c_str()) == 0) {
            Serial.print("Adding Station: ");
            Serial.println(addr);
            Station sta = stations->get(i);
            sta.selected = true;
            stations->set(i, sta);
          }
        }
      }
      else {
        for (int i = 0; i < stations->size(); i++) {
          wifi_scan_obj.getMAC(addr, stations->get(i).mac, 0); 
          if (strcmp(addr, btn_text.c_str()) == 0) {
            Serial.print("Removing Station: ");
            Serial.println(addr);
            Station sta = stations->get(i);
            sta.selected = false;
            stations->set(i, sta);
          }
        }
      }
    }
  }

  // GFX Function to build a list showing all EP HTML Files
  void MenuFunctions::selectEPHTMLGFX() {
    extern EvilPortal evil_portal_obj;
  
    lv_obj_t * list1 = lv_list_create(lv_scr_act(), NULL);
    lv_obj_set_size(list1, 160, 200);
    lv_obj_set_width(list1, LV_HOR_RES);
    lv_obj_align(list1, NULL, LV_ALIGN_CENTER, 0, 0);
  
    lv_obj_t * list_btn;
  
    lv_obj_t * label;
  
    list_btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, text09);
    lv_obj_set_event_cb(list_btn, html_list_cb);
  
    for (int i = 1; i < evil_portal_obj.html_files->size(); i++) {
      char buf[evil_portal_obj.html_files->get(i).length() + 1] = {};
      evil_portal_obj.html_files->get(i).toCharArray(buf, evil_portal_obj.html_files->get(i).length() + 1);
      
      list_btn = lv_list_add_btn(list1, LV_SYMBOL_FILE, buf);
      lv_btn_set_checkable(list_btn, true);
      lv_obj_set_event_cb(list_btn, html_list_cb);
  
      if (i == evil_portal_obj.selected_html_index)
        lv_btn_toggle(list_btn);
    }
  }

  void html_list_cb(lv_obj_t * btn, lv_event_t event) {
    extern EvilPortal evil_portal_obj;
    extern MenuFunctions menu_function_obj;
  
    String btn_text = lv_list_get_btn_text(btn);
    String display_string = "";
    
    if (event == LV_EVENT_CLICKED) {
      if (btn_text != text09) {
      }
      else {
        Serial.println("Exiting...");
        lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(btn)));
  
        for (int i = 1; i < evil_portal_obj.html_files->size(); i++) {
          if (i == evil_portal_obj.selected_html_index) {
            Serial.println("Selected: " + (String)evil_portal_obj.html_files->get(i));
          }
        }
  
        printf("LV_EVENT_CANCEL\n");
        menu_function_obj.deinitLVGL();
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        display_obj.exit_draw = true; // set everything back to normal
      }
    }
    
    if (event == LV_EVENT_VALUE_CHANGED) {      
      if (lv_btn_get_state(btn) == LV_BTN_STATE_CHECKED_RELEASED) {
        for (int i = 1; i < evil_portal_obj.html_files->size(); i++) {
          if (evil_portal_obj.html_files->get(i) == btn_text) {
            Serial.println("Setting HTML: " + (String)evil_portal_obj.html_files->get(i));
            evil_portal_obj.selected_html_index = i;
            evil_portal_obj.target_html_name = (String)evil_portal_obj.html_files->get(i);
          }
        }

        // Deselect buttons that were previously selected
        lv_obj_t * list = lv_obj_get_parent(btn);

        lv_obj_t * next_btn = lv_obj_get_child(list, NULL);
        while (next_btn != NULL) {
          if (next_btn != btn) {
            lv_btn_set_state(next_btn, LV_BTN_STATE_RELEASED);
          }
          next_btn = lv_obj_get_child(list, next_btn);
        }
      }
    }
  }
  
  // GFX Function to build a list showing all APs scanned
  void MenuFunctions::addAPGFX(String type){
    extern WiFiScan wifi_scan_obj;
    extern LinkedList<AccessPoint>* access_points;
    extern LinkedList<AirTag>* airtags;
  
    lv_obj_t * list1 = lv_list_create(lv_scr_act(), NULL);
    lv_obj_set_size(list1, 160, 200);
    lv_obj_set_width(list1, LV_HOR_RES);
    lv_obj_align(list1, NULL, LV_ALIGN_CENTER, 0, 0);
  
    lv_obj_t * list_btn;
  
    lv_obj_t * label;
  
    list_btn = lv_list_add_btn(list1, LV_SYMBOL_CLOSE, text09);
    lv_obj_set_event_cb(list_btn, ap_list_cb);
  
    if ((type == "AP") || (type == "AP Info")) {
      for (int i = 0; i < access_points->size(); i++) {
        char buf[access_points->get(i).essid.length() + 1] = {};
        access_points->get(i).essid.toCharArray(buf, access_points->get(i).essid.length() + 1);
        
        list_btn = lv_list_add_btn(list1, LV_SYMBOL_WIFI, buf);
        lv_btn_set_checkable(list_btn, true);
        if (type == "AP")
          lv_obj_set_event_cb(list_btn, ap_list_cb);
        else if (type == "AP Info")
          lv_obj_set_event_cb(list_btn, ap_info_list_cb);
    
        if (access_points->get(i).selected)
          lv_btn_toggle(list_btn);
      }
    }
    else if (type == "Airtag") {
      for (int i = 0; i < airtags->size(); i++) {
        char buf[airtags->get(i).mac.length() + 1] = {};
        airtags->get(i).mac.toCharArray(buf, airtags->get(i).mac.length() + 1);
        
        list_btn = lv_list_add_btn(list1, LV_SYMBOL_BLUETOOTH, buf);
        lv_btn_set_checkable(list_btn, true);
        lv_obj_set_event_cb(list_btn, at_list_cb);
    
        //if (airtags->get(i).selected)
        //  lv_btn_toggle(list_btn);
      }
    }
  }
  
  void at_list_cb(lv_obj_t * btn, lv_event_t event) {
    extern MenuFunctions menu_function_obj;
    extern WiFiScan wifi_scan_obj;
    extern LinkedList<AirTag>* airtags;
    extern Display display_obj;
  
    String btn_text = lv_list_get_btn_text(btn);
    String display_string = "";
    
    // Button is clicked
    if (event == LV_EVENT_CLICKED) {
      if (btn_text != text09) {
      }
      // It's the back button
      else {
        Serial.println("Exiting...");
        lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(btn)));
  
        for (int i = 0; i < airtags->size(); i++) {
          if (airtags->get(i).selected) {
            Serial.println("Selected: " + (String)airtags->get(i).mac);
          }
        }
  
        printf("LV_EVENT_CANCEL\n");
        menu_function_obj.deinitLVGL();
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        display_obj.exit_draw = true; // set everything back to normal
      }
    }
    
    if (event == LV_EVENT_VALUE_CHANGED) {      
      if (lv_btn_get_state(btn) == LV_BTN_STATE_CHECKED_RELEASED) {
        bool do_that_thang = false;
        for (int i = 0; i < airtags->size(); i++) {
          if (airtags->get(i).mac == btn_text) {
            Serial.println("Selecting Airtag: " + (String)airtags->get(i).mac);
            AirTag at = airtags->get(i);
            at.selected = true;
            airtags->set(i, at);
            do_that_thang = true;
          }
          else {
            AirTag at = airtags->get(i);
            at.selected = false;
            airtags->set(i, at);
          }
        }
        // Start spoofing airtag
        if (do_that_thang) {
          menu_function_obj.deinitLVGL();
          lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(btn)));
          wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
          display_obj.clearScreen();
          menu_function_obj.orientDisplay();
          display_obj.clearScreen();
          menu_function_obj.drawStatusBar();
          wifi_scan_obj.StartScan(BT_SPOOF_AIRTAG, TFT_WHITE);
        }
      }
      else {
        for (int i = 0; i < airtags->size(); i++) {
          if (airtags->get(i).mac == btn_text) {
            Serial.println("Deselecting Airtag: " + (String)airtags->get(i).mac);
            AirTag at = airtags->get(i);
            at.selected = false;
            airtags->set(i, at);
          }
        }
      }
    }
  }
  
  void ap_list_cb(lv_obj_t * btn, lv_event_t event) {
    extern LinkedList<AccessPoint>* access_points;
    extern MenuFunctions menu_function_obj;
    extern WiFiScan wifi_scan_obj;
  
    String btn_text = lv_list_get_btn_text(btn);
    String display_string = "";
    
    if (event == LV_EVENT_CLICKED) {
      if (btn_text != text09) {
      }
      else {
        Serial.println("Exiting...");
        lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(btn)));
  
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).selected) {
            Serial.println("Selected: " + (String)access_points->get(i).essid);
          }
        }
  
        printf("LV_EVENT_CANCEL\n");
        menu_function_obj.deinitLVGL();
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        display_obj.exit_draw = true; // set everything back to normal
      }
    }
    
    if (event == LV_EVENT_VALUE_CHANGED) {      
      if (lv_btn_get_state(btn) == LV_BTN_STATE_CHECKED_RELEASED) {
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).essid == btn_text) {
            Serial.println("Adding AP: " + (String)access_points->get(i).essid);
            AccessPoint ap = access_points->get(i);
            ap.selected = true;
            access_points->set(i, ap);
          }
        }
      }
      else {
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).essid == btn_text) {
            Serial.println("Removing AP: " + (String)access_points->get(i).essid);
            AccessPoint ap = access_points->get(i);
            ap.selected = false;
            access_points->set(i, ap);
          }
        }
      }
    }
  }
  
  void ap_info_list_cb(lv_obj_t * btn, lv_event_t event) {
    extern LinkedList<AccessPoint>* access_points;
    extern MenuFunctions menu_function_obj;
    extern WiFiScan wifi_scan_obj;
  
    String btn_text = lv_list_get_btn_text(btn);
    String display_string = "";
    
    // Exit function
    if (event == LV_EVENT_CLICKED) {
      if (btn_text != text09) {
        for (int i = 0; i < access_points->size(); i++) {
          if (access_points->get(i).essid == btn_text) {
            lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(btn)));
  
            printf("LV_EVENT_CANCEL\n");
            menu_function_obj.deinitLVGL();
            wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
            //display_obj.exit_draw = true; // set everything back to normal
            menu_function_obj.orientDisplay();
            menu_function_obj.changeMenu(&menu_function_obj.apInfoMenu);
            wifi_scan_obj.RunAPInfo(i);
          }
        }
      }
      else {
        Serial.println("Exiting...");
        lv_obj_del_async(lv_obj_get_parent(lv_obj_get_parent(btn)));
  
        printf("LV_EVENT_CANCEL\n");
        menu_function_obj.deinitLVGL();
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        display_obj.exit_draw = true; // set everything back to normal
      }
    }
  }

  void MenuFunctions::addSSIDGFX(){
    extern LinkedList<ssid>* ssids;
    
    String display_string = "";
    // Create a keyboard and apply the styles
    kb = lv_keyboard_create(lv_scr_act(), NULL);
    lv_obj_set_size(kb, LV_HOR_RES, LV_VER_RES / 2);
    lv_obj_set_event_cb(kb, add_ssid_keyboard_event_cb);
  
    // Create one text area
    // Store all SSIDs
    ta1 = lv_textarea_create(lv_scr_act(), NULL);
    lv_textarea_set_one_line(ta1, false);
    lv_obj_set_width(ta1, LV_HOR_RES);
    lv_obj_set_height(ta1, (LV_VER_RES / 2) - 35);
    lv_obj_set_pos(ta1, 5, 20);
    lv_textarea_set_cursor_hidden(ta1, true);
    lv_obj_align(ta1, NULL, LV_ALIGN_IN_TOP_MID, NULL, NULL);
    lv_textarea_set_placeholder_text(ta1, text_table1[0]);
  
    // Create second text area
    // Add SSIDs
    ta2 = lv_textarea_create(lv_scr_act(), ta1);
    lv_textarea_set_cursor_hidden(ta2, false);
    lv_textarea_set_one_line(ta2, true);
    lv_obj_align(ta2, NULL, LV_ALIGN_IN_TOP_MID, NULL, (LV_VER_RES / 2) - 35);
    lv_textarea_set_text(ta2, "");
    lv_textarea_set_placeholder_text(ta2, text_table1[1]);
    
  
    // After generating text areas, add text to first text box
    for (int i = 0; i < ssids->size(); i++)
      display_string.concat((String)ssids->get(i).essid + "\n");
      
    lv_textarea_set_text(ta1, display_string.c_str());
  
    // Focus it on one of the text areas to start
    lv_keyboard_set_textarea(kb, ta2);
    lv_keyboard_set_cursor_manage(kb, true);
    
  }
  
  // Keyboard callback dedicated to joining wifi
  void add_ssid_keyboard_event_cb(lv_obj_t * keyboard, lv_event_t event){
    extern Display display_obj;
    extern MenuFunctions menu_function_obj;
    extern WiFiScan wifi_scan_obj;
    extern LinkedList<ssid>* ssids;
    
    lv_keyboard_def_event_cb(kb, event);
  
    // User has applied text box
    if(event == LV_EVENT_APPLY){
      String display_string = "";
      printf("LV_EVENT_APPLY\n");
  
      // Get text from SSID text box
      String ta2_text = lv_textarea_get_text(ta2);
      Serial.println(ta2_text);
  
      // Add text box text to list of SSIDs
      wifi_scan_obj.addSSID(ta2_text);
  
      // Update large text box with ssid
      for (int i = 0; i < ssids->size(); i++)
        display_string.concat((String)ssids->get(i).essid + "\n");
      lv_textarea_set_text(ta1, display_string.c_str());
  
      lv_textarea_set_text(ta2, "");
    }else if(event == LV_EVENT_CANCEL){
      printf("LV_EVENT_CANCEL\n");
      menu_function_obj.deinitLVGL();
      display_obj.exit_draw = true; // set everything back to normal
    }
  }
  
  
  void ta_event_cb(lv_obj_t * ta, lv_event_t event)
  {
    if(event == LV_EVENT_CLICKED) {
      if(kb != NULL)
        lv_keyboard_set_textarea(kb, ta);
    }
  
  }

#endif
//// END LV_ARDUINO STUFF

void MenuFunctions::buttonNotSelected(int b, int x) {
  if (x == -1)
    x = b;

  // Ensure b is within valid button index range
  b = (x - menu_start_index) % BUTTON_SCREEN_LIMIT;

  uint16_t color = this->getColor(current_menu->list->get(x).color);

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setFreeFont(NULL);
    display_obj.key[b].drawButton(false, tr(current_menu->list->get(x).name));
  #endif

  #ifdef HAS_FULL_SCREEN
    display_obj.tft.setFreeFont(MENU_FONT);
    display_obj.key[b].drawButton(false, tr(current_menu->list->get(x).name));
    drawLangCode(current_menu->list->get(x).name, KEY_Y + b * (KEY_H + KEY_SPACING_Y));
    if (isActiveLang(current_menu->list->get(x).name))
      drawLangCheck(KEY_Y + b * (KEY_H + KEY_SPACING_Y));
    if (current_menu->list->get(x).name == "Color Theme")
          drawThemeSwatch(KEY_Y + (b * (KEY_H + KEY_SPACING_Y)));
    else if ((current_menu->list->get(x).name != text09) && (current_menu->list->get(x).icon != 255))
          display_obj.tft.drawXBitmap(0,
                                      KEY_Y + (b * (KEY_H + KEY_SPACING_Y)) - (ICON_H / 2),
                                      menu_icons[current_menu->list->get(x).icon],
                                      ICON_W,
                                      ICON_H,
                                      g_bgColor,
                                      color);
    display_obj.tft.setFreeFont(NULL);
  #endif
}

void MenuFunctions::buttonSelected(int b, int x) {
  if (x == -1)
    x = b;

  // Ensure b is within valid button index range
  b = (x - menu_start_index) % BUTTON_SCREEN_LIMIT;

  uint16_t color = this->getColor(current_menu->list->get(x).color);

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setFreeFont(NULL);
    display_obj.key[b].drawButton(true, tr(current_menu->list->get(x).name));
  #endif

  #ifdef HAS_FULL_SCREEN
    display_obj.tft.setFreeFont(MENU_FONT);
    display_obj.key[b].drawButton(true, tr(current_menu->list->get(x).name));
    drawLangCode(current_menu->list->get(x).name, KEY_Y + b * (KEY_H + KEY_SPACING_Y));
    if (isActiveLang(current_menu->list->get(x).name))
      drawLangCheck(KEY_Y + b * (KEY_H + KEY_SPACING_Y));
    if (current_menu->list->get(x).name == "Color Theme")
          drawThemeSwatch(KEY_Y + (b * (KEY_H + KEY_SPACING_Y)));
    else if ((current_menu->list->get(x).name != text09) && (current_menu->list->get(x).icon != 255))
          display_obj.tft.drawXBitmap(0,
                                      KEY_Y + (b * (KEY_H + KEY_SPACING_Y)) - (ICON_H / 2),
                                      menu_icons[current_menu->list->get(x).icon],
                                      ICON_W,
                                      ICON_H,
                                      g_bgColor,
                                      color);
    display_obj.tft.setFreeFont(NULL);
  #endif
}

// Function to check menu input
void MenuFunctions::main(uint32_t currentTime)
{
    if (display_obj.exit_draw) {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        display_obj.exit_draw = false;
        this->orientDisplay();
    }
    if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF) ||
        (wifi_scan_obj.currentScanMode == OTA_UPDATE) ||
        (wifi_scan_obj.currentScanMode == ESP_UPDATE) ||
        (wifi_scan_obj.currentScanMode == SHOW_INFO) ||
        (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_DATA) ||
        (wifi_scan_obj.currentScanMode == WIFI_SCAN_GPS_NMEA)) {
        if (wifi_scan_obj.orient_display) {
            this->orientDisplay();
            wifi_scan_obj.orient_display = false;
        }
    }

    if (currentTime != 0) {
      if (currentTime - initTime >= BANNER_TIME) {
        this->initTime = millis();
        if ((wifi_scan_obj.currentScanMode != LV_JOIN_WIFI) &&
            (wifi_scan_obj.currentScanMode != LV_ADD_SSID))
          this->updateStatusBar();

        // Do channel analyzer stuff
        if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER)){
          #ifdef HAS_SCREEN
            this->setGraphScale(this->graphScaleCheck(wifi_scan_obj._analyzer_values));

            this->drawGraph(wifi_scan_obj._analyzer_values);
          #endif
        }
      }
    }

    static bool ignore_first_touch = true;

    
    boolean pressed = false;

    #if defined(CYD_32CAP) || defined(CYD_35CAP)
      int16_t t_x[5] = {0, 0, 0, 0, 0}, t_y[5] = {0, 0, 0, 0, 0}; // To store the touch coordinates
      int16_t points = 0;
    #else
      uint16_t t_x = 0, t_y = 0;
    #endif
    
    // Handle LVGL modes explicitly
    if (wifi_scan_obj.currentScanMode == LV_JOIN_WIFI || wifi_scan_obj.currentScanMode == LV_ADD_SSID) {
        display_obj.main(wifi_scan_obj.currentScanMode); // Calls lv_task_handler for LVGL
        return;
    }

    if ((wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_BEACON_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AP_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_MANUAL) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_MIMIC) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_RICK_ROLL))
      display_obj.displayBuffer();


#ifdef CYD_24CAP
  int pre_getTouch = millis();
#endif

#ifdef CYD_24CAP || defined(CYD_22CAP)
  int pre_getTouchBBC = millis();
#endif

#if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    #if defined(CYD_32CAP) || defined(CYD_35CAP)
      if (!this->disable_touch)
        points = this->updateTouch(t_x, t_y);
        pressed = points && points > 0;
    #else
      if (!this->disable_touch)
      pressed = this->updateTouch(&t_x, &t_y);
    #endif
      
    #ifdef HAS_SCREEN
    if ((wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
        (pressed) &&
        (wifi_scan_obj.currentScanMode != OTA_UPDATE) &&
        (wifi_scan_obj.currentScanMode != ESP_UPDATE) &&
        (wifi_scan_obj.currentScanMode != SHOW_INFO) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_DATA) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_NMEA)) 
      {
        //Stop the current scan
        if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_EVIL_PORTAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP_FULL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PWN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ESPRESSIF) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AP_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_MIMIC) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_AIRTAG) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_FLIPPER) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_FLIPPER_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_SPOOF_AIRTAG) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE_CONT) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER)) 
        {
            wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
            display_obj.tft.init();
            changeMenu(current_menu);
            delay(100);
        }

        x = -1;
        y = -1;

        return;
        
        /*for (int i = 0; i < BUTTON_ARRAY_LEN; i++) {
          display_obj.key[i].press(false);
        }*/
      }
  #endif

  #ifdef HAS_BUTTONS
    bool c_btn_press = c_btn.justPressed();

    #if defined(HAS_SCREEN)
      if ((c_btn_press) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) &&
          (wifi_scan_obj.currentScanMode != OTA_UPDATE) &&
          (wifi_scan_obj.currentScanMode != ESP_UPDATE) &&
          (wifi_scan_obj.currentScanMode != SHOW_INFO) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_DATA) &&
          (wifi_scan_obj.currentScanMode != WIFI_SCAN_GPS_NMEA))
        {
        if ((wifi_scan_obj.currentScanMode == WIFI_SCAN_PROBE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_STATION) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_EVIL_PORTAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_TARGET_AP_FULL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_AP_STA) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PWN) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ESPRESSIF) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AP_SPAM) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_AUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_MANUAL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_DEAUTH_TARGETED) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_MIMIC) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_RICK_ROLL) ||
            (wifi_scan_obj.currentScanMode == WIFI_ATTACK_BEACON_LIST) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SOUR_APPLE) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SWIFTPAIR_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SPAM_ALL) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_SAMSUNG_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_ATTACK_GOOGLE_SPAM) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_WAR_DRIVE_CONT) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_SKIMMERS) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ACTIVE_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_ACTIVE_LIST_EAPOL) ||
            (wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
            (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
            (wifi_scan_obj.currentScanMode == BT_SCAN_ANALYZER))
          {
            wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
            display_obj.tft.init();
            changeMenu(current_menu);
            delay(100); // Brief delay to ignore residual touches after exit
          }

        x = -1;
        y = -1;

        /*for (int i = 0; i < BUTTON_ARRAY_LEN; i++) {
          display_obj.key[i].press(false);
        }*/
        return;
    }
  #endif
  
#endif

#if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    if ((wifi_scan_obj.currentScanMode != WIFI_ATTACK_BEACON_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AP_SPAM) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_AUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_MANUAL) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_DEAUTH_TARGETED) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_MIMIC) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_PACKET_RATE) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_RAW_CAPTURE) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_CHAN_ANALYZER) &&
        (wifi_scan_obj.currentScanMode != WIFI_SCAN_SIG_STREN) &&
        (wifi_scan_obj.currentScanMode != WIFI_ATTACK_RICK_ROLL))
        {
          #if !(defined(CYD_32CAP) || defined(CYD_35CAP))
          // Corner back arrow: a tap in the top-left goes up one menu
          {
            static bool _backHeld = false;
            if (pressed && t_x < 40 && t_y < 40) {
              if (!_backHeld && current_menu->parentMenu != NULL) {
                _backHeld = true;
                if (wifi_scan_obj.currentScanMode != WIFI_SCAN_OFF) {  // stop any live scan (e.g. GPS Data/NMEA) so it can't draw over the menu
                  wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
                  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
                }
                this->changeMenu(current_menu->parentMenu);
                return;
              }
              _backHeld = true;
            } else if (!pressed) {
              _backHeld = false;
            }
          }
          #endif
          #if defined(CYD_32CAP) || defined(CYD_35CAP)
          for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++) {
            bool found = false;
            if (pressed) {
                //Serial.print("Checking button "); Serial.print(b); Serial.print(" with "); Serial.print(points); Serial.println(" points");
                for (int16_t i = 0; i < points && i < 5; i++) {
                    if (display_obj.key[b].contains(t_x[i], t_y[i])) {
                        found = true;
                        //Serial.print("Button "); Serial.print(b); Serial.println(" pressed");
                        break;
                    }
                }
                display_obj.key[b].press(found);
            } else {
                display_obj.key[b].press(false);
            }
          }
          #else
          for (uint8_t b = 0; b < BUTTON_ARRAY_LEN; b++) {
            if (pressed && display_obj.key[b].contains(t_x, t_y)) {
                display_obj.key[b].press(true);
                //Serial.print("Button "); Serial.print(b); Serial.println(" pressed");
            } else {
                display_obj.key[b].press(false);
            }
          }
          #endif

          uint16_t _total = current_menu->list->size();
          uint16_t _base = (current_menu->parentMenu != NULL) ? 1 : 0;
          uint16_t _pageStart = this->menu_start_index;
          if (_pageStart < _base) _pageStart = _base;
          uint8_t _visible = (_total - _pageStart >= BUTTON_SCREEN_LIMIT) ? BUTTON_SCREEN_LIMIT : (uint8_t)(_total - _pageStart);

          // Handle Prev/Next page taps first (wrap around at the ends)
          bool _paged = false;
          if (_total - _base > BUTTON_SCREEN_LIMIT) {
            if (display_obj.key[BUTTON_SCREEN_LIMIT].justPressed()) {
              int _ns = (_pageStart - BUTTON_SCREEN_LIMIT >= _base)
                          ? (int)(_pageStart - BUTTON_SCREEN_LIMIT)
                          : (int)(_base + ((_total - _base - 1) / BUTTON_SCREEN_LIMIT) * BUTTON_SCREEN_LIMIT);
              this->buildButtons(current_menu, _ns);
              this->displayCurrentMenu(_ns);
              _paged = true;
            }
            else if (display_obj.key[BUTTON_SCREEN_LIMIT + 1].justPressed()) {
              int _ns = (_pageStart + BUTTON_SCREEN_LIMIT < _total)
                          ? (int)(_pageStart + BUTTON_SCREEN_LIMIT)
                          : (int)_base;
              this->buildButtons(current_menu, _ns);
              this->displayCurrentMenu(_ns);
              _paged = true;
            }
          }

          if (!_paged)
          for (uint8_t b = 0; b < _visible; b++) {
            uint16_t _idx = _pageStart + b;
            display_obj.tft.setFreeFont(MENU_FONT);
            if (display_obj.key[b].justPressed()) {
                drawMenuLabel(b, KEY_Y + b * (KEY_H + KEY_SPACING_Y), this->getColor(current_menu->list->get(_idx).color), current_menu->list->get(_idx).name, true); // Pressed state
                if (current_menu->list->get(_idx).name == "Color Theme") {
                    drawThemeSwatch(KEY_Y + b * (KEY_H + KEY_SPACING_Y));
                } else if (current_menu->list->get(_idx).name != text09 && current_menu->list->get(_idx).icon != 255) {
                    uint16_t icon_color = this->getColor(current_menu->list->get(_idx).color);
                    display_obj.tft.setTextColor(icon_color, TFT_BLACK); // Set color state explicitly
                    display_obj.tft.drawXBitmap(0,
                                                KEY_Y + b * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                                menu_icons[current_menu->list->get(_idx).icon],
                                                ICON_W,
                                                ICON_H,
                                                this->getColor(current_menu->list->get(_idx).color),
                                                TFT_BLACK);
                }
            }

            // If button was just release, execute the button's function
        if ((display_obj.key[b].justReleased()) && (!pressed))
        {
          drawMenuLabel(b, KEY_Y + b * (KEY_H + KEY_SPACING_Y), this->getColor(current_menu->list->get(_idx).color), current_menu->list->get(_idx).name, false);
          current_menu->list->get(_idx).callable();
        }
        // This
        else if ((display_obj.key[b].justReleased()) && (pressed)) {
          drawMenuLabel(b, KEY_Y + b * (KEY_H + KEY_SPACING_Y), this->getColor(current_menu->list->get(_idx).color), current_menu->list->get(_idx).name, false);
          if (current_menu->list->get(_idx).name == "Color Theme")
            drawThemeSwatch(KEY_Y + b * (KEY_H + KEY_SPACING_Y));
          else if (current_menu->list->get(_idx).name != text09 && current_menu->list->get(_idx).icon != 255)
            display_obj.tft.drawXBitmap(0,
                                        KEY_Y + b * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                        menu_icons[current_menu->list->get(_idx).icon],
                                        ICON_W,
                                        ICON_H,
                                        TFT_BLACK,
                                        this->getColor(current_menu->list->get(_idx).color));
        }

        display_obj.tft.setFreeFont(NULL);
      }
    }
    x = -1;
    y = -1;

    /*for (int i = 0; i < BUTTON_ARRAY_LEN; i++) {
          display_obj.key[i].press(false);
        }*/
  #endif

  // Menu navigation and paging
  #ifdef HAS_BUTTONS
    #if !(defined(MARAUDER_V6) || defined(MARAUDER_V6_1))
    #ifndef MARAUDER_M5STICKC
    if (u_btn.justPressed()) {
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF || wifi_scan_obj.currentScanMode == OTA_UPDATE) {
            if (current_menu->selected > 0) {
                current_menu->selected--;
                if (current_menu->selected < this->menu_start_index) {
                    this->buildButtons(current_menu, current_menu->selected);
                    this->displayCurrentMenu(current_menu->selected);
                }
                this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
                if (!current_menu->list->get(current_menu->selected + 1).selected)
                    this->buttonNotSelected(current_menu->selected + 1 - this->menu_start_index, current_menu->selected + 1);
            } else {
                current_menu->selected = current_menu->list->size() - 1;
                if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                    this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                    this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                }
                this->buttonSelected(current_menu->selected, current_menu->selected);
                if (!current_menu->list->get(0).selected)
                    this->buttonNotSelected(0, this->menu_start_index);
              }
            }   
            else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_RAW_CAPTURE) ||
                  (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
            if (wifi_scan_obj.set_channel < 14)
              wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel + 1);
            else
              wifi_scan_obj.changeChannel(1);
          }
        }
    #endif
    if (d_btn.justPressed()) {
        if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF || wifi_scan_obj.currentScanMode == OTA_UPDATE) {
            if (current_menu->selected < current_menu->list->size() - 1) {
                current_menu->selected++;
                // Page down
                if (current_menu->selected - this->menu_start_index >= BUTTON_SCREEN_LIMIT) {
                    this->buildButtons(current_menu, current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                    this->displayCurrentMenu(current_menu->selected + 1 - BUTTON_SCREEN_LIMIT);
                }
                else
                  this->buttonSelected(current_menu->selected - this->menu_start_index, current_menu->selected);
                if (!current_menu->list->get(current_menu->selected - 1).selected)
                  this->buttonNotSelected(current_menu->selected - 1 - this->menu_start_index, current_menu->selected - 1);
              } else {
                if (current_menu->selected >= BUTTON_SCREEN_LIMIT) {
                    current_menu->selected = 0;
                    this->buildButtons(current_menu);
                    this->displayCurrentMenu();
                    this->buttonSelected(current_menu->selected);
              } 
            else {
              current_menu->selected = 0;
              //this->buildButtons(current_menu);  // Ensure all buttons are refreshed
              //this->displayCurrentMenu();
              this->buttonSelected(current_menu->selected);
              if (!current_menu->list->get(current_menu->list->size() - 1).selected)
                this->buttonNotSelected(current_menu->list->size() - 1);
              //if (!current_menu->list->get(current_menu->list->size() - 1).selected)
              //  this->buttonNotSelected(BUTTON_SCREEN_LIMIT - 1, current_menu->list->size() - 1);
                }
            }
          } 
          else if ((wifi_scan_obj.currentScanMode == WIFI_PACKET_MONITOR) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_EAPOL) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_CHAN_ANALYZER) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_PACKET_RATE) ||
                (wifi_scan_obj.currentScanMode == WIFI_SCAN_SIG_STREN)) {
          if (wifi_scan_obj.set_channel > 1)
            wifi_scan_obj.changeChannel(wifi_scan_obj.set_channel - 1);
          else
            wifi_scan_obj.changeChannel(14);
        }
      }
      if(c_btn_press){
        current_menu->list->get(current_menu->selected).callable();
      }
    #endif
  #endif
}


#if BATTERY_ANALOG_ON == 1
byte battery_analog_array[10];
byte battery_count = 0;
byte battery_analog_last = 101;
#define BATTERY_CHECK 50
uint16_t battery_analog = 0;
void MenuFunctions::battery(bool initial)
{
  if (BATTERY_ANALOG_ON) {
    uint8_t n = 0;
    byte battery_analog_sample[10];
    byte deviation;
    if (battery_count == BATTERY_CHECK - 5)  digitalWrite(BATTERY_PIN, HIGH);
    else if (battery_count == 5) digitalWrite(BATTERY_PIN, LOW);
    if (battery_count == 0) {
      battery_analog = 0;
      for (n = 9; n > 0; n--)battery_analog_array[n] = battery_analog_array[n - 1];
      for (n = 0; n < 10; n++) {
        battery_analog_sample[n] = map((analogRead(ANALOG_PIN) * 5), 2400, 4200, 0, 100);
        if (battery_analog_sample[n] > 100) battery_analog_sample[n] = 100;
        else if (battery_analog_sample[n] < 0) battery_analog_sample[n] = 0;
        battery_analog += battery_analog_sample[n];
      }
      battery_analog = battery_analog / 10;
      for (n = 0; n < 10; n++) {
        deviation = abs(battery_analog - battery_analog_sample[n]);
        if (deviation >= 10) battery_analog_sample[n] = battery_analog;
      }
      battery_analog = 0;
      for (n = 0; n < 10; n++) battery_analog += battery_analog_sample[n];
      battery_analog = battery_analog / 10;
      battery_analog_array[0] = battery_analog;
      if (battery_analog_array[9] > 0 ) {
        battery_analog = 0;
        for (n = 0; n < 10; n++) battery_analog += battery_analog_array[n];
        battery_analog = battery_analog / 10;
      }
      battery_count ++;
    }
    else if (battery_count < BATTERY_CHECK) battery_count++;
    else if (battery_count >= BATTERY_CHECK) battery_count = 0;

    if (battery_analog_last != battery_analog) {
      battery_analog_last = battery_analog;
      MenuFunctions::battery2();
    }
  }
}
void MenuFunctions::battery2(bool initial)
{
  uint16_t the_color;
  Serial.println("battery2 called");
  if ( digitalRead(CHARGING_PIN) == 1) the_color = TFT_BLUE;
  else if (battery_analog < 20) the_color = TFT_RED;
  else if (battery_analog < 40)  the_color = TFT_YELLOW;
  else the_color = TFT_GREEN;

  display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
  display_obj.tft.fillRect(186, 0, 50, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  display_obj.tft.drawXBitmap(186,
                              0,
                              menu_icons[STATUS_BAT],
                              16,
                              16,
                              STATUSBAR_COLOR,
                              the_color);
  #ifdef HAS_ILI9341
    display_obj.tft.drawString((String)battery_analog + "%", 204, 0, 2);
  #endif

  #ifdef HAS_ST7796
    display_obj.tft.drawString((String)battery_analog + "%", 280, 0, 2);
  #endif

  #ifdef HAS_ST7789
    display_obj.tft.drawString((String)battery_analog + "%", 280, 0, 2);
  #endif
}
#else
void MenuFunctions::battery(bool initial)
{
  #ifdef HAS_BATTERY
    uint16_t the_color;
    if (battery_obj.i2c_supported)
    {
      
      // Could use int compare maybe idk
      if (((String)battery_obj.battery_level != "25") && ((String)battery_obj.battery_level != "0"))
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;

      if ((battery_obj.battery_level != battery_obj.old_level) || (initial)) {
        battery_obj.old_level = battery_obj.battery_level;
        display_obj.tft.fillRect(204, 0, SCREEN_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
      }

      display_obj.tft.setCursor(0, 1);
      if (!this->disable_touch) {
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[STATUS_BAT],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
      }
      #ifdef HAS_ILI9341
        display_obj.tft.drawString((String)battery_obj.battery_level + "%", 204, 0, 2);
      #endif

      #ifdef HAS_ST7796
        display_obj.tft.drawString((String)battery_obj.battery_level + "%", 280, 0, 2);
      #endif

      #ifdef HAS_ST7789
        display_obj.tft.drawString((String)battery_obj.battery_level + "%", 280, 0, 2);
      #endif
    }
  #endif
}
void MenuFunctions::battery2(bool initial)
{
  MenuFunctions::battery(initial);
}
#endif

void MenuFunctions::updateStatusBar()
{
  display_obj.tft.setTextSize(1);

  bool status_changed = false;
  
  #if defined(MARAUDER_MINI) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_REV_FEATHER)
    display_obj.tft.setFreeFont(NULL);
  #endif
  
  uint16_t the_color; 

  // GPS Stuff
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      if (gps_obj.getFixStatus())
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;
        
      #ifdef HAS_FULL_SCREEN
        uint8_t current_sat_count = gps_obj.getNumSats();
        
        if (current_sat_count != this->old_gps_sat_count) {
          
          display_obj.tft.fillRect(22, 0, 28, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
          this->old_gps_sat_count = current_sat_count;
        }
        
        display_obj.tft.drawXBitmap(4, 0, menu_icons[STATUS_GPS], 16, 16, STATUSBAR_COLOR, the_color);
        display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);
        display_obj.tft.drawString(gps_obj.getNumSatsString(), 22, 0, 2);
      #elif defined(HAS_SCREEN)
        display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
        display_obj.tft.drawString("GPS", 0, 0, 1);
      #endif
    }
  #endif

  // Live clock: redraw the GPS time every tick so it doesn't freeze between full
  // status-bar redraws. Only on menu/info screens (where drawStatusBar shows the
  // time, not "CH: x"), and only when the string actually changed (no flicker).
  #if defined(HAS_GPS) && defined(HAS_FULL_SCREEN)
    if (gps_obj.getGpsModuleStatus() &&
        (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF ||
         wifi_scan_obj.currentScanMode == SHOW_INFO)) {
      String _t = gps_obj.getTime(g_utcOffsetMin);
      static String _lastClock = "";
      if (_t.length() && _t != _lastClock) {
        _lastClock = _t;
        #if defined(CYD_35CAP) || defined(CYD_35)
          int _chx = 68;
        #else
          int _chx = 50;
        #endif
        display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);
        display_obj.tft.fillRect(50, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
        display_obj.tft.drawString(_t, _chx, 2, 1);
      }
    }
  #endif

  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);

  // WiFi Channel Stuff
  if ((wifi_scan_obj.set_channel != wifi_scan_obj.old_channel) || (status_changed)) {
    wifi_scan_obj.old_channel = wifi_scan_obj.set_channel;
    #if defined(MARAUDER_MINI) || defined(MARAUDER_M5STICKC) || defined(MARAUDER_REV_FEATHER)
      display_obj.tft.fillRect(43, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #else
      display_obj.tft.fillRect(50, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #endif
    #ifdef HAS_ILI9341
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.set_channel, 50, 0, 2);
    #endif

    #ifdef HAS_ST7796
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.set_channel, 68, 0, 2);
    #endif

    #ifdef HAS_ST7789
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.set_channel, 50, 0, 2);
    #endif

    #ifdef HAS_MINI_SCREEN
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.set_channel, TFT_WIDTH/4, 0, 1);
    #endif
  }

  // RAM Stuff
  wifi_scan_obj.freeRAM();
  if ((wifi_scan_obj.free_ram != wifi_scan_obj.old_free_ram) || (status_changed)) {
    wifi_scan_obj.old_free_ram = wifi_scan_obj.free_ram;
    display_obj.tft.fillRect(100, 0, 60, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
    #ifdef HAS_ILI9341
      display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", 100, 0, 2);
    #endif

    #ifdef HAS_ST7796
      display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", 130, 0, 2);
    #endif

    #ifdef HAS_ST7789
      display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", 100, 0, 2);
    #endif


    #ifdef HAS_MINI_SCREEN
      display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", TFT_WIDTH/1.75, 0, 1);
    #endif
  }

  // Draw battery info
  #ifdef HAS_ILI9341
    MenuFunctions::battery(false);
  #endif

  #ifdef HAS_ST7796
    MenuFunctions::battery(false);
    display_obj.tft.fillRect(280, 0, 16, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #endif

  #ifdef HAS_ST7789
    MenuFunctions::battery(false);
    display_obj.tft.fillRect(186, 0, 16, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #endif

  
  #if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    #ifdef HAS_BUTTONS
      if (this->disable_touch) {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_RED);
      }
    #endif
  #endif


  // Draw SD info
  #ifdef HAS_SD
    if (sd_obj.supported)
      the_color = TFT_GREEN;
    else
      the_color = TFT_RED;

    #ifdef HAS_ILI9341
      display_obj.tft.drawXBitmap(170,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif

    #ifdef HAS_ST7796
      display_obj.tft.drawXBitmap(220,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif

    #ifdef HAS_ST7789
      display_obj.tft.drawXBitmap(170,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
    display_obj.tft.drawString("SD", TFT_WIDTH - 12, 0, 1);
  #endif

  #if !(defined(CYD_32CAP) || defined(CYD_35CAP))
    // Keep the corner back arrow visible while in a submenu
    if (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF && current_menu != NULL && current_menu->parentMenu != NULL)
      display_obj.tft.fillTriangle(4, 24, 15, 18, 15, 30, TFT_CYAN);
  #endif
}

void MenuFunctions::drawStatusBar()
{
  display_obj.tft.setTextSize(1);
  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setFreeFont(NULL);
  #endif
  display_obj.tft.fillRect(0, 0, TFT_WIDTH, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);

  uint16_t the_color;

  // GPS Stuff
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      if (gps_obj.getFixStatus())
        the_color = TFT_GREEN;
      else
        the_color = TFT_RED;
        
      #ifdef HAS_FULL_SCREEN
        display_obj.tft.drawXBitmap(4,
                                    0,
                                    menu_icons[STATUS_GPS],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    the_color);
        display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);

        display_obj.tft.drawString(gps_obj.getNumSatsString(), 22, 0, 2);
      #endif
    }
  #endif

  display_obj.tft.setTextColor(TFT_WHITE, STATUSBAR_COLOR);


  // WiFi Channel / GPS time.
  // On menu & sub-menu navigation screens show the GPS time here (user prefers
  // not to see the channel there). On an operation's inner page (a live scan /
  // attack / monitor) keep showing "CH: x".
  wifi_scan_obj.old_channel = wifi_scan_obj.set_channel;
  bool _menuCtx = (wifi_scan_obj.currentScanMode == WIFI_SCAN_OFF ||
                   wifi_scan_obj.currentScanMode == SHOW_INFO);
  String _gpsTime = "";
  #ifdef HAS_GPS
    _gpsTime = gps_obj.getTime(g_utcOffsetMin);
  #endif
  #ifdef HAS_MINI_SCREEN
    display_obj.tft.fillRect(43, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #else
    display_obj.tft.fillRect(50, 0, TFT_WIDTH * 0.21, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #endif
  #ifdef HAS_FULL_SCREEN
    #if defined(CYD_35CAP) || defined(CYD_35)
      int _chx = 68;
    #else
      int _chx = 50;
    #endif
    if (_menuCtx) {
      if (_gpsTime.length())
        display_obj.tft.drawString(_gpsTime, _chx, 2, 1);   // GPS time (UTC); font 1 fits the slot
      // else: blank on menu screens (channel intentionally hidden)
    } else {
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.set_channel, _chx, 0, 2);
    }
  #endif

  #ifdef HAS_MINI_SCREEN
    if (_menuCtx) {
      if (_gpsTime.length())
        display_obj.tft.drawString(_gpsTime, TFT_WIDTH/4, 0, 1);
    } else {
      display_obj.tft.drawString("CH: " + (String)wifi_scan_obj.set_channel, TFT_WIDTH/4, 0, 1);
    }
  #endif

  // RAM Stuff
  wifi_scan_obj.freeRAM();
  wifi_scan_obj.old_free_ram = wifi_scan_obj.free_ram;
  display_obj.tft.fillRect(100, 0, 60, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
  #ifdef HAS_FULL_SCREEN
    #if defined(CYD_35CAP) || defined(CYD_35)
      display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", 130, 0, 2);
    #else
      display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", 100, 0, 2);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.drawString((String)wifi_scan_obj.free_ram + "B", TFT_WIDTH/1.75, 0, 1);
  #endif


  MenuFunctions::battery(true);


  #if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    #ifdef HAS_BUTTONS
      if (this->disable_touch) {
        display_obj.tft.setCursor(0, 1);
        display_obj.tft.drawXBitmap(186,
                                    0,
                                    menu_icons[DISABLE_TOUCH],
                                    16,
                                    16,
                                    STATUSBAR_COLOR,
                                    TFT_RED);
      }
    #endif
  #endif

  // Draw SD info
  #ifdef HAS_SD
    if (sd_obj.supported)
      the_color = TFT_GREEN;
    else
      the_color = TFT_RED;
  

    #ifdef HAS_ILI9341
      display_obj.tft.drawXBitmap(170,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif

    #ifdef HAS_ST7796
      display_obj.tft.drawXBitmap(220,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif

    #ifdef HAS_ST7789
      display_obj.tft.drawXBitmap(170,
                                  0,
                                  menu_icons[STATUS_SD],
                                  16,
                                  16,
                                  STATUSBAR_COLOR,
                                  the_color);
    #endif
  #endif

  #ifdef HAS_MINI_SCREEN
    display_obj.tft.setTextColor(the_color, STATUSBAR_COLOR);
    display_obj.tft.drawString("SD", TFT_WIDTH - 12, 0, 1);
  #endif

  // Battery level (MAX1704x fuel gauge): small battery icon + percentage at the
  // right edge of the status bar, next to the SD indicator. drawStatusBar() runs
  // on every menu / sub-menu / info screen, so this shows everywhere.
  #ifdef HAS_BATTERY
    #ifdef HAS_FULL_SCREEN
      if (battery_obj.i2c_supported && battery_obj.battery_level >= 0) {
        int lvl = battery_obj.battery_level;
        if (lvl > 100) lvl = 100;
        uint16_t bcol = (lvl < 20) ? TFT_RED : (lvl < 50) ? TFT_YELLOW : TFT_GREEN;
        display_obj.tft.fillRect(188, 0, TFT_WIDTH - 188, STATUS_BAR_WIDTH, STATUSBAR_COLOR);
        int bx = 190, by = 3, bw = 16, bh = 10;
        display_obj.tft.drawRect(bx, by, bw, bh, bcol);               // battery body
        display_obj.tft.fillRect(bx + bw, by + 3, 2, bh - 6, bcol);   // positive terminal
        int fw = ((bw - 2) * lvl) / 100;                             // proportional fill
        if (fw > 0) display_obj.tft.fillRect(bx + 1, by + 1, fw, bh - 2, bcol);
        display_obj.tft.setTextColor(bcol, STATUSBAR_COLOR);
        display_obj.tft.drawString((String)lvl + "%", bx + bw + 4, 0, 1);
      }
    #endif
  #endif
}

void MenuFunctions::orientDisplay()
{
  display_obj.tft.init();

  display_obj.tft.setRotation(0); // Portrait

  display_obj.tft.setCursor(0, 0);

  #ifdef HAS_SCREEN
    #ifdef CYD_28
      uint16_t calData[5] = { 350, 3465, 188, 3431, 2 }; // tft.setRotation(0); // Portrait with TFT Shield
    #elif defined(CYD_24)
      uint16_t calData[5] = { 481, 3053, 433, 3296, 3 }; // tft.setRotation(0); // Portrait with TFT Shield
    #elif defined(CYD_24CAP)
      uint16_t calData[5] = { 405, 3209, 297, 3314, 2 };
    #elif defined(CYD_24G)
      uint16_t calData[5] = { 405, 3209, 297, 3314, 2 }; // tft.setRotation(0); // Portrait with TFT Shield
    #elif defined(CYD_32)
      uint16_t calData[5] = { 251, 3539, 331, 3534, 6 }; // tft.setRotation(0); // Portrait with TFT Shield
    #elif defined(CYD_35)
      uint16_t calData[5] = { 309, 3465, 297, 3552, 6 };
    #elif defined(TFT_DIY)
      uint16_t calData[5] = { 339, 3470, 237, 3438, 2 }; // tft.setRotation(0); // Portrait with DIY TFT
    #endif

    #if !defined(CYD_32CAP) && !defined(CYD_35CAP)
      display_obj.tft.setTouch(calData);
    #endif
  #endif

  changeMenu(current_menu);
}

void MenuFunctions::runBoolSetting(String key) {
  display_obj.tftDrawRedOnOffButton();
}

String MenuFunctions::callSetting(String key) {
  specSettingMenu.name = key;
  
  String setting_type = settings_obj.getSettingType(key);

  if (setting_type == "bool") {
    return "bool";
  }
}

void MenuFunctions::displaySetting(String key, Menu* menu, int index) {
  specSettingMenu.name = key;

  bool setting_value = settings_obj.loadSetting<bool>(key);

  // Make a local copy of menu node
  MenuNode node = menu->list->get(index);

  display_obj.tft.setTextWrap(false);
  display_obj.tft.setFreeFont(NULL);
  display_obj.tft.setCursor(0, 100);
  display_obj.tft.setTextSize(1);

  // Set local copy value
  if (!setting_value) {
    display_obj.tft.setTextColor(TFT_RED);
    display_obj.tft.println(F(text_table1[4]));
    node.selected = false;
  }
  else {
    display_obj.tft.setTextColor(TFT_GREEN);
    display_obj.tft.println(F(text_table1[5]));
    node.selected = true;
  }

  // Put local copy back into menu
  menu->list->set(index, node);
    
}

// Function to build the menus
void MenuFunctions::RunSetup()
{
  extern LinkedList<AccessPoint>* access_points;
  extern LinkedList<Station>* stations;
  extern LinkedList<AirTag>* airtags;

  this->disable_touch = false;
  
  #if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    this->initLVGL();
  #endif
   
  // root menu stuff
  mainMenu.list = new LinkedList<MenuNode>(); // Get list in first menu ready

  // Main menu stuff
  wifiMenu.list = new LinkedList<MenuNode>(); // Get list in second menu ready
  bluetoothMenu.list = new LinkedList<MenuNode>(); // Get list in third menu ready
  deviceMenu.list = new LinkedList<MenuNode>();
  #ifdef HAS_GPS
    if (gps_obj.getGpsModuleStatus()) {
      gpsInfoMenu.list = new LinkedList<MenuNode>();
    }
  #endif

  // Device menu stuff
  failedUpdateMenu.list = new LinkedList<MenuNode>();
  whichUpdateMenu.list = new LinkedList<MenuNode>();
  confirmMenu.list = new LinkedList<MenuNode>();
  updateMenu.list = new LinkedList<MenuNode>();
  settingsMenu.list = new LinkedList<MenuNode>();
  specSettingMenu.list = new LinkedList<MenuNode>();
  infoMenu.list = new LinkedList<MenuNode>();
  languageMenu.list = new LinkedList<MenuNode>();

  // WiFi menu stuff
  wifiSnifferMenu.list = new LinkedList<MenuNode>();
  wifiAttackMenu.list = new LinkedList<MenuNode>();
  #ifdef HAS_GPS
    wardrivingMenu.list = new LinkedList<MenuNode>();
  #endif
  wifiGeneralMenu.list = new LinkedList<MenuNode>();
  wifiAPMenu.list = new LinkedList<MenuNode>();
  apInfoMenu.list = new LinkedList<MenuNode>();
  setMacMenu.list = new LinkedList<MenuNode>();
  genAPMacMenu.list = new LinkedList<MenuNode>();
  #ifdef HAS_BT
    airtagMenu.list = new LinkedList<MenuNode>();
  #endif
  #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789)
    wifiStationMenu.list = new LinkedList<MenuNode>();
  #endif

  // WiFi HTML menu stuff
  htmlMenu.list = new LinkedList<MenuNode>();
  #if (!defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789) && defined(HAS_BUTTONS))
    miniKbMenu.list = new LinkedList<MenuNode>();
  #endif
  #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789)
    #ifdef HAS_BUTTONS
      #ifdef HAS_SD
        sdDeleteMenu.list = new LinkedList<MenuNode>();
      #endif
    #endif
  #endif

  // Bluetooth menu stuff
  bluetoothSnifferMenu.list = new LinkedList<MenuNode>();
  bluetoothAttackMenu.list = new LinkedList<MenuNode>();

  // Settings stuff
  generateSSIDsMenu.list = new LinkedList<MenuNode>();
  clearSSIDsMenu.list = new LinkedList<MenuNode>();
  clearAPsMenu.list = new LinkedList<MenuNode>();
  saveFileMenu.list = new LinkedList<MenuNode>();

  saveSSIDsMenu.list = new LinkedList<MenuNode>();
  loadSSIDsMenu.list = new LinkedList<MenuNode>();
  saveAPsMenu.list = new LinkedList<MenuNode>();
  loadAPsMenu.list = new LinkedList<MenuNode>();
  saveATsMenu.list = new LinkedList<MenuNode>();
  loadATsMenu.list = new LinkedList<MenuNode>();

  // Work menu names
  mainMenu.name = text_table1[6];
  wifiMenu.name = text_table1[7];
  deviceMenu.name = text_table1[9];
  failedUpdateMenu.name = text_table1[11];
  whichUpdateMenu.name = text_table1[12];
  confirmMenu.name = text_table1[13];
  updateMenu.name = text_table1[15];
  languageMenu.name = text_table1[16]; 
  infoMenu.name = text_table1[17];
  settingsMenu.name = text_table1[18];
  bluetoothMenu.name = text_table1[19];
  wifiSnifferMenu.name = text_table1[20];
  wifiAttackMenu.name = text_table1[21];
  wifiGeneralMenu.name = text_table1[22];
  saveFileMenu.name = "Save/Load Files";
  saveSSIDsMenu.name = "Save SSIDs";
  loadSSIDsMenu.name = "Load SSIDs";
  saveAPsMenu.name = "Save APs";
  loadAPsMenu.name = "Load APs";
  saveATsMenu.name = "Save Airtags";
  loadATsMenu.name = "Load Airtags";

  bluetoothSnifferMenu.name = text_table1[23];
  bluetoothAttackMenu.name = "Bluetooth Attacks";
  generateSSIDsMenu.name = text_table1[27];
  clearSSIDsMenu.name = text_table1[28];
  clearAPsMenu.name = text_table1[29];
  wifiAPMenu.name = "Access Points";
  apInfoMenu.name = "AP Info";
  setMacMenu.name = "Set MACs";
  genAPMacMenu.name = "Generate AP MAC";
  #ifdef HAS_BT
    airtagMenu.name = "Select Airtag";
  #endif
  #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789)
    wifiStationMenu.name = "Select Stations";
  #endif
  #ifdef HAS_GPS
    gpsInfoMenu.name = "GPS Data";
    wardrivingMenu.name = "Wardriving";
  #endif  
  htmlMenu.name = "EP HTML List";
  #if (!defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789) && defined(HAS_BUTTONS))
    miniKbMenu.name = "Mini Keyboard";
  #endif
  #ifdef HAS_SD
    #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789) && !defined(HAS_ST7789)
      sdDeleteMenu.name = "Delete SD Files";
    #endif
  #endif

  // Build Main Menu
  mainMenu.parentMenu = NULL;
  this->addNodes(&mainMenu, text_table1[7], TFTGREEN, NULL, WIFI, [this]() {
    this->changeMenu(&wifiMenu);
  });
  this->addNodes(&mainMenu, text_table1[19], TFTCYAN, NULL, BLUETOOTH, [this]() {
    this->changeMenu(&bluetoothMenu);
  });
  this->addNodes(&mainMenu, "Maps", TFTYELLOW, NULL, DEVICE, [this]() {
    showMap();
    this->changeMenu(&mainMenu);
  });
  this->addNodes(&mainMenu, "SD Folders", TFTCYAN, NULL, SDCARD_ICO, [this]() {
    showSDBrowser();
    this->changeMenu(&mainMenu);
  });
  // Device = 5th item -> last on page 1
  this->addNodes(&mainMenu, text_table1[9], TFTBLUE, NULL, DEVICE_D, [this]() {
    this->changeMenu(&deviceMenu);
  });
  // Reboot last -> lands on page 2
  this->addNodes(&mainMenu, text_table1[30], TFTLIGHTGREY, NULL, REBOOT, [this]() {
    // --- Reset button: full, clean reboot that always comes back usable. ---
    // Show feedback.
    display_obj.clearScreen();
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setTextDatum(MC_DATUM);
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
    display_obj.tft.drawString("Rebooting...", TFT_WIDTH / 2, TFT_HEIGHT / 2, 2);
    display_obj.tft.setTextDatum(TL_DATUM);

    // Stop any live scan and the radios so nothing is mid-transfer at reset.
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    WiFi.disconnect(true);
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    delay(250);

    // GPIO12 (TFT MISO) is also the ESP32 flash-voltage strapping pin; hold it
    // LOW across the reset so the bootloader reads the correct 3.3 V flash.
    pinMode(12, OUTPUT);
    digitalWrite(12, LOW);
    delay(10);

    // Full SYSTEM reset via the RTC watchdog. This resets the digital core more
    // thoroughly than esp_restart()'s CPU-only reset, so the SPI/touch hardware
    // comes back clean (a plain restart left the panel reading stuck-pressed and
    // hung the splash). The bounded splash wait is the final backstop.
    rtc_wdt_protect_off();
    rtc_wdt_set_length_of_reset_signal(RTC_WDT_SYS_RESET_SIG, RTC_WDT_LENGTH_3_2us);
    rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
    rtc_wdt_set_time(RTC_WDT_STAGE0, 50);   // fire after 50 ms
    rtc_wdt_enable();
    rtc_wdt_protect_on();
    while (true) { delay(10); }             // wait for the watchdog to reset us
  });

  // Build WiFi Menu
  wifiMenu.parentMenu = &mainMenu; // Main Menu is second menu parent
  this->addNodes(&wifiMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiMenu.parentMenu);
  });
  this->addNodes(&wifiMenu, text_table1[31], TFTYELLOW, NULL, SNIFFERS, [this]() {
    this->changeMenu(&wifiSnifferMenu);
  });
  #ifdef HAS_GPS
  this->addNodes(&wifiMenu, "Wardriving", TFTGREEN, NULL, BEACON_SNIFF, [this]() {
    this->changeMenu(&wardrivingMenu);
  });
  #endif
  this->addNodes(&wifiMenu, text_table1[32], TFTRED, NULL, ATTACKS, [this]() {
    this->changeMenu(&wifiAttackMenu);
  });
  this->addNodes(&wifiMenu, text_table1[33], TFTPURPLE, NULL, GENERAL_APPS, [this]() {
    this->changeMenu(&wifiGeneralMenu);
  });

  // Build WiFi sniffer Menu
  wifiSnifferMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiSnifferMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiSnifferMenu.parentMenu);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[42], TFTCYAN, NULL, PROBE_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_PROBE, TFT_CYAN);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[43], TFTMAGENTA, NULL, BEACON_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_AP, TFT_MAGENTA);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[44], TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_DEAUTH, TFT_RED);
  });

  this->addNodes(&wifiSnifferMenu, "Packet Count", TFTORANGE, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_PACKET_RATE, TFT_ORANGE);
    wifi_scan_obj.renderPacketRate();
  });
  
  #if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    this->addNodes(&wifiSnifferMenu, text_table1[46], TFTVIOLET, NULL, EAPOL, [this]() {
      wifi_scan_obj.StartScan(WIFI_SCAN_EAPOL, TFT_VIOLET);
    });
    this->addNodes(&wifiSnifferMenu, text_table1[45], TFTBLUE, NULL, PACKET_MONITOR, [this]() {
      wifi_scan_obj.StartScan(WIFI_PACKET_MONITOR, TFT_BLUE);
    });
  #else // No touch
    this->addNodes(&wifiSnifferMenu, text_table1[46], TFTVIOLET, NULL, EAPOL, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_EAPOL, TFT_VIOLET);
    });
    this->addNodes(&wifiSnifferMenu, text_table1[45], TFTBLUE, NULL, PACKET_MONITOR, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_PACKET_MONITOR, TFT_BLUE);
    });
    /*this->addNodes(&wifiSnifferMenu, "Packet Count", TFTORANGE, NULL, PACKET_MONITOR, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_PACKET_RATE, TFT_ORANGE);
      wifi_scan_obj.renderPacketRate();
    });*/
  #endif
  this->addNodes(&wifiSnifferMenu, "Channel Analyzer", TFTCYAN, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    this->renderGraphUI(WIFI_SCAN_CHAN_ANALYZER);
    wifi_scan_obj.StartScan(WIFI_SCAN_CHAN_ANALYZER, TFT_CYAN);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[58], TFTWHITE, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_RAW_CAPTURE, TFT_WHITE);
  });
  this->addNodes(&wifiSnifferMenu, text_table1[47], TFTRED, NULL, PWNAGOTCHI, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_PWN, TFT_RED);
  });
  #if !defined(HAS_ILI9341) && !defined(HAS_ST7789) && !defined(HAS_ST7796)
    this->addNodes(&wifiSnifferMenu, text_table1[49], TFTMAGENTA, NULL, BEACON_SNIFF, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_TARGET_AP, TFT_MAGENTA);
    });
  #endif
  this->addNodes(&wifiSnifferMenu, "Scan All", TFTLIME, NULL, BEACON_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_AP_STA, 0x97e0);
  });
  #if !defined(HAS_ILI9341) && !defined(HAS_ST7789) && !defined(HAS_ST7796)
  this->addNodes(&wifiSnifferMenu, text_table1[59], TFTORANGE, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_STATION, TFT_WHITE);
  });
  #endif
  this->addNodes(&wifiSnifferMenu, "Signal Monitor", TFTCYAN, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_SIG_STREN, TFT_CYAN);
  });
  
  // Build Wardriving menu
  #ifdef HAS_GPS
  wardrivingMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wardrivingMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wardrivingMenu.parentMenu);
  });

  { // always show wardrive options (previously gated on boot-time GPS detection)
    this->addNodes(&wardrivingMenu, "Wardrive", TFTGREEN, NULL, BEACON_SNIFF, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_WAR_DRIVE, TFT_GREEN);
    });

    this->addNodes(&wardrivingMenu, "Station Wardrive", TFTORANGE, NULL, PROBE_SNIFF, [this]() {
      display_obj.clearScreen();
      this->drawStatusBar();
      wifi_scan_obj.StartScan(WIFI_SCAN_STATION_WAR_DRIVE, TFT_ORANGE);
    });
  }

  #endif
  
  // Build WiFi attack menu
  wifiAttackMenu.parentMenu = &wifiMenu; // Main Menu is second menu parent
  this->addNodes(&wifiAttackMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiAttackMenu.parentMenu);
  });
  this->addNodes(&wifiAttackMenu, text_table1[50], TFTRED, NULL, BEACON_LIST, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_LIST, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, text_table1[51], TFTORANGE, NULL, BEACON_SPAM, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_BEACON_SPAM, TFT_ORANGE);
  });
  this->addNodes(&wifiAttackMenu, text_table1[52], TFTYELLOW, NULL, RICK_ROLL, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_RICK_ROLL, TFT_YELLOW);
  });
  this->addNodes(&wifiAttackMenu, text_table1[53], TFTRED, NULL, PROBE_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_AUTH, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, "Evil Portal", TFTORANGE, NULL, BEACON_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_SCAN_EVIL_PORTAL, TFT_ORANGE);
    wifi_scan_obj.setMac();
  });
  this->addNodes(&wifiAttackMenu, text_table1[54], TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH, TFT_RED);
  });
  this->addNodes(&wifiAttackMenu, text_table1[57], TFTMAGENTA, NULL, BEACON_LIST, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_AP_SPAM, TFT_MAGENTA);
  });
  this->addNodes(&wifiAttackMenu, text_table1[62], TFTRED, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(WIFI_ATTACK_DEAUTH_TARGETED, TFT_ORANGE);
  });

  // Build WiFi General menu
  wifiGeneralMenu.parentMenu = &wifiMenu;
  this->addNodes(&wifiGeneralMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(wifiGeneralMenu.parentMenu);
  });
  this->addNodes(&wifiGeneralMenu, text_table1[27], TFTSKYBLUE, NULL, GENERATE, [this]() {
    this->changeMenu(&generateSSIDsMenu);
    wifi_scan_obj.RunGenerateSSIDs();
  });
  #if defined(HAS_ILI9341) || defined(HAS_ST7796) && !defined(CYD_35CAP) || defined(HAS_ST7789) && !defined(CYD_32CAP)
    this->addNodes(&wifiGeneralMenu, text_table1[1], TFTNAVY, NULL, KEYBOARD_ICO, [this](){
      display_obj.clearScreen(); 
      wifi_scan_obj.StartScan(LV_ADD_SSID, TFT_YELLOW); 
      addSSIDGFX();
    });
  #endif
  #if (!defined(HAS_ILI9341) && !defined(HAS_ST7796) && defined(HAS_BUTTONS))
    this->addNodes(&wifiGeneralMenu, text_table1[1], TFTNAVY, NULL, KEYBOARD_ICO, [this](){
      this->changeMenu(&miniKbMenu);
      this->miniKeyboard(&miniKbMenu);
    });
  #endif
  this->addNodes(&wifiGeneralMenu, text_table1[28], TFTSILVER, NULL, CLEAR_ICO, [this]() {
    this->changeMenu(&clearSSIDsMenu);
    wifi_scan_obj.RunClearSSIDs();
  });
  this->addNodes(&wifiGeneralMenu, text_table1[29], TFTDARKGREY, NULL, CLEAR_ICO, [this]() {
    this->changeMenu(&clearAPsMenu);
    wifi_scan_obj.RunClearAPs();
  });
  this->addNodes(&wifiGeneralMenu, text_table1[60], TFTBLUE, NULL, CLEAR_ICO, [this]() {
    this->changeMenu(&clearAPsMenu);
    wifi_scan_obj.RunClearStations();
  });
  #if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    // Select APs on OG
    this->addNodes(&wifiGeneralMenu, "Select APs", TFTNAVY, NULL, KEYBOARD_ICO, [this](){
      // Add the back button
      display_obj.clearScreen(); 
      wifi_scan_obj.currentScanMode = LV_ADD_SSID; 
      wifi_scan_obj.StartScan(LV_ADD_SSID, TFT_RED);  
      addAPGFX();
    });
    // Select Stations on OG
    this->addNodes(&wifiGeneralMenu, text_table1[61], TFTLIGHTGREY, NULL, KEYBOARD_ICO, [this](){
      display_obj.clearScreen(); 
      wifi_scan_obj.currentScanMode = LV_ADD_SSID; 
      wifi_scan_obj.StartScan(LV_ADD_SSID, TFT_RED);  
      addStationGFX();
    });
    // Select Evil Portal Files on OG
    this->addNodes(&wifiGeneralMenu, "Select EP HTML File", TFTCYAN, NULL, KEYBOARD_ICO, [this](){
      display_obj.clearScreen(); 
      wifi_scan_obj.currentScanMode = LV_ADD_SSID; 
      wifi_scan_obj.StartScan(LV_ADD_SSID, TFT_RED);  
      selectEPHTMLGFX();
    });
    apInfoMenu.parentMenu = &wifiGeneralMenu;
    this->addNodes(&apInfoMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(apInfoMenu.parentMenu);
    });
  #else // Mini EP HTML select
    this->addNodes(&wifiGeneralMenu, "Select EP HTML File", TFTCYAN, NULL, KEYBOARD_ICO, [this](){
      // Add the back button
      htmlMenu.list->clear();
        this->addNodes(&htmlMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(htmlMenu.parentMenu);
      });

      // Populate the menu with buttons
      for (int i = 0; i < evil_portal_obj.html_files->size(); i++) {
        // This is the menu node
        this->addNodes(&htmlMenu, evil_portal_obj.html_files->get(i), TFTCYAN, NULL, 255, [this, i](){
          evil_portal_obj.selected_html_index = i;
          evil_portal_obj.target_html_name = evil_portal_obj.html_files->get(evil_portal_obj.selected_html_index);
          Serial.println("Set Evil Portal HTML as " + evil_portal_obj.target_html_name);
          evil_portal_obj.using_serial_html = false;
          this->changeMenu(htmlMenu.parentMenu);
          return;
        });
      }
      this->changeMenu(&htmlMenu);
    });

    #if (!defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789) && defined(HAS_BUTTONS))
      miniKbMenu.parentMenu = &wifiGeneralMenu;
      this->addNodes(&miniKbMenu, "a", TFTCYAN, NULL, 0, [this]() {
        this->changeMenu(miniKbMenu.parentMenu);
      });
    #endif

    htmlMenu.parentMenu = &wifiGeneralMenu;
    this->addNodes(&htmlMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(htmlMenu.parentMenu);
    });

    // Select APs on Mini
    this->addNodes(&wifiGeneralMenu, text_table1[56], TFTNAVY, NULL, KEYBOARD_ICO, [this](){
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
        AccessPoint new_ap = access_points->get(i);
        new_ap.selected = !access_points->get(i).selected;

        // Change selection status of menu node
        MenuNode new_node = current_menu->list->get(i + 1);
        new_node.selected = !current_menu->list->get(i + 1).selected;
        current_menu->list->set(i + 1, new_node);

        access_points->set(i, new_ap);
        }, access_points->get(i).selected);
      }
      this->changeMenu(&wifiAPMenu);
    });

    this->addNodes(&wifiGeneralMenu, "View AP Info", TFTCYAN, NULL, KEYBOARD_ICO, [this](){
      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){
          this->changeMenu(&apInfoMenu);
          wifi_scan_obj.RunAPInfo(i);
        });
      }
      this->changeMenu(&wifiAPMenu);
    });

    apInfoMenu.parentMenu = &wifiAPMenu;
    this->addNodes(&apInfoMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(apInfoMenu.parentMenu);
    });
    
    wifiAPMenu.parentMenu = &wifiGeneralMenu;
    this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wifiAPMenu.parentMenu);
    });


    // Select Stations on Mini v2
    this->addNodes(&wifiGeneralMenu, "Select Stations", TFTCYAN, NULL, KEYBOARD_ICO, [this](){
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu);
      });

      int menu_limit = access_points->size();

      /*if (access_points->size() <= BUTTON_ARRAY_LEN)
        menu_limit = access_points->size();
      else
        menu_limit = BUTTON_ARRAY_LEN;*/

      for (int i = 0; i < menu_limit; i++) {
        wifiStationMenu.list->clear();
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTCYAN, NULL, 255, [this, i](){

          wifiStationMenu.list->clear();

          // Add back button to the APs
          this->addNodes(&wifiStationMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
            this->changeMenu(wifiStationMenu.parentMenu);
          });

          // Add the AP's stations to the specific AP menu
          for (int x = 0; x < access_points->get(i).stations->size(); x++) {
            int cur_ap_sta = access_points->get(i).stations->get(x);

            this->addNodes(&wifiStationMenu, macToString(stations->get(cur_ap_sta)), TFTCYAN, NULL, 255, [this, i, cur_ap_sta, x](){
            Station new_sta = stations->get(cur_ap_sta);
            new_sta.selected = !stations->get(cur_ap_sta).selected;

            // Change selection status of menu node
            MenuNode new_node = current_menu->list->get(x + 1);
            new_node.selected = !current_menu->list->get(x + 1).selected;
            current_menu->list->set(x + 1, new_node);

            // Change selection status of button key
            //if (new_sta.selected) {
            //  this->buttonSelected(i + 1);
            //} else {
            //  this->buttonNotSelected(i + 1);
            //}

            stations->set(cur_ap_sta, new_sta);
            }, stations->get(cur_ap_sta).selected);
          }

          // Final change menu to the menu of Stations
          this->changeMenu(&wifiStationMenu);
          
        }, false);
      }
      this->changeMenu(&wifiAPMenu);
    });

    wifiStationMenu.parentMenu = &wifiAPMenu;
    this->addNodes(&wifiStationMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(wifiStationMenu.parentMenu);
    });
  #endif

  #if defined(HAS_ILI9341) || defined(HAS_ST7789) || defined(HAS_ST7796)
    this->addNodes(&wifiGeneralMenu, "View AP Info", TFTLIGHTGREY, NULL, 0, [this]() {
      display_obj.clearScreen();
      wifi_scan_obj.currentScanMode = LV_ADD_SSID;
      wifi_scan_obj.StartScan(LV_ADD_SSID, TFT_WHITE);
      addAPGFX("AP Info");
    });
  #endif

  this->addNodes(&wifiGeneralMenu, "Set MACs", TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(&setMacMenu);
  });


  // Menu for generating and setting MAC addrs for AP and STA
  setMacMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&setMacMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(setMacMenu.parentMenu);
  });

  // Generate random MAC for AP
  this->addNodes(&setMacMenu, "Generate AP MAC", TFTLIME, NULL, 0, [this]() {
    this->changeMenu(&genAPMacMenu);
    wifi_scan_obj.RunGenerateRandomMac(true);
  });

  // Generate random MAC for AP
  this->addNodes(&setMacMenu, "Generate STA MAC", TFTCYAN, NULL, 0, [this]() {
    this->changeMenu(&genAPMacMenu);
    wifi_scan_obj.RunGenerateRandomMac(false);
  });

  // Clone AP MAC to ESP32 for button folks
  #if !defined(HAS_ILI9341) && !defined(HAS_ST7789) && !defined(HAS_ST7789)
    this->addNodes(&setMacMenu, "Clone AP MAC", TFTRED, NULL, CLEAR_ICO, [this](){
      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu);
      });

      // Populate the menu with buttons
      for (int i = 0; i < access_points->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, access_points->get(i).essid, TFTLIME, NULL, 255, [this, i](){
          this->changeMenu(&genAPMacMenu);
          wifi_scan_obj.RunSetMac(access_points->get(i).bssid, true);
        });
      }
      this->changeMenu(&wifiAPMenu);
    });

    this->addNodes(&setMacMenu, "Clone STA MAC", TFTMAGENTA, NULL, CLEAR_ICO, [this](){
      // Add the back button
      wifiAPMenu.list->clear();
        this->addNodes(&wifiAPMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(wifiAPMenu.parentMenu);
      });

      // Populate the menu with buttons
      for (int i = 0; i < stations->size(); i++) {
        // This is the menu node
        this->addNodes(&wifiAPMenu, macToString(stations->get(i).mac), TFTMAGENTA, NULL, 255, [this, i](){
          this->changeMenu(&genAPMacMenu);
          wifi_scan_obj.RunSetMac(stations->get(i).mac, false);
        });
      }
      this->changeMenu(&wifiAPMenu);
    });
  #endif

  // Menu for generating and setting access point MAC (just goes bacK)
  genAPMacMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&genAPMacMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(genAPMacMenu.parentMenu);
  });
  
  // Build generate ssids menu
  generateSSIDsMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&generateSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(generateSSIDsMenu.parentMenu);
  });

  // Build clear ssids menu
  clearSSIDsMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&clearSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(clearSSIDsMenu.parentMenu);
  });
  clearAPsMenu.parentMenu = &wifiGeneralMenu;
  this->addNodes(&clearAPsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(clearAPsMenu.parentMenu);
  });

  // Build Bluetooth Menu
  bluetoothMenu.parentMenu = &mainMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(bluetoothMenu.parentMenu);
  });
  this->addNodes(&bluetoothMenu, text_table1[31], TFTYELLOW, NULL, SNIFFERS, [this]() {
    this->changeMenu(&bluetoothSnifferMenu);
  });
  this->addNodes(&bluetoothMenu, "Bluetooth Attacks", TFTRED, NULL, ATTACKS, [this]() {
    this->changeMenu(&bluetoothAttackMenu);
  });

  // Build bluetooth sniffer Menu
  bluetoothSnifferMenu.parentMenu = &bluetoothMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothSnifferMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(bluetoothSnifferMenu.parentMenu);
  });
  this->addNodes(&bluetoothSnifferMenu, text_table1[34], TFTGREEN, NULL, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_ALL, TFT_GREEN);
  });
  this->addNodes(&bluetoothSnifferMenu, "Flipper Sniff", TFTORANGE, NULL, FLIPPER, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_FLIPPER, TFT_ORANGE);
  });
  this->addNodes(&bluetoothSnifferMenu, "Airtag Sniff", TFTWHITE, NULL, BLUETOOTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_AIRTAG, TFT_WHITE);
  });
  #ifdef HAS_GPS
    if (true) {   // always show (no hidden menus, even without a GPS module)
      this->addNodes(&bluetoothSnifferMenu, "BT Wardrive", TFTCYAN, NULL, BLUETOOTH_SNIFF, [this]() {
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan(BT_SCAN_WAR_DRIVE, TFT_GREEN);
      });
      this->addNodes(&bluetoothSnifferMenu, "BT Wardrive Continuous", TFTRED, NULL, REBOOT, [this]() {
        display_obj.clearScreen();
        this->drawStatusBar();
        wifi_scan_obj.StartScan(BT_SCAN_WAR_DRIVE_CONT, TFT_GREEN);
      });
    }
  #endif
  this->addNodes(&bluetoothSnifferMenu, text_table1[35], TFTMAGENTA, NULL, CC_SKIMMERS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_SCAN_SKIMMERS, TFT_MAGENTA);
  });
  this->addNodes(&bluetoothSnifferMenu, "Bluetooth Analyzer", TFTCYAN, NULL, PACKET_MONITOR, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    this->renderGraphUI(BT_SCAN_ANALYZER);
    wifi_scan_obj.StartScan(BT_SCAN_ANALYZER, TFT_CYAN);
  });

  // Bluetooth Attack menu
  bluetoothAttackMenu.parentMenu = &bluetoothMenu; // Second Menu is third menu parent
  this->addNodes(&bluetoothAttackMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(bluetoothAttackMenu.parentMenu);
  });
  this->addNodes(&bluetoothAttackMenu, "Sour Apple", TFTGREEN, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SOUR_APPLE, TFT_GREEN);
  });
  this->addNodes(&bluetoothAttackMenu, "Swiftpair Spam", TFTCYAN, NULL, KEYBOARD_ICO, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SWIFTPAIR_SPAM, TFT_CYAN);
  });
  this->addNodes(&bluetoothAttackMenu, "Samsung BLE Spam", TFTRED, NULL, GENERAL_APPS, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SAMSUNG_SPAM, TFT_RED);
  });
  this->addNodes(&bluetoothAttackMenu, "Google BLE Spam", TFTPURPLE, NULL, GOOGLE_G, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_GOOGLE_SPAM, TFT_PURPLE);
  });
  this->addNodes(&bluetoothAttackMenu, "Flipper BLE Spam", TFTORANGE, NULL, FLIPPER, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_FLIPPER_SPAM, TFT_ORANGE);
  });
  this->addNodes(&bluetoothAttackMenu, "BLE Spam All", TFTMAGENTA, NULL, DEAUTH_SNIFF, [this]() {
    display_obj.clearScreen();
    this->drawStatusBar();
    wifi_scan_obj.StartScan(BT_ATTACK_SPAM_ALL, TFT_MAGENTA);
  });

  #if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    this->addNodes(&bluetoothAttackMenu, "Spoof Airtag", TFTWHITE, NULL, ATTACKS, [this](){
      display_obj.clearScreen();
      wifi_scan_obj.currentScanMode = LV_ADD_SSID;
      wifi_scan_obj.StartScan(LV_ADD_SSID, TFT_WHITE);
      addAPGFX("Airtag");
    });
  #endif

  #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789)
    #ifdef HAS_BT
    // Select Airtag on Mini
      this->addNodes(&bluetoothAttackMenu, "Spoof Airtag", TFTWHITE, NULL, ATTACKS, [this](){
          // Clear nodes and add back button
          airtagMenu.list->clear();
          this->addNodes(&airtagMenu, text09, TFT_LIGHTGREY, NULL, 0, [this]() {
          this->changeMenu(airtagMenu.parentMenu);
        });

        // Add buttons for all airtags
        // Find out how big our menu is going to be
        int menu_limit;
        if (airtags->size() <= BUTTON_ARRAY_LEN)
          menu_limit = airtags->size();
        else
          menu_limit = BUTTON_ARRAY_LEN;

        // Create the menu nodes for all of the list items
        for (int i = 0; i < menu_limit; i++) {
          this->addNodes(&airtagMenu, airtags->get(i).mac, TFTWHITE, NULL, BLUETOOTH, [this, i](){
            AirTag new_at = airtags->get(i);
            new_at.selected = true;

            airtags->set(i, new_at);

            // Set all other airtags to "Not Selected"
            for (int x = 0; x < airtags->size(); x++) {
              if (x != i) {
                AirTag new_atx = airtags->get(x);
                new_atx.selected = false;
                airtags->set(x, new_atx);
              }
            }

            // Start the spoof
            display_obj.clearScreen();
            this->drawStatusBar();
            wifi_scan_obj.StartScan(BT_SPOOF_AIRTAG, TFT_WHITE);

          });
        }
        this->changeMenu(&airtagMenu);
      });

      airtagMenu.parentMenu = &bluetoothAttackMenu;
      this->addNodes(&airtagMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        this->changeMenu(airtagMenu.parentMenu);
      });
    #endif

  #endif

  // Device menu
  deviceMenu.parentMenu = &mainMenu;
  this->addNodes(&deviceMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(deviceMenu.parentMenu);
  });
  this->addNodes(&deviceMenu, text_table1[15], TFTORANGE, NULL, UPDATE, [this]() {
    wifi_scan_obj.currentScanMode = OTA_UPDATE;
    this->changeMenu(&whichUpdateMenu);
  });

  this->addNodes(&deviceMenu, "Save/Load Files", TFTCYAN, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveFileMenu);
  });

  this->addNodes(&deviceMenu, text_table1[16], TFTGREEN, NULL, LANGUAGE, [this]() {

    wifi_scan_obj.currentScanMode = SHOW_INFO;
    this->changeMenu(&languageMenu);
  });
  this->addNodes(&deviceMenu, "Color Theme", TFTMAGENTA, NULL, LANGUAGE, [this]() {
    showColorTheme();
    this->changeMenu(&deviceMenu);
  });
  this->addNodes(&deviceMenu, text_table1[17], TFTWHITE, NULL, DEVICE_INFO, [this]() {
    wifi_scan_obj.currentScanMode = SHOW_INFO;
    this->changeMenu(&infoMenu);
    wifi_scan_obj.RunInfo();
  });
  this->addNodes(&deviceMenu, "GPS Settings", TFTGREEN, NULL, GPS_MENU, [this]() {
    showGpsSettings();
    this->changeMenu(&deviceMenu);
  });
  this->addNodes(&deviceMenu, "Fuel Gauge", TFTYELLOW, NULL, DEVICE_INFO, [this]() {
    showFuelGauge();
    this->changeMenu(&deviceMenu);
  });
  this->addNodes(&deviceMenu, "UTC Offset", TFTGREEN, NULL, GPS_MENU, [this]() {
    showUtcSetting();
    this->changeMenu(&deviceMenu);
  });
  this->addNodes(&deviceMenu, text08, TFTNAVY, NULL, KEYBOARD_ICO, [this]() {
    this->changeMenu(&settingsMenu);
  });

  #ifdef HAS_SD
    if (true) {   // always show (no hidden menus, even without an SD card)
      this->addNodes(&deviceMenu, "Delete SD Files", TFTCYAN, NULL, SD_UPDATE, [this]() {
        #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789)
          #ifdef HAS_BUTTONS
            this->changeMenu(&sdDeleteMenu);
            #if !(defined(MARAUDER_V6) || defined(MARAUDER_V6_1))

              bool deleting = true;

              display_obj.tft.setTextWrap(false);
              display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
              display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
              display_obj.tft.println("Loading...");

              while (deleting) {
                // Build list of files
                sd_obj.sd_files->clear();
                delete sd_obj.sd_files;

                sd_obj.sd_files = new LinkedList<String>();

                sd_obj.sd_files->add("Back");

                sd_obj.listDirToLinkedList(sd_obj.sd_files);

                int sd_file_index = 0;

                this->sdDeleteMenu.list->set(0, MenuNode{sd_obj.sd_files->get(sd_file_index), false, TFTCYAN, 0, NULL, true, NULL});
                this->buildButtons(&sdDeleteMenu);
                this->displayCurrentMenu();

                // Start button loop
                while(true) {
                  #ifndef MARAUDER_M5STICKC
                    if (u_btn.justPressed()) {
                      if (sd_file_index > 0)
                        sd_file_index--;
                      else
                        sd_file_index = sd_obj.sd_files->size() - 1;

                      this->sdDeleteMenu.list->set(0, MenuNode{sd_obj.sd_files->get(sd_file_index), false, TFTCYAN, 0, NULL, true, NULL});
                      this->buildButtons(&sdDeleteMenu);
                      this->displayCurrentMenu();
                    }
                  #endif
                  if (d_btn.justPressed()) {
                    if (sd_file_index < sd_obj.sd_files->size() - 1)
                      sd_file_index++;
                    else
                      sd_file_index = 0;

                    this->sdDeleteMenu.list->set(0, MenuNode{sd_obj.sd_files->get(sd_file_index), false, TFTCYAN, 0, NULL, true, NULL});
                    this->buildButtons(&sdDeleteMenu, 0, sd_obj.sd_files->get(sd_file_index));
                    this->displayCurrentMenu();
                  }
                  if (c_btn.justPressed()) {
                    if (sd_obj.sd_files->get(sd_file_index) != "Back") {
                      if (sd_obj.removeFile("/" + sd_obj.sd_files->get(sd_file_index)))
                        Serial.println("Successfully Removed File: /" + sd_obj.sd_files->get(sd_file_index));
                        display_obj.tft.setTextWrap(false);
                        display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
                        display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
                        display_obj.tft.println("Deleting /" + sd_obj.sd_files->get(sd_file_index) + "...");
                    }
                    else {
                      this->changeMenu(sdDeleteMenu.parentMenu);
                      deleting = false;
                    }
                    break;
                  }
                }
              }
            #endif
          #endif
        #endif
      });
    }
  #endif

  #ifdef HAS_SD
    #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789)
      #ifdef HAS_BUTTONS
        sdDeleteMenu.parentMenu = &deviceMenu;
        this->addNodes(&sdDeleteMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
          this->changeMenu(sdDeleteMenu.parentMenu);
        });
      #endif
    #endif
  #endif

  // Save Files Menu
  saveFileMenu.parentMenu = &deviceMenu;
  this->addNodes(&saveFileMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveFileMenu.parentMenu);
  });
  this->addNodes(&saveFileMenu, "Save SSIDs", TFTCYAN, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveSSIDsMenu);
    wifi_scan_obj.RunSaveSSIDList(true);
  });
  this->addNodes(&saveFileMenu, "Load SSIDs", TFTSKYBLUE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&loadSSIDsMenu);
    wifi_scan_obj.RunLoadSSIDList();
  });
  this->addNodes(&saveFileMenu, "Save APs", TFTNAVY, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveAPsMenu);
    wifi_scan_obj.RunSaveAPList();
  });
  this->addNodes(&saveFileMenu, "Load APs", TFTBLUE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&loadAPsMenu);
    wifi_scan_obj.RunLoadAPList();
  });
  this->addNodes(&saveFileMenu, "Save Airtags", TFTWHITE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&saveAPsMenu);
    wifi_scan_obj.RunSaveATList();
  });
  this->addNodes(&saveFileMenu, "Load Airtags", TFTWHITE, NULL, SD_UPDATE, [this]() {
    this->changeMenu(&loadAPsMenu);
    wifi_scan_obj.RunLoadATList();
  });

  saveSSIDsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveSSIDsMenu.parentMenu);
  });

  loadSSIDsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadSSIDsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(loadSSIDsMenu.parentMenu);
  });

  saveAPsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveAPsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveAPsMenu.parentMenu);
  });

  loadAPsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadAPsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(loadAPsMenu.parentMenu);
  });

  saveATsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&saveATsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(saveATsMenu.parentMenu);
  });

  loadATsMenu.parentMenu = &saveFileMenu;
  this->addNodes(&loadATsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(loadATsMenu.parentMenu);
  });

  // GPS Menu
  #ifdef HAS_GPS
    if (true) {   // always show (no hidden menus, even without a GPS module)
      this->addNodes(&deviceMenu, "GPS Data", TFTRED, NULL, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_GPS_DATA;
        this->changeMenu(&gpsInfoMenu);
        wifi_scan_obj.StartScan(WIFI_SCAN_GPS_DATA, TFT_CYAN);
      });

      this->addNodes(&deviceMenu, "NMEA Stream", TFTORANGE, NULL, GPS_MENU, [this]() {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_GPS_NMEA;
        this->changeMenu(&gpsInfoMenu);
        wifi_scan_obj.StartScan(WIFI_SCAN_GPS_NMEA, TFT_ORANGE);
      });

      // GPS Info Menu
      gpsInfoMenu.parentMenu = &deviceMenu;
      this->addNodes(&gpsInfoMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
        wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
        wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
        this->changeMenu(gpsInfoMenu.parentMenu);
      }); 
    }
  #endif

  // Settings menu
  settingsMenu.parentMenu = &deviceMenu;
  this->addNodes(&settingsMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    changeMenu(settingsMenu.parentMenu);
  });
  for (int i = 0; i < settings_obj.getNumberSettings(); i++) {
    if (this->callSetting(settings_obj.setting_index_to_name(i)) == "bool") {
      String snm = settings_obj.setting_index_to_name(i);
      bool on = settings_obj.loadSetting<bool>(snm);
      // Each bool setting is a toggle BUTTON (not a sub-menu). The label shows
      // its state and the colour reflects it (green = enabled, red = disabled).
      this->addNodes(&settingsMenu, snm + (on ? ": ENABLED" : ": DISABLED"),
                     on ? TFTGREEN : TFTRED, NULL, 0, [this, i]() {
        String nm = settings_obj.setting_index_to_name(i);
        bool nv = settings_obj.toggleSetting(nm);        // flip and get new value
        MenuNode node = settingsMenu.list->get(i + 1);   // index 0 is the Back node
        node.name = nm + (nv ? ": ENABLED" : ": DISABLED");
        node.color = nv ? TFTGREEN : TFTRED;
        node.selected = nv;
        settingsMenu.list->set(i + 1, node);
        this->changeMenu(&settingsMenu);                 // redraw in place; stay here
      }, on);
    }
  }

  // Specific setting menu
  specSettingMenu.parentMenu = &settingsMenu;
  addNodes(&specSettingMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    this->changeMenu(specSettingMenu.parentMenu);
  });

  // Select update
  whichUpdateMenu.parentMenu = &deviceMenu;
  this->addNodes(&whichUpdateMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    this->changeMenu(whichUpdateMenu.parentMenu);
  });
  #ifdef HAS_SD
    if (sd_obj.supported) addNodes(&whichUpdateMenu, text_table1[40], TFTMAGENTA, NULL, SD_UPDATE, [this]() {
      wifi_scan_obj.currentScanMode = OTA_UPDATE;
      this->changeMenu(&confirmMenu);
    });
    // Wi-Fi update: join a saved network (/wifi_creds) and flash the latest GitHub release.
    if (sd_obj.supported) addNodes(&whichUpdateMenu, "WiFi Update", TFTCYAN, NULL, WIFI, [this]() {
      wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
      requestWifiUpdate();                     // flags + reboots; OTA runs early in setup() (clean heap)
      this->changeMenu(&whichUpdateMenu);      // returned = it failed/was skipped
    });

    // Confirm SD update menu
    confirmMenu.parentMenu = &whichUpdateMenu;
    this->addNodes(&confirmMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      this->changeMenu(confirmMenu.parentMenu);
    });
    this->addNodes(&confirmMenu, text14, TFTORANGE, NULL, UPDATE, [this]() {
      wifi_scan_obj.currentScanMode = OTA_UPDATE;
      this->changeMenu(&failedUpdateMenu);
      sd_obj.runUpdate();
    });
  #endif

  // Web Update
  updateMenu.parentMenu = &deviceMenu;

  // Failed update menu
  failedUpdateMenu.parentMenu = &whichUpdateMenu;
  this->addNodes(&failedUpdateMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    this->changeMenu(failedUpdateMenu.parentMenu);
  });

  // Device info menu
  infoMenu.parentMenu = &deviceMenu;
  this->addNodes(&infoMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
    wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
    this->changeMenu(infoMenu.parentMenu);
  });
  //language info menu
  languageMenu.parentMenu = &deviceMenu;
    this->addNodes(&languageMenu, text09, TFTLIGHTGREY, NULL, 0, [this]() {
      wifi_scan_obj.currentScanMode = WIFI_SCAN_OFF;
      this->changeMenu(infoMenu.parentMenu);
    });
    // Language entries (labels only - this fork has no translation engine, so these do not change the UI text)
    this->addNodes(&languageMenu, "English",    TFTWHITE, NULL, 255, [this]() { saveLanguage(0); this->changeMenu(languageMenu.parentMenu); });
    this->addNodes(&languageMenu, "Japanese",   TFTWHITE, NULL, 255, [this]() { saveLanguage(1); this->changeMenu(languageMenu.parentMenu); });
    this->addNodes(&languageMenu, "German",     TFTWHITE, NULL, 255, [this]() { saveLanguage(2); this->changeMenu(languageMenu.parentMenu); });
    this->addNodes(&languageMenu, "French",     TFTWHITE, NULL, 255, [this]() { saveLanguage(3); this->changeMenu(languageMenu.parentMenu); });
    this->addNodes(&languageMenu, "Italian",    TFTWHITE, NULL, 255, [this]() { saveLanguage(4); this->changeMenu(languageMenu.parentMenu); });
    this->addNodes(&languageMenu, "Portuguese", TFTWHITE, NULL, 255, [this]() { saveLanguage(5); this->changeMenu(languageMenu.parentMenu); });
    this->addNodes(&languageMenu, "Turkish",    TFTWHITE, NULL, 255, [this]() { saveLanguage(6); this->changeMenu(languageMenu.parentMenu); });
  // Set the current menu to the mainMenu
  loadLanguage();
  loadTheme();
  loadUtcCfg();
  this->changeMenu(&mainMenu);

  this->initTime = millis();
}

#if (!defined(HAS_ILI9341) && !defined(HAS_ST7796)  && !defined(HAS_ST7789) && defined(HAS_BUTTONS))
  void MenuFunctions::miniKeyboard(Menu * targetMenu) {
    // Prepare a char array and reset temp SSID string
    extern LinkedList<ssid>* ssids;

    bool pressed = true;

    wifi_scan_obj.current_mini_kb_ssid = "";

    if (c_btn.isHeld()) {
      while (!c_btn.justReleased())
        delay(1);
    }

    int str_len = wifi_scan_obj.alfa.length() + 1; 

    char char_array[str_len];

    wifi_scan_obj.alfa.toCharArray(char_array, str_len);

    // Button loop until hold center button
    #ifdef HAS_BUTTONS
      #if !(defined(MARAUDER_V6) || defined(MARAUDER_V6_1))
        while(true) {
          // Cycle char previous
          #ifdef HAS_L
            if (l_btn.justPressed()) {
              pressed = true;
              if (this->mini_kb_index > 0)
                this->mini_kb_index--;
              else
                this->mini_kb_index = str_len - 2;

              targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
              this->buildButtons(targetMenu);
              while (!l_btn.justReleased())
                delay(1);
            }
          #endif

          // Cycle char next
          #ifdef HAS_R
            if (r_btn.justPressed()) {
              pressed = true;
              if (this->mini_kb_index < str_len - 2)
                this->mini_kb_index++;
              else
                this->mini_kb_index = 0;

              targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
              this->buildButtons(targetMenu, 0, String(char_array[this->mini_kb_index]).c_str());
              while (!r_btn.justReleased())
                delay(1);
            }
          #endif

          //// 5-WAY SWITCH STUFF
          // Add character
          #if (defined(HAS_D) && defined(HAS_R))
            if (d_btn.justPressed()) {
              pressed = true;
              wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
              while (!d_btn.justReleased())
                delay(1);
            }
          #endif

          // Remove character
          #if (defined(HAS_U) && defined(HAS_L))
            if (u_btn.justPressed()) {
              pressed = true;
              wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
              while (!u_btn.justReleased())
                delay(1);
            }
          #endif

          //// PARTIAL SWITCH STUFF
          // Advance char or add char
          #if (defined(HAS_D) && !defined(HAS_R))
            if (d_btn.justPressed()) {
              bool was_held = false;
              pressed = true;
              while(!d_btn.justReleased()) {
                d_btn.justPressed();

                // Add letter to string
                if (d_btn.isHeld()) {
                  wifi_scan_obj.current_mini_kb_ssid.concat(String(char_array[this->mini_kb_index]).c_str());
                  was_held = true;
                  break;
                }
              }
              if (!was_held) {
                if (this->mini_kb_index < str_len - 2)
                  this->mini_kb_index++;
                else
                  this->mini_kb_index = 0;

                targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                this->buildButtons(targetMenu, 0, String(char_array[this->mini_kb_index]).c_str());
              }
            }
          #endif

          // Prev char or remove char
          #if (defined(HAS_U) && !defined(HAS_L))
            if (u_btn.justPressed()) {
              bool was_held = false;
              pressed = true;
              while(!u_btn.justReleased()) {
                u_btn.justPressed();

                // Remove letter from string
                if (u_btn.isHeld()) {
                  wifi_scan_obj.current_mini_kb_ssid.remove(wifi_scan_obj.current_mini_kb_ssid.length() - 1);
                  was_held = true;
                  break;
                }
              }
              if (!was_held) {
                if (this->mini_kb_index > 0)
                  this->mini_kb_index--;
                else
                  this->mini_kb_index = str_len - 2;

                targetMenu->list->set(0, MenuNode{String(char_array[this->mini_kb_index]).c_str(), false, TFTCYAN, 0, NULL, true, NULL});
                this->buildButtons(targetMenu);
              }
            }
          #endif

          // Add SSID
          #ifdef HAS_C
            if (c_btn.justPressed()) {
              while (!c_btn.justReleased()) {
                c_btn.justPressed(); // Need to continue updating button hold status. My shitty library.

                // Exit
                if (c_btn.isHeld()) {
                  this->changeMenu(targetMenu->parentMenu);
                  return;
                }
                delay(1);
              }
              // If we have a string, add it to list of SSIDs
              if (wifi_scan_obj.current_mini_kb_ssid != "") {
                pressed = true;
                ssid s = {wifi_scan_obj.current_mini_kb_ssid, random(1, 12), {random(256), random(256), random(256), random(256), random(256), random(256)}, false};
                ssids->unshift(s);
                wifi_scan_obj.current_mini_kb_ssid = "";
              }
            }
          #endif

          // Display info on screen
          if (pressed) {
            this->displayCurrentMenu();
            display_obj.tft.setTextWrap(false);
            display_obj.tft.fillRect(0, SCREEN_HEIGHT / 3, SCREEN_WIDTH, STATUS_BAR_WIDTH, TFT_BLACK);
            display_obj.tft.fillRect(0, SCREEN_HEIGHT / 3 + TEXT_HEIGHT * 2, SCREEN_WIDTH, STATUS_BAR_WIDTH, TFT_BLACK);
            display_obj.tft.setCursor(0, SCREEN_HEIGHT / 3);
            display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
            display_obj.tft.println(wifi_scan_obj.current_mini_kb_ssid + "\n");
            display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);

            display_obj.tft.println(ssids->get(0).essid);

            display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);
            display_obj.tft.println("U/D - Rem/Add Char");
            display_obj.tft.println("L/R - Prev/Nxt Char");
            display_obj.tft.println("C - Save");
            display_obj.tft.println("C(Hold) - Exit");
            pressed = false;
          }
        }
      #endif
    #endif
  }
#endif

// Function to show all MenuNodes in a Menu
void MenuFunctions::showMenuList(Menu * menu, int layer)
{
  // Iterate through all of the menu nodes in the menu
  for (uint8_t i = 0; i < menu->list->size(); i++)
  {
    // Depending on layer, indent
    for (uint8_t x = 0; x < layer * 4; x++)
      Serial.print(" ");
    Serial.print("Node: ");
    Serial.println(menu->list->get(i).name);
  }
  Serial.println();
}


// Function to add MenuNodes to a menu
/*void MenuFunctions::addNodes(Menu * menu, String name, uint16_t color, Menu * child, int place, std::function<void()> callable, bool selected, String command)
{
  TFT_eSPI_Button new_button;
  menu->list->add(MenuNode{name, false, color, place, &new_button, selected, callable});
  //menu->list->add(MenuNode{name, false, color, place, selected, callable});
}*/

void MenuFunctions::addNodes(Menu * menu, String name, uint8_t color, Menu * child, int place, std::function<void()> callable, bool selected, String command)
{
  TFT_eSPI_Button new_button;
  menu->list->add(MenuNode{name, false, color, place, &new_button, selected, callable});
  //menu->list->add(MenuNode{name, false, color, place, selected, callable});
}

void MenuFunctions::setGraphScale(float scale) {
  this->_graph_scale = scale;
}

float MenuFunctions::calculateGraphScale(int16_t value) {
  if (value < GRAPH_VERT_LIM) {
    return 1.0;  // No scaling needed if the value is within the limit
  }

  // Calculate the multiplier proportionally
  return (0.5 * GRAPH_VERT_LIM) / value;
}

float MenuFunctions::graphScaleCheck(const int16_t array[TFT_WIDTH]) {
  int16_t maxValue = 0;

  // Iterate through the array to find the highest value
  for (int16_t i = 0; i < TFT_WIDTH; i++) {
    if (array[i] > maxValue) {
      maxValue = array[i];
    }
  }

  // If the highest value exceeds GRAPH_VERT_LIM, call calculateMultiplier
  if (maxValue > GRAPH_VERT_LIM) {
    return this->calculateGraphScale(maxValue);
  }

  // If the highest value does not exceed GRAPH_VERT_LIM, return 1.0
  return 1.0;
}

void MenuFunctions::drawMaxLine(int16_t value, uint16_t color) {
  display_obj.tft.drawLine(0, TFT_HEIGHT - (value * this->_graph_scale), TFT_WIDTH, TFT_HEIGHT - (value * this->_graph_scale), color);
  display_obj.tft.setCursor(0, TFT_HEIGHT - (value * this->_graph_scale));
  display_obj.tft.setTextColor(color, TFT_BLACK);
  display_obj.tft.setTextSize(1);
  display_obj.tft.println((String)value);
}

void MenuFunctions::drawGraph(int16_t *values) {
  int16_t maxValue = 0;
  int total = 0;
  for (int i = TFT_WIDTH - 1; i >= 0; i--) {
    if (values[i] >= 0) {
      total = total + values[i];
      if (values[i] > maxValue) {
        maxValue = values[i];
      }
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_BLACK);
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - (values[i] * this->_graph_scale), TFT_CYAN);
    }
    else {
      int16_t ch_val = values[i] * -1;
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_BLACK);
      display_obj.tft.drawLine(i, TFT_HEIGHT, i, TFT_HEIGHT - GRAPH_VERT_LIM, TFT_RED);
      display_obj.tft.setCursor(i, TFT_HEIGHT - GRAPH_VERT_LIM);
      display_obj.tft.setTextColor(TFT_BLACK, TFT_RED);
      display_obj.tft.setTextSize(1);
      display_obj.tft.println((String)ch_val);
    }


  }

  this->drawMaxLine(maxValue, TFT_GREEN); // Draw max
  this->drawMaxLine(total / TFT_WIDTH, TFT_ORANGE); // Draw average
}

void MenuFunctions::renderGraphUI(uint8_t scan_mode) {
  display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (scan_mode == WIFI_SCAN_CHAN_ANALYZER)
    display_obj.tft.drawCentreString("Frames/" + (String)BANNER_TIME + "ms", TFT_WIDTH / 2, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), 1);
  else if (scan_mode == BT_SCAN_ANALYZER)
    display_obj.tft.drawCentreString("BLE Beacons/" + (String)BANNER_TIME + "ms", TFT_WIDTH / 2, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 2), 1);
  display_obj.tft.drawLine(0, TFT_HEIGHT - GRAPH_VERT_LIM - 1, TFT_WIDTH, TFT_HEIGHT - GRAPH_VERT_LIM - 1, TFT_WHITE);
  display_obj.tft.setCursor(0, TFT_HEIGHT - GRAPH_VERT_LIM - (CHAR_WIDTH * 8));
  display_obj.tft.setTextSize(1);
  display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  display_obj.tft.println("Max");
  display_obj.tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  display_obj.tft.println("Average");
  display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
  if (scan_mode != BT_SCAN_ANALYZER)
    display_obj.tft.println("Channel Marker");
}

uint16_t MenuFunctions::getColor(uint16_t color) {
  if (color == TFTWHITE) return TFT_WHITE;
  else if (color == TFTCYAN) return TFT_CYAN;
  else if (color == TFTBLUE) return TFT_BLUE;
  else if (color == TFTRED) return TFT_RED;
  else if (color == TFTGREEN) return TFT_GREEN;
  else if (color == TFTGREY) return TFT_LIGHTGREY;
  else if (color == TFTGRAY) return TFT_LIGHTGREY;
  else if (color == TFTMAGENTA) return TFT_MAGENTA;
  else if (color == TFTVIOLET) return TFT_VIOLET;
  else if (color == TFTORANGE) return TFT_ORANGE;
  else if (color == TFTYELLOW) return TFT_YELLOW;
  else if (color == TFTLIGHTGREY) return TFT_LIGHTGREY;
  else if (color == TFTPURPLE) return TFT_PURPLE;
  else if (color == TFTNAVY) return TFT_NAVY;
  else if (color == TFTSILVER) return TFT_SILVER;
  else if (color == TFTDARKGREY) return TFT_DARKGREY;
  else if (color == TFTSKYBLUE) return TFT_SKYBLUE;
  else if (color == TFTLIME) return 0x97e0;
  else return color;
}

// Function to change menu
void MenuFunctions::changeMenu(Menu * menu)
{
  display_obj.initScrollValues();
  display_obj.setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  display_obj.tft.init();
  current_menu = menu;

  current_menu->selected = 0;
  
  buildButtons(menu);

  displayCurrentMenu();
}

void MenuFunctions::buildButtons(Menu *menu, int starting_index, String button_name) {
  if (menu->list == NULL || menu->list->size() == 0)
      return;

  int _base = (menu->parentMenu != NULL) ? 1 : 0; // always hide the "Back" item (index 0) on submenus

  // Ensure starting index is within bounds
  if (starting_index >= menu->list->size())
    starting_index = menu->list->size() - BUTTON_SCREEN_LIMIT;
  if (starting_index < _base)
    starting_index = _base;

  this->menu_start_index = starting_index;

  // Determine the number of buttons to display (limited to screen capacity)
  uint8_t visible_buttons = min(BUTTON_SCREEN_LIMIT, menu->list->size() - starting_index);

  // Loop through and create only the visible buttons
  for (uint8_t i = 0; i < visible_buttons; i++) {
    uint16_t color = this->getColor(menu->list->get(starting_index + i).color);
    
    char buf[menu->list->get(starting_index + i).name.length() + 1] = {};
    if (button_name != "")
      menu->list->get(starting_index + i).name.toCharArray(buf, menu->list->get(starting_index + i).name.length() + 1);
    else
      button_name.toCharArray(buf, button_name.length() + 1);

    if (i >= BUTTON_SCREEN_LIMIT) {
      Serial.println("Error: Trying to access out-of-bounds button index " + (String)i);
      break;
    }

    display_obj.key[i].initButton(&display_obj.tft,
                                  KEY_X + 0 * (KEY_W + KEY_SPACING_X),
                                  KEY_Y + i * (KEY_H + KEY_SPACING_Y), // Positioning buttons vertically
                                  KEY_W,
                                  KEY_H,
                                  g_lineColor,                        // Outline (theme line colour)
                                  g_bgColor,                          // Fill (theme background)
                                  g_textColor ? g_textColor : color,  // Text colour (theme, or item colour)
                                  buf,
                                  KEY_TEXTSIZE);


    // Language rows draw a 2-letter code (EN/JP/...) in the left slot via
    // drawLangCode. Push the name further right on those rows so the code and
    // the name read as "EN  English" with a clear gap instead of butting together.
    int _labelPad = (langCode(String(buf)) != "") ? (BUTTON_PADDING + 16) : BUTTON_PADDING;
    display_obj.key[i].setLabelDatum(_labelPad - (KEY_W / 2), 2, ML_DATUM);
  }

  // Pagination nav buttons: Prev (left half) and Next (right half) along the bottom
  uint16_t navTxt = g_textColor ? g_textColor : TFT_CYAN;
  display_obj.key[BUTTON_SCREEN_LIMIT].initButton(&display_obj.tft, 60, 294, 118, 42,
      g_lineColor, g_bgColor, navTxt, (char*)"< Prev", 2);
  display_obj.key[BUTTON_SCREEN_LIMIT + 1].initButton(&display_obj.tft, 180, 294, 118, 42,
      g_lineColor, g_bgColor, navTxt, (char*)"Next >", 2);
}

void MenuFunctions::displayCurrentMenu(int start_index)
{
  // Clear the screen before displaying the current menu
  display_obj.clearScreen();
  display_obj.updateBanner(tr(current_menu->name));
  display_obj.tft.setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
  this->drawStatusBar();

  int _base = (current_menu->parentMenu != NULL) ? 1 : 0; // always skip hidden Back item on submenus
  if (start_index < _base) start_index = _base;
  #if !(defined(CYD_32CAP) || defined(CYD_35CAP))
    if (current_menu->parentMenu != NULL)
      display_obj.tft.fillTriangle(4, 24, 15, 18, 15, 30, TFT_CYAN); // corner back arrow
  #endif

  if (current_menu->list != NULL)
  {
    #ifdef HAS_FULL_SCREEN
      display_obj.tft.setFreeFont(MENU_FONT);
    #endif

    #ifdef HAS_MINI_SCREEN
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextSize(1);
    #endif
    for (uint16_t i = start_index; i < min(start_index + BUTTON_SCREEN_LIMIT, current_menu->list->size()); i++)
    {
      if (!current_menu || !current_menu->list || i >= current_menu->list->size())
        continue;
      uint16_t color = this->getColor(current_menu->list->get(i).color);
      #ifdef HAS_FULL_SCREEN
        #if !defined(HAS_ILI9341) && !defined(HAS_ST7796) && !defined(HAS_ST7789)
          if ((current_menu->list->get(i).selected) || (current_menu->selected == i)) {
            display_obj.key[i - start_index].drawButton(true, current_menu->list->get(i).name);
          }
          else {
            display_obj.key[i].drawButton(false, current_menu->list->get(i).name);
          }
        #else
          drawMenuLabel(i - start_index, KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y), color, current_menu->list->get(i).name, false);
        #endif

        if (current_menu->list->get(i).name == "Color Theme")
          drawThemeSwatch(KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y));
        else if ((current_menu->list->get(i).name != text09) && (current_menu->list->get(i).icon != 255))
          display_obj.tft.drawXBitmap(0,
                                      KEY_Y + (i - start_index) * (KEY_H + KEY_SPACING_Y) - (ICON_H / 2),
                                      menu_icons[current_menu->list->get(i).icon],
                                      ICON_W,
                                      ICON_H,
                                      g_bgColor,
                                      color);

      #endif

      #ifdef HAS_MINI_SCREEN
        if ((current_menu->selected == i) || (current_menu->list->get(i).selected))
          display_obj.key[i - start_index].drawButton(true, current_menu->list->get(i).name);
        else
          display_obj.key[i - start_index].drawButton(false, current_menu->list->get(i).name);
      #endif
    }

    // Pagination: a row of page dots above a split Prev/Next bar
    uint16_t _total = current_menu->list->size();
    uint16_t _items = _total - _base;
    if (_items > BUTTON_SCREEN_LIMIT) {
      uint16_t _pages = (_items + BUTTON_SCREEN_LIMIT - 1) / BUTTON_SCREEN_LIMIT;
      uint16_t _cur = (start_index - _base) / BUTTON_SCREEN_LIMIT;
      int _spacing = 14;
      int _dotX = (TFT_WIDTH / 2) - ((int)(_pages - 1) * _spacing) / 2;
      for (uint16_t p = 0; p < _pages; p++)
        display_obj.tft.fillCircle(_dotX + p * _spacing, 260, 3, (p == _cur) ? TFT_CYAN : TFT_DARKGREY);
      display_obj.tft.setFreeFont(NULL);
      display_obj.key[BUTTON_SCREEN_LIMIT].drawButton(false, "< Prev");
      display_obj.key[BUTTON_SCREEN_LIMIT + 1].drawButton(false, "Next >");
    }
    display_obj.tft.setFreeFont(NULL);
  }
}

#endif



