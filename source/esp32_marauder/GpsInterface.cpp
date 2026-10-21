#include "GpsInterface.h"
#include <Preferences.h>

#ifdef HAS_GPS

extern GpsInterface gps_obj;

char nmeaBuffer[100];

MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));

HardwareSerial Serial2(GPS_SERIAL_INDEX);

// Send a u-blox UBX-CFG-RATE message to set the measurement/navigation rate.
// u-blox receivers (e.g. NEO-M8N) use this binary packet, not the NMEA $PMTK220;
// non-u-blox modules just ignore the binary bytes, so sending both covers any module.
static void gpsUbxSetRate(uint16_t measMs) {
  uint8_t p[14] = {
    0xB5, 0x62,                                          // UBX sync chars
    0x06, 0x08,                                          // class CFG, id RATE
    0x06, 0x00,                                          // payload length = 6
    (uint8_t)(measMs & 0xFF), (uint8_t)(measMs >> 8),    // measRate (ms)
    0x01, 0x00,                                          // navRate = 1 cycle
    0x01, 0x00,                                          // timeRef = 1 (GPS time)
    0x00, 0x00                                           // checksum (computed below)
  };
  uint8_t a = 0, b = 0;
  for (int i = 2; i < 12; i++) { a += p[i]; b += a; }    // Fletcher over class..payload
  p[12] = a; p[13] = b;
  Serial2.write(p, sizeof(p));
}

void GpsInterface::begin() {

  /*#ifdef MARAUDER_MINI
    pinMode(26, OUTPUT);

    delay(1);

    analogWrite(26, 243);
    delay(1);

    Serial.println("Activated GPS");
    delay(100);
  #endif*/

  
  // GPS wiring is runtime-configurable from the "GPS Settings" screen and
  // stored in NVS ("marauder" namespace). Fall back to the compile-time
  // defaults when nothing has been saved yet.
  //   gRx  = ESP GPIO that RECEIVES the GPS module's TX  (Serial2 rxPin)
  //   gTx  = ESP GPIO that SENDS to the GPS module's RX  (Serial2 txPin)
  //   gBaud= serial baud rate
  int espRx = GPS_TX;      // note: GPS_TX macro is the ESP RX pin (module-side naming)
  int espTx = 21;          // ESP TX pin -> GPS module RX; default IO21
  uint32_t baud = 9600;
  uint32_t gpsHz = 1;      // position/NMEA output rate in Hz (GPS Settings)
  {
    Preferences _gp;
    if (_gp.begin("marauder", true)) {
      espRx = _gp.getInt("gRx", GPS_TX);
      espTx = _gp.getInt("gTx", 21);
      baud  = _gp.getUInt("gBaud", 9600);
      gpsHz = _gp.getUInt("gHz", 1);
      _gp.end();
    }
  }
  Serial.printf("GPS serial: baud=%u rxPin=%d txPin=%d rate=%uHz\n", baud, espRx, espTx, gpsHz);
  Serial2.begin(baud, SERIAL_8N1, espRx, espTx);

  MicroNMEA::sendSentence(Serial2, "$PSTMSETPAR,1201,0x00000042");
  MicroNMEA::sendSentence(Serial2, "$PSTMSAVEPAR");

  MicroNMEA::sendSentence(Serial2, "$PSTMSRR");

  // Apply the configured fix/output rate in both common dialects so it works on any
  // module: MTK-style $PMTK220,<ms> (MediaTek, e.g. Adafruit Ultimate GPS) and the
  // u-blox UBX-CFG-RATE binary packet (e.g. NEO-M8N). Each module ignores the dialect
  // it doesn't understand.
  if (gpsHz < 1) gpsHz = 1;
  {
    uint16_t _ms = (uint16_t)(1000 / gpsHz);
    char _rate[24];
    snprintf(_rate, sizeof(_rate), "$PMTK220,%u", (unsigned)_ms);
    MicroNMEA::sendSentence(Serial2, _rate);   // MediaTek
    gpsUbxSetRate(_ms);                          // u-blox
  }

  delay(1000);

  if (Serial2.available()) {
    Serial.println("GPS Attached Successfully");
    this->gps_enabled = true;
    while (Serial2.available()) {
      //Fetch the character one by one
      char c = Serial2.read();
      //Serial.print(c);
      //Pass the character to the library
      nmea.process(c);
    }
  }
  else {
    this->gps_enabled = false;
    Serial.println("GPS Not Found");
  }
  

  this->type_flag=GPSTYPE_NATIVE; //enforce default
  this->disable_queue(); //init the queue, disabled, kill NULLs

  nmea.setUnknownSentenceHandler(gps_nmea_notimp);

}

//passthrough for other objects
void gps_nmea_notimp(MicroNMEA& nmea){
  gps_obj.enqueue(nmea);
}

void GpsInterface::enqueue(MicroNMEA& nmea){
  std::string nmea_sentence = std::string(nmea.getSentence());

  if(nmea_sentence.length()){
    this->notimp_nmea_sentence = nmea_sentence.c_str();

    bool unparsed=1;
    bool enqueue=1;

    char system=nmea.getTalkerID();
    String msg_id=nmea.getMessageID();
    int length=nmea_sentence.length();

    if(length>0&&length<256){
      if(system){
        if(msg_id=="TXT"){
          if(length>8){
            std::string content=nmea_sentence.substr(7,std::string::npos);

            int tot_brk=content.find(',');
            int num_brk=content.find(',',tot_brk+1);
            int txt_brk=content.find(',',num_brk+1);
            int chk_brk=content.rfind('*');

            if(tot_brk!=std::string::npos && num_brk!=std::string::npos && txt_brk!=std::string::npos && chk_brk!=std::string::npos
                && chk_brk>txt_brk && txt_brk>num_brk && num_brk>tot_brk && tot_brk>=0){
              std::string total_str=content.substr(0,tot_brk);
              std::string num_str=content.substr(tot_brk+1,num_brk-tot_brk-1);
              std::string type_str=content.substr(num_brk+1,txt_brk-num_brk-1);
              std::string text_str=content.substr(txt_brk+1,chk_brk-txt_brk-1);
              std::string checksum=content.substr(chk_brk+1,std::string::npos);

              int total=0;
              if(total_str.length()) total=atoi(total_str.c_str());

              int num=0;
              if(num_str.length()) num=atoi(num_str.c_str());

              int type=0;
              if(type_str.length()) type=atoi(type_str.c_str());

              if(text_str.length() && checksum.length()){
                String text=text_str.c_str();
                if(type>1){
                  char type_cstr[4];
                  snprintf(type_cstr, 4, "%02d ", type);
                  type_cstr[3]='\0';
                  text=type_cstr+text;
                }

                if((num<=1||total<=1) && this->queue_enabled_flag){
                  if(this->text){
                    if(this->text_in){
                      int size=text_in->size();
                      if(size){
                        #ifdef GPS_TEXT_MAXCYCLES
                          if(this->text_cycles>=GPS_TEXT_MAXCYCLES){
                        #else
                          if(this->text_cycles){
                        #endif
                            if(this->text->size()){
                              LinkedList<String> *delme=this->text;
                              this->text=new LinkedList<String>;
                              delete delme;
                              this->text_cycles=0;
                            }
                          }
                        
                        for(int i=0;i<size;i++){
                          this->text->add(this->text_in->get(i));
                        }
                        LinkedList<String> *delme=this->text_in;
                        this->text_in=new LinkedList<String>;
                        delete delme;
                        this->text_cycles++;

                        this->gps_text=text;
                      }
                    }
                    else
                      this->text_in=new LinkedList<String>;
                  }
                  else{
                    if(this->text_in){
                      this->text_cycles=0;
                      this->text=this->text_in;
                      if(this->text->size()){
                        if(this->gps_text=="") this->gps_text=this->text->get(0);
                        this->text_cycles++;
                      }
                      this->text_in=new LinkedList<String>;
                    }
                    else {
                      this->text_cycles=0;
                      this->text=new LinkedList<String>;
                      this->text_in=new LinkedList<String>;
                    }
                  }

                  this->text_in->add(text);
                }
                else if(this->queue_enabled_flag){
                  if(!this->text_in) this->text_in=new LinkedList<String>;
                  this->text_in->add(text);
                  int size=this->text_in->size();

                  #ifdef GPS_TEXT_MAXLINES
                    if(size>=GPS_TEXT_MAXLINES){
                  #else
                    if(size>=5){
                  #endif
                      #ifdef GPS_TEXT_MAXCYCLES
                        if(this->text_cycles>=GPS_TEXT_MAXCYCLES){
                      #else
                        if(this->text_cycles){
                      #endif
                          if(this->text->size()){
                            LinkedList<String> *delme=this->text;
                            this->text=new LinkedList<String>;
                            delete delme;
                            this->text_cycles=0;
                          }
                        }
                      
                        for(int i=0;i<size;i++)
                          this->text->add(this->text_in->get(i));

                        LinkedList<String> *delme=this->text_in;
                        this->text_in=new LinkedList<String>;
                        delete delme;
                        this->text_cycles++;
                      }
                }
                else
                  if(num<=1||total<=1) this->gps_text=text;

                if(this->gps_text=="") this->gps_text=text;
                unparsed=0;
              }
            }
          }
        }
      }
    }

    if(unparsed)
      this->notparsed_nmea_sentence = nmea_sentence.c_str();

    if(this->queue_enabled_flag){
      if(enqueue){
        nmea_sentence_t line = { unparsed, msg_id, nmea_sentence.c_str() };

        if(this->queue){
          #ifdef GPS_NMEA_MAXQUEUE
            if(this->queue->size()>=GPS_NMEA_MAXQUEUE)
          #else
            if(this->queue->size()>=30)
          #endif
              this->flush_queue();
        }
        else
           this->new_queue();

        this->queue->add(line);
      }
      else
        if(!this->queue)
          this->new_queue();
    }
    else
      this->flush_queue();
  }
  else
    if(!this->queue_enabled_flag)
      this->flush_queue();
}

void GpsInterface::enable_queue(){
  if(this->queue_enabled_flag){
    if(!this->queue)
      this->new_queue();
    if(!this->text)
      this->text=new LinkedList<String>;
    if(!this->text_in)
      this->text_in=new LinkedList<String>;
  }
  else {
    this->flush_queue();
    this->queue_enabled_flag=1;
  }
}

void GpsInterface::disable_queue(){
  this->queue_enabled_flag=0;
  this->flush_queue();
}

bool GpsInterface::queue_enabled(){
  return this->queue_enabled_flag;
}

LinkedList<nmea_sentence_t>* GpsInterface::get_queue(){
  return this->queue;
}

void GpsInterface::new_queue(){
  this->queue=new LinkedList<nmea_sentence_t>;
}

void GpsInterface::flush_queue(){
  this->flush_queue_nmea();
  this->flush_text();
}

void GpsInterface::flush_queue_nmea(){
  if(this->queue){
    if(this->queue->size()){
      LinkedList<nmea_sentence_t> *delme=this->queue;
      this->new_queue();
      delete delme;
    }
  }
  else
    this->new_queue();
}

void GpsInterface::flush_text(){
  this->flush_queue_text();
  this->flush_queue_textin();
}

void GpsInterface::flush_queue_text(){
  this->text_cycles=0;

  if(this->text){
    if(this->text->size()){
      LinkedList<String> *delme=this->text;
      this->text=new LinkedList<String>;
      delete delme;
    }
  }
  else
    this->text=new LinkedList<String>;
}

void GpsInterface::flush_queue_textin(){
  if(this->text_in){
    if(this->text_in->size()){
      LinkedList<String> *delme=this->text_in;
      this->text_in=new LinkedList<String>;
      delete delme;
    }
  }
  else
    this->text_in=new LinkedList<String>;
}

void GpsInterface::sendSentence(const char* sentence){
  MicroNMEA::sendSentence(Serial2, sentence);
}

void GpsInterface::sendSentence(Stream &s, const char* sentence){
  MicroNMEA::sendSentence(s, sentence);
}

void GpsInterface::setType(String t){
  if(t == "native")
    this->type_flag=GPSTYPE_NATIVE;
  else if(t == "gps")
    this->type_flag=GPSTYPE_GPS;
  else if(t == "glonass")
    this->type_flag=GPSTYPE_GLONASS;
  else if(t == "galileo")
    this->type_flag=GPSTYPE_GALILEO;
  else if(t == "navic")
    this->type_flag=GPSTYPE_NAVIC;
  else if(t == "qzss")
    this->type_flag=GPSTYPE_QZSS;        
  else if(t == "beidou")
    this->type_flag=GPSTYPE_BEIDOU;
  else if(t == "beidou_bd")
    this->type_flag=GPSTYPE_BEIDOU_BD;    
  else
    this->type_flag=GPSTYPE_ALL;
}

String GpsInterface::generateGXgga(){
  String msg_type="$"+this->generateType()+"GGA,";

  char timeStr[8];
  snprintf(timeStr, 8, "%02d%02d%02d,", (int)(nmea.getHour()), (int)(nmea.getMinute()), (int)(nmea.getSecond()));

  long lat = nmea.getLatitude();
  char latDir = lat < 0 ? 'S' : 'N';
  lat = abs(lat);
  char latStr[12];
  snprintf(latStr, 12, "%02ld%08.5f,", lat / 1000000, ((lat % 1000000)*60) / 1000000.0);

  long lon = nmea.getLongitude();
  char lonDir = lon < 0 ? 'W' : 'E';
  lon = abs(lon);
  char lonStr[13];
  snprintf(lonStr, 13, "%03ld%08.5f,", lon / 1000000, ((lon % 1000000)*60) / 1000000.0);

  int fixQuality = nmea.isValid() ? 1 : 0;
  char fixStr[3];
  snprintf(fixStr, 3, "%01d,", fixQuality);

  int numSatellites = nmea.getNumSatellites();
  char satStr[4];
  snprintf(satStr, 4, "%02d,", numSatellites);

  unsigned long hdop = nmea.getHDOP();
  char hdopStr[13];
  snprintf(hdopStr, 13, "%01.2f,", 2.5 * (((float)(hdop))/10));

  long altitude;
  if(!nmea.getAltitude(altitude)) altitude=0;
  char altStr[9];
  snprintf(altStr, 9, "%01.1f,", altitude/1000.0);

  String message = msg_type + timeStr + latStr + latDir + ',' + lonStr + lonDir +
                    ',' + fixStr + satStr + hdopStr + altStr + "M,,M,,";

  return message;
}

String GpsInterface::generateGXrmc(){
  String msg_type="$"+this->generateType()+"RMC,";

  char timeStr[8];
  snprintf(timeStr, 8, "%02d%02d%02d,", (int)(nmea.getHour()), (int)(nmea.getMinute()), (int)(nmea.getSecond()));

  char dateStr[8];
  snprintf(dateStr, 8, "%02d%02d%02d,", (int)(nmea.getDay()), (int)(nmea.getMonth()), (int)(nmea.getYear()%100));

  char status = nmea.isValid() ? 'A' : 'V';
  char mode = nmea.isValid() ? 'A' : 'N';

  long lat = nmea.getLatitude();
  char latDir = lat < 0 ? 'S' : 'N';
  lat = abs(lat);
  char latStr[12];
  snprintf(latStr, 12, "%02ld%08.5f,", lat / 1000000, ((lat % 1000000)*60) / 1000000.0);

  long lon = nmea.getLongitude();
  char lonDir = lon < 0 ? 'W' : 'E';
  lon = abs(lon);
  char lonStr[13];
  snprintf(lonStr, 13, "%03ld%08.5f,", lon / 1000000, ((lon % 1000000)*60) / 1000000.0);

  char speedStr[8];
  snprintf(speedStr, 8, "%01.1f,", nmea.getSpeed() / 1000.0);

  char courseStr[7];
  snprintf(courseStr, 7, "%01.1f,", nmea.getCourse() / 1000.0);

  String message = msg_type + timeStr + status + ',' + latStr + latDir + ',' +
                    lonStr + lonDir + ',' + speedStr + courseStr + dateStr + ',' + ',' + mode;
  return message;
}

String GpsInterface::generateType(){
  String msg_type="";

  if(this->type_flag<8) //8=BeiDou in BD mode
    msg_type+='G';

  if(this->type_flag == GPSTYPE_NATIVE){ //type_flag=0
    char system=this->nav_system;
    if(system)
      msg_type+=system;
    else
      msg_type+='N';
  }
  else if(this->type_flag == GPSTYPE_GPS) //type_flag=2
    msg_type+='P';
  else if(this->type_flag == GPSTYPE_GLONASS) //type_flag=3
    msg_type+='L';
  else if(this->type_flag == GPSTYPE_GALILEO) //type_flag=4
    msg_type+='A';
  else if(this->type_flag == GPSTYPE_NAVIC) //type_flag=5
    msg_type+='I';
  else if(this->type_flag == GPSTYPE_QZSS) //type_flag=6
    msg_type+='Q';
  else if(this->type_flag == GPSTYPE_BEIDOU) //type_flag=7
    msg_type+='B';
  else if(this->type_flag == GPSTYPE_BEIDOU_BD){ //type_flag=8
    msg_type+='B';
    msg_type+='D';
  }
  else{ //type_flag=1=all ... also default if unset/wrong (obj default is type_flag=0=native)
    if(this->type_flag>=8) //catch uncaught first char, assume G if not already output
      msg_type+='G';
    msg_type+='N';
  }

  return msg_type;
}

// Thanks JosephHewitt
String GpsInterface::dt_string_from_gps(){
  //Return a datetime String using GPS data only.
  String datetime = "";
  if (nmea.isValid() && nmea.getYear() > 0){
    datetime += nmea.getYear();
    datetime += "-";
    datetime += nmea.getMonth();
    datetime += "-";
    datetime += nmea.getDay();
    datetime += " ";
    datetime += nmea.getHour();
    datetime += ":";
    datetime += nmea.getMinute();
    datetime += ":";
    datetime += nmea.getSecond();
  }
  return datetime;
}

void GpsInterface::setGPSInfo() {
  String nmea_sentence = String(nmea.getSentence());
  if(nmea_sentence != "") this->nmea_sentence = nmea_sentence;

  this->good_fix = nmea.isValid();
  this->nav_system = nmea.getNavSystem();
  this->num_sats = nmea.getNumSatellites();

  this->datetime = this->dt_string_from_gps();

  this->lat = String((float)nmea.getLatitude()/1000000, 7);
  this->lon = String((float)nmea.getLongitude()/1000000, 7);
  long alt = 0;
  if (!nmea.getAltitude(alt)){
    alt = 0;
  }
  this->altf = (float)alt / 1000;

  this->accuracy = 2.5 * ((float)nmea.getHDOP()/10);

  //nmea.clear();
}

float GpsInterface::getAccuracy() {
  return this->accuracy;
}

String GpsInterface::getLat() {
  return this->lat;
}

String GpsInterface::getLon() {
  return this->lon;
}

float GpsInterface::getAlt() {
  return this->altf;
}

String GpsInterface::getDatetime() {
  return this->datetime;
}

// GPS gives UTC; add the local UTC offset (minutes) to show local time. The
// offset is set from Device > UTC Offset and passed in by the caller.
String GpsInterface::getTime(int offsetMin) {
  if (nmea.isValid() && nmea.getYear() > 0) {
    int total = nmea.getHour() * 60 + nmea.getMinute() + offsetMin;
    total = ((total % 1440) + 1440) % 1440;     // wrap within 0..1439 minutes/day
    int h = total / 60;
    int m = total % 60;
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, nmea.getSecond());
    return String(buf);
  }
  return "";
}

String GpsInterface::getNumSatsString() {
  return (String)num_sats;
}

int GpsInterface::getNumSats() {
  return num_sats;
}

bool GpsInterface::getFixStatus() {
  return this->good_fix;
}

String GpsInterface::getFixStatusAsString() {
  if (this->getFixStatus())
    return "Yes";
  else
    return "No";
}

bool GpsInterface::getGpsModuleStatus() {
  return this->gps_enabled;
}

uint32_t GpsInterface::getBaud() {
  Preferences _gp; uint32_t b = 9600;
  if (_gp.begin("marauder", true)) { b = _gp.getUInt("gBaud", 9600); _gp.end(); }
  return b;
}
float GpsInterface::getUpdateHz() { return this->_measuredHzX10 / 10.0f; }

String GpsInterface::getText() {
  return this->gps_text;
}

int GpsInterface::getTextQueueSize() {
  if(this->queue_enabled_flag){
    bool exists=0;
    if(this->text){
      int size=this->text->size();
      if(size) return size;
      exists=1;
    }
    if(this->text_in){
      int size=this->text_in->size();
      if(size) return size;
      exists=1;
    }
    if(exists)
      return 0;
    else
      return -2;
  }
  else
    return -1;
}

String GpsInterface::getTextQueue(bool flush) {
  if(this->queue_enabled_flag){
    if(this->text){
      int size=this->text->size();
      if(size){
        String text;
        for(int i=0;i<size;i++){
          String now=this->text_in->get(i);
          if(now!=""){
            if(text!=""){
              text+='\r';
              text+='\n';
            }
            text+=now;
          }
        }
        if(flush){
          LinkedList<String> *delme=this->text;
          this->text_cycles=0;
          this->text=this->text_in;
          if(!this->text) this->text=new LinkedList<String>;
          if(this->text->size()) this->text_cycles++;
          this->text_in=new LinkedList<String>;
          delete delme;
        }
        return text;
      }
    }
    else{
      this->text=new LinkedList<String>;
      this->text_cycles=0;
    }

    if(this->text_in){
      int size=this->text_in->size();
      if(size){
        LinkedList<String> *buffer=this->text_in;
        if(flush)
          this->text_in=new LinkedList<String>;
        String text;
        for(int i=0;i<size;i++){
          String now=buffer->get(i);
          if(now!=""){
            if(text!=""){
              text+='\r';
              text+='\n';
            }
            text+=now;
          }
        }
        if(flush)
          delete buffer;
        return text;
      }
    }
    else
      this->text_in=new LinkedList<String>;

    return this->gps_text;
  }
  else
    return this->gps_text;
}

String GpsInterface::getNmea() {
  return this->nmea_sentence;
}

String GpsInterface::getNmeaNotimp() {
  return this->notimp_nmea_sentence;
}

String GpsInterface::getNmeaNotparsed() {
  return this->notparsed_nmea_sentence;
}

void GpsInterface::main() {
  bool got_bytes = false;
  while (Serial2.available()) {
    //Fetch the character one by one
    char c = Serial2.read();
    //Serial.print(c);
    //Pass the character to the library. process() returns true when a full
    //sentence completed; count RMC (exactly one per position epoch on every
    //GPS) to MEASURE the real update rate the module is producing.
    if (nmea.process(c) && strcmp(nmea.getMessageID(), "RMC") == 0) this->_hzCount++;
    got_bytes = true;
  }

  // Compute the measured fix rate over a ~1s sliding window.
  uint32_t _now = millis();
  if (this->_hzWindowMs == 0) this->_hzWindowMs = _now;
  else if (_now - this->_hzWindowMs >= 1000) {
    uint32_t x10 = (uint32_t)this->_hzCount * 10000UL / (_now - this->_hzWindowMs);
    this->_measuredHzX10 = (x10 > 255) ? 255 : (uint8_t)x10;
    this->_hzCount = 0;
    this->_hzWindowMs = _now;
  }

  // Hot-detect: if the module wasn't present at boot but is now sending data,
  // enable it on the fly so the clock/GPS status light up without a reboot.
  if (got_bytes && !this->gps_enabled) {
    this->gps_enabled = true;
    Serial.println("GPS detected at runtime - enabling");
  }

  // Refresh the cached GPS state every loop so a fix (and sat count) shows the
  // instant the module reports it. The old code only updated on (valid && sats>0)
  // or (!valid && sats==0), so during acquisition (sats>0 but not yet valid) the
  // status lagged behind the module -- which looked like "slow to see the lock".
  this->setGPSInfo();
}
#endif
