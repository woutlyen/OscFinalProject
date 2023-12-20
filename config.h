/**
 * \author Wout Lyen
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

typedef struct {
    sensor_id_t id;
    sensor_value_t value;
    sensor_ts_t ts;
} sensor_data_t;

#define SIZE 128

#ifndef SET_MAX_TEMP
#define SET_MAX_TEMP 20
#endif // !SET_MAX_TEMP

#ifndef SET_MIN_TEMP
#define SET_MIN_TEMP 10
#endif // !SET_MIN_TEMP

#ifndef TIMEOUT
#define TIMEOUT 5
#endif // !TIMEOUT

#endif /* _CONFIG_H_ */
