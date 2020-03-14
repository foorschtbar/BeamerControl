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
    char mqtt_topicSubscribe[50];
    char mqtt_topicPublish[50];

    uint8_t led_brightness; // in percent

} configData_t;

#endif