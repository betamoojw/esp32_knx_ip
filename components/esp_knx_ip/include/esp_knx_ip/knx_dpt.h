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
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t weekday;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	bool fault;
	bool working_day;
	bool working_day_valid;
	bool date_valid;
	bool weekday_valid;
	bool time_valid;
	bool daylight_saving_time;
	bool clock_quality;
} knx_dpt19_datetime_t;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} knx_dpt232_color_t;

typedef struct {
	bool control;
	bool value;
} knx_dpt2_control_t;

typedef struct {
	bool control;
	uint8_t step_code;
} knx_dpt3_control_t;

typedef struct {
	bool learn;
	uint8_t scene_number;
} knx_dpt18_scene_control_t;

typedef struct {
	bool active;
	uint8_t scene_number;
} knx_dpt26_scene_info_t;

typedef struct {
	uint16_t value;
	uint16_t mask;
} knx_dpt27_combined_status_t;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t white;
	uint8_t valid_channels;
} knx_dpt251_color_t;

bool knx_dpt1_decode(const uint8_t *data, size_t length, bool *value);
bool knx_dpt1_encode(bool value, uint8_t *data, size_t length);
bool knx_dpt2_decode(const uint8_t *data, size_t length,
					 knx_dpt2_control_t *value);
bool knx_dpt2_encode(const knx_dpt2_control_t *value, uint8_t *data,
					 size_t length);
bool knx_dpt3_decode(const uint8_t *data, size_t length,
					 knx_dpt3_control_t *value);
bool knx_dpt3_encode(const knx_dpt3_control_t *value, uint8_t *data,
					 size_t length);
bool knx_dpt4_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt4_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt4_ascii_decode(const uint8_t *data, size_t length, char *value);
bool knx_dpt4_ascii_encode(char value, uint8_t *data, size_t length);
bool knx_dpt9_decode(const uint8_t *data, size_t length, float *value);
bool knx_dpt9_encode(float value, uint8_t *data, size_t length);
bool knx_dpt5_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt5_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt5_scaling_decode(const uint8_t *data, size_t length, float *value);
bool knx_dpt5_scaling_encode(float value, uint8_t *data, size_t length);
bool knx_dpt5_angle_decode(const uint8_t *data, size_t length, float *value);
bool knx_dpt5_angle_encode(float value, uint8_t *data, size_t length);
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
bool knx_dpt15_decode(const uint8_t *data, size_t length, uint32_t *value);
bool knx_dpt15_encode(uint32_t value, uint8_t *data, size_t length);
bool knx_dpt16_decode(const uint8_t *data, size_t length, char *value,
					  size_t value_capacity);
bool knx_dpt16_encode(const char *value, uint8_t *data, size_t length);
bool knx_dpt17_decode(const uint8_t *data, size_t length, uint8_t *scene_number);
bool knx_dpt17_encode(uint8_t scene_number, uint8_t *data, size_t length);
bool knx_dpt18_decode(const uint8_t *data, size_t length,
					  knx_dpt18_scene_control_t *value);
bool knx_dpt18_encode(const knx_dpt18_scene_control_t *value, uint8_t *data,
					  size_t length);
bool knx_dpt19_decode(const uint8_t *data, size_t length,
					  knx_dpt19_datetime_t *value);
bool knx_dpt19_encode(const knx_dpt19_datetime_t *value, uint8_t *data,
					  size_t length);
bool knx_dpt20_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt20_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt21_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt21_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt22_decode(const uint8_t *data, size_t length, uint16_t *value);
bool knx_dpt22_encode(uint16_t value, uint8_t *data, size_t length);
bool knx_dpt23_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt23_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt24_decode(const uint8_t *data, size_t length, char *value,
					  size_t value_capacity);
bool knx_dpt24_encode(const char *value, uint8_t *data, size_t length,
					  size_t *encoded_length);
bool knx_dpt25_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt25_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt26_decode(const uint8_t *data, size_t length,
					  knx_dpt26_scene_info_t *value);
bool knx_dpt26_encode(const knx_dpt26_scene_info_t *value, uint8_t *data,
					  size_t length);
bool knx_dpt27_decode(const uint8_t *data, size_t length,
					  knx_dpt27_combined_status_t *value);
bool knx_dpt27_encode(const knx_dpt27_combined_status_t *value, uint8_t *data,
					  size_t length);
bool knx_dpt28_decode(const uint8_t *data, size_t length, char *value,
					  size_t value_capacity);
bool knx_dpt28_encode(const char *value, uint8_t *data, size_t length,
					  size_t *encoded_length);
bool knx_dpt29_decode(const uint8_t *data, size_t length, int64_t *value);
bool knx_dpt29_encode(int64_t value, uint8_t *data, size_t length);
bool knx_dpt30_decode(const uint8_t *data, size_t length, uint8_t *value);
bool knx_dpt30_encode(uint8_t value, uint8_t *data, size_t length);
bool knx_dpt31_decode(const uint8_t *data, size_t length, uint32_t *value);
bool knx_dpt31_encode(uint32_t value, uint8_t *data, size_t length);
bool knx_dpt232_decode(const uint8_t *data, size_t length,
					   knx_dpt232_color_t *value);
bool knx_dpt232_encode(const knx_dpt232_color_t *value, uint8_t *data,
					   size_t length);
bool knx_dpt234_decode(const uint8_t *data, size_t length, char language[3]);
bool knx_dpt234_encode(const char language[3], uint8_t *data, size_t length);
bool knx_dpt251_decode(const uint8_t *data, size_t length,
					   knx_dpt251_color_t *value);
bool knx_dpt251_encode(const knx_dpt251_color_t *value, uint8_t *data,
					   size_t length);

#ifdef __cplusplus
}
#endif