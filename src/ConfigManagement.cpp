#include "ConfigManagement.h"

#include <ArduinoJson.h>
#include <SPIFFS.h>


///////////////////////////////////////
// Global variables
///////////////////////////////////////

// Loads the configuration from a file
void loadConfiguration(const char *filename, tsWiFiConfig &config) {
  // Open file for reading
  File file = SPIFFS.open(filename);

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Copy values from the JsonDocument to the Config
  config.sSSID = doc["ssid"] | "";
  config.sPassword = doc["password"] | "";
  config.smDNSname = doc["mdns_name"] | "";
  // strlcpy(config.hostname,                  // <- destination
  //         doc["hostname"] | "example.com",  // <- source
  //         sizeof(config.hostname));         // <- destination's capacity

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
}

// Saves the configuration to a file
void saveConfiguration(const char *filename, const tsWiFiConfig &config) {
  // Delete existing file, otherwise the configuration is appended to the file
  SPIFFS.remove(filename);

  // Open file for writing
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["ssid"] = config.sSSID;
  doc["password"] = config.sPassword;
  doc["mdns_name"] = config.smDNSname;

  // Serialize JSON to file
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  // Close the file
  file.close();
}

// Prints the content of a file to the Serial
void printConfiguration(const tsWiFiConfig &config) {
  Serial.println(F("-------------------------"));
  Serial.println(F("-- System configuration"));
  Serial.print("ssid:\t"); Serial.println(config.sSSID);
  Serial.print("password:\t"); Serial.println(config.sPassword);
  Serial.print("mdns_name:\t"); Serial.println(config.smDNSname);
  Serial.println(F("-------------------------"));
}

