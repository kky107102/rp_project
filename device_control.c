#include <stdio.h>
#include <stdlib.h>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
#include <string.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pthread.h>
#include <softTone.h>
#include <softPwm.h>

#define FND_GPIO_D 16
#define FND_GPIO_C 20
#define FND_GPIO_B 21
#define FND_GPIO_A 12
#define LED_GPIO 18
#define BUZZER_GPIO 25 
#define THREASHOLD_LIGHT 180
#define TOTAL 16

static int fd = -1;
static volatile int buzzer_stop_flag = 0;
int fnd_gpiopins[4] = {FND_GPIO_D, FND_GPIO_C, FND_GPIO_B, FND_GPIO_A};

void init_device() {
    if (wiringPiSetupGpio() == -1){
        perror("wiringPiSetupGpio");
        exit(1);
    }

    pinMode(LED_GPIO, OUTPUT);
    if (softPwmCreate(LED_GPIO, 0, 100) != 0) {
        perror("softPwmCreate");
        exit(1);
    }

    fd = wiringPiI2CSetupInterface("/dev/i2c-1", 0x48);
    if (fd < 0) {
        perror("wiringPiI2CSetupInterface failed");
        exit(1);
    }

    for (int i = 0; i < 4; i++) {
        pinMode(fnd_gpiopins[i], OUTPUT); 	/* 모든 Pin의 출력 설정 */
    }

    softToneCreate(BUZZER_GPIO);
}

int read_lightSensor(){
	int a2dChannel = 0;     // analog channel AIN0, CDS sensor
	
    if (fd < 0) {
        fprintf(stderr, "I2C device not initialized\n");
    }
	
    wiringPiI2CWrite(fd, 0x00 | a2dChannel);       // 0000_0000 
    int a2dVal = wiringPiI2CRead(fd);
    printf("a2dVal = %d, ", a2dVal);
    
    int state;
    if(a2dVal < THREASHOLD_LIGHT) { 
        state = 1;
        printf("Bright!!\n");
    } else {
        state = 0;
        printf("Dark!!\n"); 
    }
    return state;
}

// led on/off
void set_ledState(int state){
    if(state == 1){
        softPwmWrite(LED_GPIO, 100);
    }
    else if (state == 0){
        softPwmWrite(LED_GPIO, 0);
    }
    else {
        printf("Invalid LED state\n");
    }
}

// led on/off
void set_ledLevel(int level){
    if(level == 1){
        softPwmWrite(LED_GPIO, 30);
        printf("밝기: 30%%\n");
    }
    else if (level == 2){
        softPwmWrite(LED_GPIO, 50);
        printf("밝기: 50%%\n");
    }
    else if (level == 3){
        softPwmWrite(LED_GPIO, 100);
        printf("밝기: 100%%\n");
    }
    else {
        printf("Invalid level (1~3)\n");
    }
}

int number[10][4] = { 
    {0,0,0,0}, 	/* 0 */
    {0,0,0,1}, 	/* 1 */
    {0,0,1,0}, 	/* 2 */
    {0,0,1,1}, 	/* 3 */
    {0,1,0,0}, 	/* 4 */
    {0,1,0,1}, 	/* 5 */
    {0,1,1,0}, 	/* 6 */
    {0,1,1,1}, 	/* 7 */
    {1,0,0,0}, 	/* 8 */
    {1,0,0,1}   /* 9 */
}; 	

void fnd_display(int num){
    for (int i = 0; i < 4; i++) {
        digitalWrite(fnd_gpiopins[i], number[num][i] ? HIGH:LOW);
    }
}

void fnd_clear(){
    for(int i = 0; i < 4; i++) { 	
        digitalWrite(fnd_gpiopins[i], HIGH);
    }
}

void set_buzzer_stop_flag(int flag) {
    buzzer_stop_flag = flag;
}

int get_buzzer_stop_flag() {
    return buzzer_stop_flag;
}

void buzzer_play(){
    int melody[] = { 	/* 작은별을 연주하기 위한 계이름 */
        261, 261, 392, 392, 440, 440, 392, 0,       
        349, 349, 329, 329, 294, 294, 261, 0
    };

    for (int i = 0; i < TOTAL; ++i) {
        if (get_buzzer_stop_flag()) break;
        softToneWrite(BUZZER_GPIO, melody[i]);
        delay(300); 		/* 음의 전체 길이만큼 출력되도록 대기 */
    }

    softToneWrite(BUZZER_GPIO, 0);
    set_buzzer_stop_flag(0);
}

