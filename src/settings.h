#ifndef settings_h
#define settings_h

// 'byte' und 'word' doesn't work!
typedef struct
{
    uint8_t configversion;

    char wifi_ssid[30];
    char wifi_psk[30];

    char hostname[30];
    char note[30];

    char beamermodel[10];
    uint16_t beamerbaudrate;

    char admin_username[30];
    char admin_password[30];

    char mqtt_server[30];
    uint16_t mqtt_port;
    char mqtt_user[50];
    char mqtt_password[50];
    char mqtt_prefix[50];
    uint16_t mqtt_periodic_update_interval;

    uint8_t led_brightness; // in percent

    char api_username[30];
    char api_password[30];

} configData_t;

#endif