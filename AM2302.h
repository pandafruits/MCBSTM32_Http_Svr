#ifndef AM2302_H
#define AM2302_H

#include <stdint.h>

typedef struct {
	float Temp;
	float Hum;
} AM2302_Data_t;

extern AM2302_Data_t Ambient;

void AM2302_Init(void);
void AM2302_Read(void);

#endif

