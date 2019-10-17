//
// Created by wuyuncheng on 11/10/19.
//

#include "util.h"
#include "encoder.h"
#include <cstdarg>
#include <ctime>
#include <cstdlib>


void logger(FILE *out, const char *format, ...) {

    char buf[BUFSIZ] = {'\0'};
    char date_buf[50] = {'\0'};

    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    time_t current_time;
    current_time = time(NULL);
    struct tm *tm_struct = localtime(&current_time);
    sprintf(date_buf,"%04d-%02d-%02d %02d:%02d:%02d",
            tm_struct->tm_year + 1900,
            tm_struct->tm_mon + 1,
            tm_struct->tm_mday,
            tm_struct->tm_hour,
            tm_struct->tm_min,
            tm_struct->tm_sec);

    fprintf(out, "%s %s", date_buf, buf);
}


void print_string(const char *str){
    logger(stdout, "%s", str);
}





