#ifndef DHT22_H
#define DHT22_H

#include <stdbool.h>

bool dht22_read(int *temp_out, int *humi_out);

#endif // DHT22_H