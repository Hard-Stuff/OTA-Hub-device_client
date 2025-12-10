#pragma once
#include <Arduino.h>

String getMacAddress()
{
  uint8_t baseMac[6];
  // Get MAC address for WiFi station
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char baseMacChr[18] = {0};
  sprintf(baseMacChr, "%02X:%02X:%02X:%02X:%02X:%02X", baseMac[0], baseMac[1], baseMac[2], baseMac[3], baseMac[4], baseMac[5]);
  return String(baseMacChr);
}

time_t cvtDate()
{
  char s_month[5];
  int year;
  tmElements_t t;
  static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

  sscanf(__DATE__, "%s %hhd %d", s_month, &t.Day, &year);
  sscanf(__TIME__, "%2hhd %*c %2hhd %*c %2hhd", &t.Hour, &t.Minute, &t.Second);

  // Find where is s_month in month_names. Deduce month value.
  t.Month = (strstr(month_names, s_month) - month_names) / 3 + 1;

  // year can be given as '2010' or '10'. It is converted to years since 1970
  if (year > 99)
    t.Year = year - 1970;
  else
    t.Year = year + 30;

  return makeTime(t);
}
