/* http_get - Retrieves a web page over HTTP GET.
 *
 * See http_get_ssl for a TLS-enabled version.
 *
 * This sample code is in the public domain.,
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "ssid_config.h"

#define WEB_SERVER "chainxor.org"
#define WEB_PORT 80
#define WEB_URL "http://chainxor.org/"

/* searches for the given pattern s2 into the string s1 */
int xstrsearch ( char * s1, char * s2 )
{
    int i, j, k ;
    int l1 = strlen ( s1 ) ;
    int l2 = strlen ( s2 ) ;

    for ( i = 0 ; i <= l1 - l2 ; i++ )
    {
        j = 0 ;
        k = i ;
        while ( ( s1[k] == s2[j] ) && ( j < l2 ) )
        {
            k++ ;
            j++ ;
        }
        if ( j == l2 )
            return i ;
    }
    return -1 ;
}

//char *append(const char* str1, const char* str2)
//{
//    char * appended, *ptr;
//
//    if(appended = pvPortMalloc((strlen(str1) + strlen(str2) + 1) * sizeof(char)) == NULL) {
//        //No memory
//        return NULL;
//    }
//    ptr = appended;
//
//    for(; *str1; str1++) {*ptr = *str1; ptr++;}
//    for(; *str2; str2++) {*ptr = *str2; ptr++;}
//    
//    return appended;
//}


//void http_get(char *IP, int *port)
void http_get_time(void *pvParameters)
{
    int socket_fd, time_header_pos, r;
    struct sockaddr_in server;
    static char recv_buf[256], date[11];

    while(1) {

        socket_fd = socket(AF_INET , SOCK_STREAM , 0);
        if(socket_fd < 0) {
            printf("Failed to create socket\n");
            vTaskDelay(4000 / portTICK_RATE_MS);
            continue;
        }

        printf("socket was created\n");

        server.sin_addr.s_addr = inet_addr("192.168.2.102");
        server.sin_family = AF_INET;
        server.sin_port = htons( 8000 );

        if (connect(socket_fd , (struct sockaddr *)&server , sizeof(server)) < 0) {
            printf("connect error\n");
            close(socket_fd);
            vTaskDelay(4000 / portTICK_RATE_MS);
            continue;
        }
        printf("Connected...\n");
        //freeaddrinfo(server);

        const char* request = "GET / HTTP/1.0\r\n\r\n";
        if( send(socket_fd , request, strlen(request) , 0) < 0) {
            printf("Failed to send request\n");
            close(socket_fd);
            vTaskDelay(4000 / portTICK_RATE_MS);
            continue;
        }

        printf("Data sended\n");

        char * response = pvPortMalloc(1000);
        printf("\n\nLast data received: %d", strlen(response));
        do {
            bzero(recv_buf, 256);
            r = read(socket_fd, recv_buf, 255);
            if(r > 0) {
                printf("%s", recv_buf);
                response = strcat(response, recv_buf);
                if(response == NULL) {
                    printf("No memory to handle response");
                    close(socket_fd);
                    vTaskDelay(4000 / portTICK_RATE_MS);
                    vPortFree(response);
                    response = NULL;
                    continue;
                }
            }
        } while(r > 0);

        printf("\n\nData received: %d", strlen(response));
        time_header_pos = xstrsearch(response, "Posix-time:");
        if (time_header_pos > 0) {
            for(int i=0; i < 10; i++) {
                date[i] = response[time_header_pos + 12 + i];
            }
            printf("\nDate from server: %s\n", date);
        }

        printf("Closing...");
        vPortFree(response);
        close(socket_fd);
        response = NULL;
        printf("Closed...");
        vTaskDelay(4000 / portTICK_RATE_MS);
    }
}

void http_get_task(void *pvParameters)
{
    int successes = 0, failures = 0;
    printf("HTTP get task starting...\r\n");

    while(1) {
        const struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
        };
        struct addrinfo *res;

        printf("Running DNS lookup for %s...\r\n", WEB_SERVER);
        int err = getaddrinfo(WEB_SERVER, "80", &hints, &res);

        if(err != 0 || res == NULL) {
            printf("DNS lookup failed err=%d res=%p\r\n", err, res);
            if(res)
                freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_RATE_MS);
            failures++;
            continue;
        }
        /* Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        struct in_addr *addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        printf("DNS lookup succeeded. IP=%s\r\n", inet_ntoa(*addr));

        int s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            printf("... Failed to allocate socket.\r\n");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_RATE_MS);
            failures++;
            continue;
        }

        printf("... allocated socket\r\n");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            close(s);
            freeaddrinfo(res);
            printf("... socket connect failed.\r\n");
            vTaskDelay(4000 / portTICK_RATE_MS);
            failures++;
            continue;
        }

        printf("... connected\r\n");
        freeaddrinfo(res);

        const char *req =
            "GET "WEB_URL" HTTP/1.1\r\n"
            "User-Agent: esp-open-rtos/0.1 esp8266\r\n"
            "\r\n";
        printf("%s", req);
        if (write(s, req, strlen(req)) < 0) {
            printf("... socket send failed\r\n");
            close(s);
            vTaskDelay(4000 / portTICK_RATE_MS);
            failures++;
            continue;
        }
        printf("... socket send success\r\n");

        static char recv_buf[128];
        int r;
        do {
            bzero(recv_buf, 128);
            r = read(s, recv_buf, 127);
            if(r > 0) {
                printf("%s", recv_buf);
            }
        } while(r > 0);

        printf("... done reading from socket. Last read return=%d errno=%d\r\n", r, errno);
        if(r != 0)
            failures++;
        else
            successes++;
        close(s);
        printf("successes = %d failures = %d\r\n", successes, failures);
        for(int countdown = 10; countdown >= 0; countdown--) {
            printf("%d... ", countdown);
            vTaskDelay(1000 / portTICK_RATE_MS);
        }
        printf("\r\nStarting again!\r\n");
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };

    /* required to call wifi_set_opmode before station_set_config */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    //char *m_ip = "192.168.2.104";
    //int m_port = 8000;

    //http_get( m_ip, &m_port);

    xTaskCreate(&http_get_time, (signed char *)"get_time", 1024, NULL, 2, NULL);
//    xTaskCreate(&http_get_task, (signed char *)"get_task", 256, NULL, 2, NULL);
}

