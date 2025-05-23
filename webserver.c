#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wiringPi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include "device_control.h"
#include "web_response.h"

void *handle = NULL;
char error_message[256] = "";  

// void daemonize() {
//     pid_t pid = fork();
//     if (pid < 0) exit(EXIT_FAILURE);
//     if (pid > 0) exit(EXIT_SUCCESS);  
//     if (setsid() < 0) exit(EXIT_FAILURE);

//     pid = fork();
//     if (pid < 0) exit(EXIT_FAILURE);
//     if (pid > 0) exit(EXIT_SUCCESS);  

//     // 파일 권한 설정
//     umask(0);
//     chdir("/");

//     // 표준 파일 디스크립터 닫기
//     close(STDIN_FILENO);
//     close(STDOUT_FILENO);
//     close(STDERR_FILENO);

//     // syslog 사용 예시
//     openlog("daemon_server", LOG_PID, LOG_DAEMON);
//     syslog(LOG_INFO, "Daemon started successfully.");
// }

// LED
int led_override = 0;
int current_led_level = 0;
int current_led_state = 0;
int current_sensor_state = 0;
volatile int sensor_stop_flag = 0;
volatile int led_stop_flag = 0;
pthread_mutex_t led_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cds_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lib_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t tid_sensor, tid_led, tid_lib;

// 세그먼트/부저
int buzzer_state = 0;
volatile int buzzer_stop_flag = 0;
volatile int segment_stop_flag = 0;
pthread_t tid_seg;
pthread_mutex_t buzzer_lock = PTHREAD_MUTEX_INITIALIZER;

typedef void (*void_func_int)(int);
typedef void (*void_func_void)();
typedef int  (*int_func_void)();

// LED
void_func_void p_init_device = NULL;
int_func_void p_read_lightSensor = NULL;
void_func_int p_set_ledState = NULL;
void_func_int p_set_ledLevel = NULL;
void_func_void p_fnd_clear = NULL;
void_func_void p_fnd_display = NULL;
void_func_void p_buzzer_play = NULL;
void_func_int p_set_buzzer_stop_flag = NULL;

void reset_function_pointers() {
    p_init_device = NULL;
    p_read_lightSensor = NULL;
    p_set_ledState = NULL;
    p_set_ledLevel = NULL;
    p_fnd_clear = NULL;
    p_fnd_display = NULL;
    p_buzzer_play = NULL;
    p_set_buzzer_stop_flag = NULL;
}

void* safe_dlsym(void* handle, const char* name) {
    dlerror(); // 에러 플래그 초기화
    void* sym = dlsym(handle, name);
    char* error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym error for %s: %s\n", name, error);
        exit(1);
    }
    return sym;
}

void load_shared_library(const char* path){
    pthread_mutex_lock(&lib_mutex);
    // if (handle) {
    //     printf("hi\n");
    //     //reset_function_pointers();
    //     dlclose(handle);
    //     handle = NULL;
    // }
    // printf("dlclosed\n");
    handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        pthread_mutex_unlock(&lib_mutex);
        exit(1);
    }
    p_init_device         = (void_func_void) safe_dlsym(handle, "init_device");
    p_read_lightSensor    = (int_func_void)  safe_dlsym(handle, "read_lightSensor");
    p_set_ledState        = (void_func_int)  safe_dlsym(handle, "set_ledState");
    p_set_ledLevel        = (void_func_int)  safe_dlsym(handle, "set_ledLevel");
    p_fnd_clear           = (void_func_void) safe_dlsym(handle, "fnd_clear");
    p_fnd_display         = (void_func_void) safe_dlsym(handle, "fnd_display");
    p_buzzer_play         = (void_func_void) safe_dlsym(handle, "buzzer_play");
    p_set_buzzer_stop_flag = (void_func_int) safe_dlsym(handle, "set_buzzer_stop_flag");
    pthread_mutex_unlock(&lib_mutex);
}

void *lightSensor_thread(void *arg) {
    while (!sensor_stop_flag) {
        pthread_mutex_lock(&lib_mutex);
        if (p_read_lightSensor != NULL)
            current_sensor_state = p_read_lightSensor();
        pthread_mutex_unlock(&lib_mutex);

        pthread_mutex_lock(&led_lock);
        if (!led_override) {  
            current_led_state = current_sensor_state;
            current_led_level = (current_sensor_state == 1) ? 3 : 0;
        } 
        pthread_mutex_unlock(&led_lock);

        
        sleep(1);
    }
    pthread_exit(NULL);
}

void *led_thread(void *arg) {
    int last_state = -1;
    int last_level = -1;

    while (!led_stop_flag) {
        pthread_mutex_lock(&led_lock);
        if (current_led_state != last_state || current_led_level != last_level) {
            if (current_led_state == 0) {
                p_set_ledState(0);
            } else {
                p_set_ledLevel(current_led_level);
            }
            last_state = current_led_state;
            last_level = current_led_level;
        }
        pthread_mutex_unlock(&led_lock);

        usleep(100000); // 0.1초
    }
    pthread_exit(NULL);
}

void* buzzer_thread(void* arg) {
    p_buzzer_play();
    pthread_mutex_lock(&buzzer_lock);
    buzzer_state = 0;
    pthread_mutex_unlock(&buzzer_lock);
    pthread_exit(NULL);
}

void* segment_thread(void* arg) {
    int num = *((int*)arg);
    free(arg);

    while (num >= 0) {
        p_fnd_display(num);
        sleep(1);
        p_fnd_clear();
        num--;
        if (segment_stop_flag == 1){
            //break;
            pthread_exit(NULL);
            return NULL;
        }
    }

    pthread_mutex_lock(&buzzer_lock);
    if (buzzer_state == 0) {
        buzzer_state = 1;
        pthread_t tid_buzzer;
        pthread_create(&tid_buzzer, NULL, buzzer_thread, NULL);
        pthread_detach(tid_buzzer);
    }
    pthread_mutex_unlock(&buzzer_lock);

    pthread_exit(NULL);
}

static void *clnt_connection(void *arg);
    void sendOk(FILE fp);
    void sendError(FILE* fp);
    void sendMainPage(FILE* fp);
    void sendLedBuzzerStatusPage(FILE* fp);
    void sendLightStatusPage(FILE* fp);

void *clnt_connection(void *arg)
{
    int csock = *((int*)arg);
    FILE *clnt_read = fdopen(csock, "r");
    FILE *clnt_write = fdopen(dup(csock), "w");

    char reg_line[BUFSIZ];
    char method[16], path[256];

    fgets(reg_line, sizeof(reg_line), clnt_read);

    sscanf(reg_line, "%s %s", method, path);

    if(strcmp(method, "GET") == 0) {
        if(strcmp(path, "/") == 0 || strcmp(path, "/index") == 0) {
            sendMainPage(clnt_write);
        }
        else if(strcmp(path, "/status") == 0) {
            sendLedBuzzerStatusPage(clnt_write);
        }
        else if(strcmp(path, "/lightstatus") == 0) {
            sendLightStatusPage(clnt_write);
        }
        else if(strncmp(path, "/setnumber", 10) == 0) {
            char *query = strchr(path, '?');
            int number = -1;
            if(query != NULL) {
                if(sscanf(query, "?num=%d", &number) == 1) {
                    printf("Received number: %d\\n", number);
                    if (number >= 0 && number <= 9) {
                        error_message[0] = '\0';
                        int* arg = malloc(sizeof(int));
                        *arg = number;
                        segment_stop_flag = 0;
                        pthread_create(&tid_seg, NULL, segment_thread, arg);
                        if (segment_stop_flag == 0){
                            pthread_detach(tid_seg);
                            //continue;
                        }
                    } else {
                        snprintf(error_message, sizeof(error_message), "segment seconds must be set 0-9.");
                        //continue;
                    }
                }
            }
            sendLedBuzzerStatusPage(clnt_write);
        }
        else if (strncmp(path, "/ledon", 6) == 0) {
            int level = 1;  // 기본값
            char* query = strchr(path, '?');
            if (query && strstr(query, "level=")) {
                sscanf(query, "?level=%d", &level);
            }
            pthread_mutex_lock(&led_lock);
            led_override = 1;
            current_led_state = 1;
            current_led_level = level;
            //set_led_level(level);
            sendLedBuzzerStatusPage(clnt_write);
            pthread_mutex_unlock(&led_lock);
        }
        else if(strcmp(path, "/ledoff") == 0) {
            pthread_mutex_lock(&led_lock);
            led_override = 0;
            current_led_state = 0;
            current_led_level = 0;
            sendLedBuzzerStatusPage(clnt_write);
            pthread_mutex_unlock(&led_lock);
        }
        else if(strcmp(path, "/buzzon") == 0) {
            pthread_mutex_lock(&buzzer_lock);
            if (buzzer_state == 0) {
                buzzer_state = 1;
                pthread_t tid_buzzer;
                pthread_create(&tid_buzzer, NULL, buzzer_thread, NULL);
                pthread_detach(tid_buzzer);
                printf("부저 수동 ON\n");
            }
            sendLedBuzzerStatusPage(clnt_write);
            pthread_mutex_unlock(&buzzer_lock);
        }
        else if(strcmp(path, "/buzzoff") == 0) {
            pthread_mutex_lock(&buzzer_lock);
            if (buzzer_state == 1) {
                p_set_buzzer_stop_flag(1);
                buzzer_state = 0;
            }
            sendLedBuzzerStatusPage(clnt_write);
            pthread_mutex_unlock(&buzzer_lock);
        }
        else if(strcmp(path, "/update")==0){
            printf("shared library를 업데이트합니다.\n");
            sensor_stop_flag = 1;    
            led_stop_flag = 1;
            segment_stop_flag = 1;
            p_set_buzzer_stop_flag(1);
            buzzer_state = 0;
            pthread_join(tid_sensor, NULL);
            pthread_join(tid_led, NULL);
            if (pthread_join(tid_seg, NULL) != 0) {
                perror("pthread_join tid_seg");
            }
            
            printf("reload...\n");
            load_shared_library("./libshared.so");
            if (!handle || !p_init_device || !p_read_lightSensor) {
                fprintf(stderr, "공유 라이브러리 로딩 실패 또는 심볼 없음. update 중단.\n");
                exit(1);
            }

            p_init_device();
            
            sensor_stop_flag = 0;
            led_stop_flag = 0;
            pthread_create(&tid_sensor, NULL, lightSensor_thread, NULL);
            pthread_create(&tid_led, NULL, led_thread, NULL);

            printf("업데이트 완료.\n");
            sendMainPage(clnt_write);
        }
        else {
            sendError(clnt_write);
        }
    }
    fclose(clnt_read);
    fclose(clnt_write);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    //daemonize();
    //syslog(LOG_INFO, "Main loop starting.");
    int ssock;
    pthread_t thread;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int len;

    if(argc!=2) {
        printf("usage: %s <port>\\n", argv[0]);
        return -1;
    }

    load_shared_library("./libshared.so");
    if (!handle || !p_init_device || !p_read_lightSensor) {
        fprintf(stderr, "공유 라이브러리 로딩 실패 또는 심볼 없음. update 중단.\n");
    }
    p_init_device();

    
    pthread_create(&tid_sensor, NULL, lightSensor_thread, NULL);
    pthread_create(&tid_led, NULL, led_thread, NULL);

    char input[100];

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if(ssock == -1) {
        perror("socket()");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    int optval = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1) {
        perror("bind()");
        return -1;
    }

    if(listen(ssock, 10) == -1) {
        perror("listen()");
        return -1;
    }

    while(1) {
        int csock;
        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr*)&cliaddr, &len);
        pthread_create(&thread, NULL, clnt_connection, &csock);
        pthread_detach(thread); // join 대신 detach
    }
    return 0;

}

