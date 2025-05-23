#ifndef WEB_RESPONSE_H
#define WEB_RESPONSE_H

#include <stdio.h>
extern int current_led_level;
extern int current_led_state;
extern int buzzer_state;
extern int current_sensor_state;
extern char error_message[256]; 

void sendError(FILE* fp);
void sendMainPage(FILE* fp);
void sendNumberInputPage(FILE* fp);
void handleNumberInput(FILE* fp, char *path);
void sendLedBuzzerStatusPage(FILE* fp);
void sendLightStatusPage(FILE* fp);

#endif