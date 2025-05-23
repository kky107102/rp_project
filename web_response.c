#include <stdio.h>
#include <string.h>
#include "web_response.h"

void sendError(FILE* fp)
{
    fprintf(fp, "HTTP/1.1 400 Bad Request\r\n");
    fprintf(fp, "Content-Type: text/html\r\n\r\n");
    fprintf(fp, "<html><body><h1>Bad Request</h1></body></html>");
    fflush(fp);
}

void sendMainPage(FILE* fp)
{
    fprintf(fp, "HTTP/1.1 200 OK\r\n");
    fprintf(fp, "Content-Type: text/html\r\n\r\n");
    fprintf(fp, "<html><head><title>Main Page</title></head><body>");
    //fprintf(fp, "<h1>%s</h1>", greeting);
    fprintf(fp, "<h2> Good night, Sweet dreams</h2>");
    fprintf(fp, "<ul>");
    fprintf(fp, "<li><a href=\"/lightstatus\">Show CDS Status</a></li>");
    fprintf(fp, "<li><a href=\"/status\">Control 7Segment & LED & Buzzer</a></li>");
    fprintf(fp, "<li><a href=\"/update\">Device Update</a></li>");
    fprintf(fp, "</ul>");
    fprintf(fp, "</body></html>");
    fflush(fp);
}

void sendLedBuzzerStatusPage(FILE* fp)
{
    fprintf(fp, "HTTP/1.1 200 OK\r\n");
    fprintf(fp, "Content-Type: text/html\r\n\r\n");
    fprintf(fp, "<html><head><title>7Segment & LED & Buzzer Status</title></head><body>");
    fprintf(fp, "<h1>7Segment Control </h1>");
    fprintf(fp, "<form action=\"/setnumber\" method=\"GET\">");
    fprintf(fp, "After (Input number) seconds, play goodnight music \"Little Star\". <br><br><input type=\"number\" name=\"num\">");
    fprintf(fp, "<input type=\"submit\" value=\"Enter\">");
    fprintf(fp, "</form><br>");
    if (strlen(error_message) > 0) {
        fprintf(fp, "<p style='color: red;'><strong>%s</strong></p><br>", error_message);
    }
    fprintf(fp, "<h1>Buzzer Status: %s</h1>", buzzer_state ? "ON" : "OFF");
    fprintf(fp, "<a href=\"/buzzon\">Turn Music ON</a><br>");
    fprintf(fp, "<a href=\"/buzzoff\">Turn Music OFF</a><br><br>");
    fprintf(fp, "<h1>LED Status: %s (Level %d)</h1>", current_led_state ? "ON" : "OFF", current_led_level);
    fprintf(fp, "<a href=\"/ledon?level=1\">Turn LED ON: level 1</a><br>");
    fprintf(fp, "<a href=\"/ledon?level=2\">Turn LED ON: level 2</a><br>");
    fprintf(fp, "<a href=\"/ledon?level=3\">Turn LED ON: level 3</a><br>");
    fprintf(fp, "<a href=\"/ledoff\">Turn LED OFF</a><br><br>");
    fprintf(fp, "<a href=\"/lightstatus\">Go to CDS check page</a><br>");
    fprintf(fp, "<a href=\"/\">Go to main page</a>");
    fprintf(fp, "</body></html>");
    fflush(fp);
}

void sendLightStatusPage(FILE* fp)
{
    fprintf(fp, "HTTP/1.1 200 OK\\r\\n");
    fprintf(fp, "Content-Type: text/html\r\n\r\n");
    fprintf(fp, "<html><head><title>CDS status</title></head><body>");
    fprintf(fp, "<h3>Please press F5 to update CDS status.</h3>");
    fprintf(fp, "<h1>current CDS status: %d</h1>", current_sensor_state);
    fprintf(fp, "<a href=\"/status\">Go to control page</a><br>");
    fprintf(fp, "<a href=\"/\">Go to main page</a>");
    fprintf(fp, "</body></html>");
    fflush(fp);
}