#ifndef settings_h
#define settings_h

// 'byte' und 'word' doesn't work!
//  int valid;
//  char singleChar;
//  long longVar;
//  float field1;
//  float field2;
//  //String note;          // the length is not defined in this case
//  char charstrg1[30];     // The string could be max. 29 chars. long, the last char is '\0'
typedef struct
{
    uint8_t configversion;                  // 1 byte
    char wifi_ssid[50];                     // 50 bytes
    char wifi_psk[50];                      // 50 bytes
    char hostname[30];                      // 30 bytes
    char note[30];                          // 30 bytes
    char beamermodel[10];                   // 10 bytes
    uint32_t beamerbaudrate;                // 4 bytes
    char admin_username[30];                // 30 bytes
    char admin_password[30];                // 30 bytes
    char mqtt_server[30];                   // 30 bytes
    uint16_t mqtt_port;                     // 2 bytes
    char mqtt_user[50];                     // 50 bytes
    char mqtt_password[50];                 // 50 bytes
    char mqtt_prefix[50];                   // 50 bytes
    uint16_t mqtt_periodic_update_interval; // 2 bytes
    uint8_t led_brightness;                 // 1byte (in percent)
    char api_username[30];                  // 30 bytes
    char api_password[30];                  // 30 bytes
                                            // Total: 480 bytes
} configData_t;

#endif