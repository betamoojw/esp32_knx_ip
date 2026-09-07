#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KNX_DPT9_MIN_VALUE (-671088.64f)
#define KNX_DPT9_MAX_VALUE (670760.96f)
#define KNX_DPT9_ENCODER_MIN_VALUE (-670760.96f)

typedef struct {
	uint8_t weekday;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} knx_dpt10_time_t;

typedef struct {
	uint8_t day;
	uint8_t month;
	uint8_t year;
} knx_dpt11_date_t;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} knx_dpt232_color_t;

bool knx_dpt1_decode(const uint8_t *data, size_t length, bool *value);
bool knx_dpt1_encode(bool value, uint8_t *data, size_t length);
bool knx_dpt9_decode(const uint8_t *data, size_t length, float *value);
bool knx_dpt9_encode(float value, uint8_t *data, size_t length);
bool knx_dpt5_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt5_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt6_decode(const uint8_t *data, size_t length, int8_t *value);
bool knx_dpt6_encode(int8_t value, uint8_t *data, size_t length);
bool knx_dpt7_decode(const uint8_t *data, size_t length, uint16_t *value);
bool knx_dpt7_encode(uint16_t value, uint8_t *data, size_t length);
bool knx_dpt8_decode(const uint8_t *data, size_t length, int16_t *value);
bool knx_dpt8_encode(int16_t value, uint8_t *data, size_t length);
bool knx_dpt10_decode(const uint8_t *data, size_t length, knx_dpt10_time_t *value);
bool knx_dpt10_encode(const knx_dpt10_time_t *value, uint8_t *data, size_t length);
bool knx_dpt11_decode(const uint8_t *data, size_t length, knx_dpt11_date_t *value);
bool knx_dpt11_encode(const knx_dpt11_date_t *value, uint8_t *data, size_t length);
bool knx_dpt12_decode(const uint8_t *data, size_t length, uint32_t *value);
bool knx_dpt12_encode(uint32_t value, uint8_t *data, size_t length);
bool knx_dpt13_decode(const uint8_t *data, size_t length, int32_t *value);
bool knx_dpt13_encode(int32_t value, uint8_t *data, size_t length);
bool knx_dpt14_decode(const uint8_t *data, size_t length, float *value);
bool knx_dpt14_encode(float value, uint8_t *data, size_t length);
bool knx_dpt16_decode(const uint8_t *data, size_t length, char *value,
					  size_t value_capacity);
bool knx_dpt16_encode(const char *value, uint8_t *data, size_t length);
bool knx_dpt232_decode(const uint8_t *data, size_t length,
					   knx_dpt232_color_t *value);
bool knx_dpt232_encode(const knx_dpt232_color_t *value, uint8_t *data,
					   size_t length);

#ifdef __cplusplus
}
#endif