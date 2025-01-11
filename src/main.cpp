#include "logger.h"
#include "xml.h"

int main() {
  std::string testDocument =
      "<?xml version=\"1.0\"?>\r\n"
      "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n"
      "<specVersion>\r\n"
      "<major>1</major>\r\n"
      "<minor>0</minor>\r\n"
      "</specVersion>\r\n"
      "<URLBase>http://192.168.4.7:49152/upnp/</URLBase>\r\n"
      "<device>\r\n"
      "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>\r\n"
      "<friendlyName>Canon EOS Rebel T6</friendlyName>\r\n"
      "<manufacturer>Canon</manufacturer>\r\n"
      "<manufacturerURL>http://www.canon.com/</manufacturerURL>\r\n"
      "<modelDescription>Canon Digital Camera</modelDescription>\r\n"
      "<modelName>Canon EOS Rebel T6</modelName>\r\n"
      "<serialNumber>123456789012</serialNumber>\r\n"
      "<UDN>uuid:00000000-0000-0000-0001-F4A99DEF7ECD</UDN>\r\n"
      "<serviceList>\r\n"
      "<service>\r\n"
      "<serviceType>urn:schemas-canon-com:service:ICPO-\r\n"
      "SmartPhoneEOSSystemService:1</serviceType>\r\n"
      "<serviceId>urn:schemas-canon-com:serviceId:ICPO-\r\n"
      "SmartPhoneEOSSystemService-1</serviceId>\r\n"
      "<SCPDURL>CameraSvcDesc.xml</SCPDURL>\r\n"
      "<controlURL>control/CanonCamera/</controlURL>\r\n"
      "<eventSubURL></eventSubURL>\r\n"
      "<ns:X_targetId \r\n"
      "xmlns:ns=\"urn:schemas-canon-com:schema-upnp\">uuid:79621526-2AB6-A74E-"
      "\r\n"
      "9879-3C7C0A6C528F</ns:X_targetId>\r\n"
      "<ns:X_onService \r\n"
      "xmlns:ns=\"urn:schemas-canon-com:schema-upnp\">0</ns:X_onService>\r\n"
      "<ns:X_deviceUsbId \r\n"
      "xmlns:ns=\"urn:schemas-canon-com:schema-upnp\">32b4</"
      "ns:X_deviceUsbId>\r\n"
      "<ns:X_deviceNickname \r\n"
      "xmlns:ns=\"urn:schemas-canon-com:schema-upnp\">EV0314</\r\n"
      "ns:X_deviceNickname>\r\n"
      "</service>\r\n"
      "</serviceList>\r\n"
      "<presentationURL>/</presentationURL>\r\n"
      "</device>\r\n"
      "</root>\r\n";

  Buffer buffer;
  for (char& c : testDocument) {
    buffer.push_back(c);
  }

  XMLDocument doc;
  doc.unpack(buffer);

  Logger::log(doc, true);
  Logger::log();

  std::string friendlyName = doc["device"]["friendlyName"];
  std::string manufacturer = doc["device"]["manufacturer"];
  std::string modelName = doc["device"]["modelName"];
  std::string serialNumber = doc["device"]["serialNumber"];

  Logger::log("Friendly name: %s", friendlyName.c_str());
  Logger::log("Manufacturer: %s", manufacturer.c_str());
  Logger::log("Model name: %s", modelName.c_str());
  Logger::log("Serial number: %s", serialNumber.c_str());

  return 0;
}

#if defined(ESP32)
#include <WiFi.h>
#include <esp_pthread.h>
#include <thread>

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin("ESP8266_AP", "defgecd7");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("\nIP address: ");
  Serial.println(WiFi.localIP());

  delay(10000);

  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.stack_size = (4096);
  esp_pthread_set_cfg(&cfg);

  std::thread mainThread(main);
  mainThread.join();
}

void loop() {}
#endif