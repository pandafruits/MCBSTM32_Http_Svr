#ifndef __RTC_H
#define __RTC_H
#include <stdint.h>

extern char DateTimeNow[20];

void  RTC_Config(void);
void  RTC_SetDateTime(char *dtString);
char* RTC_GetDateTime(void);

#endif
