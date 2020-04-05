#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

typedef struct tsWiFiConfig {
  String sSSID;
  String sPassword;
  String smDNSname;
}tsWiFiConfig;

// Loads the configuration from a file
void loadConfiguration(const char *filename, tsWiFiConfig &config);


// Saves the configuration to a file
void saveConfiguration(const char *filename, const tsWiFiConfig &config);

// Prints the content of a file to the Serial
void printConfiguration(const tsWiFiConfig &config);

#endif