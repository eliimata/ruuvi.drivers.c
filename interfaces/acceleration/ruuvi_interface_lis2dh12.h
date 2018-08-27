/**
 *  Ruuvi sensor abstraction functions for LIS2DH12
 *
 * License: BSD-3
 * Author: Otso Jousimaa <otso@ojousima.net>
 */

#ifndef RUUVI_INTERFACE_LIS2DH12_H
#define RUUVI_INTERFACE_LIS2DH12_H
#include "ruuvi_driver_error.h"
#include "ruuvi_driver_sensor.h"

// Counts of self-test change in 10 bit resolution 2 G scale, datasheet.
#define RUUVI_INTERFACE_LIS2DH12_SELFTEST_DIFF_MIN 17
#define RUUVI_INTERFACE_LIS2DH12_SELFTEST_DIFF_MAX 360
#define RUUVI_INTERFACE_LIS2DH12_DEFAULT_SCALE 2
#define RUUVI_INTERFACE_LIS2DH12_DEFAULT_RESOLUTION 10

ruuvi_driver_status_t ruuvi_interface_lis2dh12_init(ruuvi_driver_sensor_t* acceleration_sensor, ruuvi_driver_bus_t bus, uint8_t handle);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_uninit(ruuvi_driver_sensor_t* acceleration_sensor, ruuvi_driver_bus_t bus, uint8_t handle);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_samplerate_set(uint8_t* samplerate);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_samplerate_get(uint8_t* samplerate);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_resolution_set(uint8_t* resolution);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_resolution_get(uint8_t* resolution);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_scale_set(uint8_t* scale);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_scale_get(uint8_t* scale);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_dsp_set(uint8_t* dsp, uint8_t* parameter);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_dsp_get(uint8_t* dsp, uint8_t* parameter);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_mode_set(uint8_t*);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_mode_get(uint8_t*);
ruuvi_driver_status_t ruuvi_interface_lis2dh12_data_get(void* data);

#endif