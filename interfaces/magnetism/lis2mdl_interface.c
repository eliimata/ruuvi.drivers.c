/**
 * LIS2MDL innterface implementation
 * Requires application config.h and boards.h for pinout definition
 * Requires STM Lis2mdl driver, available on github under unknown license
 */
#include "application_config.h"
#include "boards.h"
#if LIS2MDL_MAGNETISM
#include "ruuvi_error.h"
#include "ruuvi_sensor.h"
#include "lis2mdl_reg.h"
#include "lis2mdl_interface.h"
#include "i2c.h"
#include "yield.h"
#include "magnetism.h"

#define PLATFORM_LOG_MODULE_NAME lis2mdl_iface
#if LIS2MDL_INTERFACE_LOG_ENABLED
#define PLATFORM_LOG_LEVEL       LIS2MDL_INTERFACE_LOG_LEVEL
#define PLATFORM_LOG_INFO_COLOR  LIS2MDL_INTERFACE_INFO_COLOR
#else // ANT_BPWR_LOG_ENABLED
#define PLATFORM_LOG_LEVEL       0
#endif // ANT_BPWR_LOG_ENABLED
#include "platform_log.h"
PLATFORM_LOG_MODULE_REGISTER();

static lis2mdl_ctx_t dev_ctx;
static ruuvi_sensor_mode_t mode;
static uint8_t handle = LIS2MDL_ADDRESS;

/*
*  Initialize mems driver interface.
*/
ruuvi_status_t lis2mdl_interface_init(ruuvi_sensor_t* magnetic_sensor)
{
  if(NULL == magnetic_sensor) { return RUUVI_ERROR_NULL; }
  PLATFORM_LOG_INFO("Start LIS2MDL init");
  // LIS2DH12 functions return error codes from I2C stach
  ruuvi_status_t err_code = RUUVI_SUCCESS;
  uint8_t whoamI, rst;
  dev_ctx.write_reg = i2c_stm_platform_write;
  dev_ctx.read_reg = i2c_stm_platform_read;
  dev_ctx.handle = &handle;

  /*
   *  Check device ID.
   */
  whoamI = 0;
  err_code |= lis2mdl_device_id_get(&dev_ctx, &whoamI);
  if ( whoamI != LIS2MDL_ID )
  {
    PLATFORM_LOG_ERROR("LIS2MDL Not found. err_code 0x%d, WHOAMI 0x%X", err_code, whoamI);
    return RUUVI_ERROR_NOT_FOUND;
  }

  /*
   *  Restore default configuration.
   */
  err_code |= lis2mdl_reset_set(&dev_ctx, PROPERTY_ENABLE);
  do {
    err_code |= lis2mdl_reset_get(&dev_ctx, &rst);
  } while (rst);

  /*
   *  Disable Block Data Update.
   */
  err_code |= lis2mdl_block_data_update_set(&dev_ctx, PROPERTY_DISABLE);

  /*
   * Set Output Data Rate.
   */
  err_code |= lis2mdl_data_rate_set(&dev_ctx, LIS2MDL_ODR_10Hz);

  /*
   * Set / Reset sensor mode.
   */
  err_code |= lis2mdl_set_rst_mode_set(&dev_ctx, LIS2MDL_SENS_OFF_CANC_EVERY_ODR);

  /*
   * Enable temperature compensation.
   */
  err_code |= lis2mdl_offset_temp_comp_set(&dev_ctx, PROPERTY_ENABLE);

  /*
   * Set device in sleep mode.
   */
  err_code |= lis2mdl_operating_mode_set(&dev_ctx, LIS2MDL_POWER_DOWN);
  mode = RUUVI_SENSOR_MODE_SLEEP;


  if(RUUVI_SUCCESS == err_code)
  {
    magnetic_sensor->init           = lis2mdl_interface_init;
    magnetic_sensor->uninit         = lis2mdl_interface_uninit;
    magnetic_sensor->samplerate_set = lis2mdl_interface_samplerate_set;
    magnetic_sensor->samplerate_get = lis2mdl_interface_samplerate_get;
    magnetic_sensor->resolution_set = lis2mdl_interface_resolution_set;
    magnetic_sensor->resolution_get = lis2mdl_interface_resolution_get;
    magnetic_sensor->scale_set      = lis2mdl_interface_scale_set;
    magnetic_sensor->scale_get      = lis2mdl_interface_scale_get;
    magnetic_sensor->dsp_set        = lis2mdl_interface_dsp_set;
    magnetic_sensor->dsp_get        = lis2mdl_interface_dsp_get;
    magnetic_sensor->mode_set       = lis2mdl_interface_mode_set;
    magnetic_sensor->mode_get       = lis2mdl_interface_mode_get;
    magnetic_sensor->interrupt_set  = lis2mdl_interface_interrupt_set;
    magnetic_sensor->interrupt_get  = lis2mdl_interface_interrupt_get;
    magnetic_sensor->data_get       = lis2mdl_interface_data_get;
 }

  PLATFORM_LOG_INFO("LIS2MDL Init status 0x%X", err_code);
  return err_code;

}
ruuvi_status_t lis2mdl_interface_uninit(ruuvi_sensor_t* acceleration_sensor)
{
  return RUUVI_ERROR_NOT_IMPLEMENTED;
}

ruuvi_status_t lis2mdl_interface_samplerate_set(ruuvi_sensor_samplerate_t* samplerate)
{
  if (NULL == samplerate) { return RUUVI_ERROR_NULL; }
  ruuvi_status_t err_code = RUUVI_SUCCESS;

  if (RUUVI_SENSOR_SAMPLERATE_STOP == *samplerate) { err_code |= lis2mdl_operating_mode_set(&dev_ctx, LIS2MDL_POWER_DOWN); }
  else if (RUUVI_SENSOR_SAMPLERATE_MIN == *samplerate) { err_code |= lis2mdl_data_rate_set(&dev_ctx, LIS2MDL_ODR_10Hz); }
  else if (RUUVI_SENSOR_SAMPLERATE_MAX == *samplerate) { err_code |= lis2mdl_data_rate_set(&dev_ctx, LIS2MDL_ODR_10Hz); }
  else if (RUUVI_SENSOR_SAMPLERATE_NO_CHANGE == *samplerate) { return RUUVI_SUCCESS; }
  else if (10  >= *samplerate) { err_code |= lis2mdl_data_rate_set(&dev_ctx, LIS2MDL_ODR_10Hz);  }
  else if (20  >= *samplerate) { err_code |= lis2mdl_data_rate_set(&dev_ctx, LIS2MDL_ODR_20Hz);  }
  else if (50  >= *samplerate) { err_code |= lis2mdl_data_rate_set(&dev_ctx, LIS2MDL_ODR_50Hz);  }
  else if (100 >= *samplerate) { err_code |= lis2mdl_data_rate_set(&dev_ctx, LIS2MDL_ODR_100Hz); }

  if (RUUVI_SENSOR_MODE_CONTINOUS == mode) { err_code |= lis2mdl_operating_mode_set(&dev_ctx, LIS2MDL_CONTINUOUS_MODE); }

  return err_code;
}

ruuvi_status_t lis2mdl_interface_samplerate_get(ruuvi_sensor_samplerate_t* samplerate)
{
  if (NULL == samplerate) { return RUUVI_ERROR_NULL; }
  ruuvi_status_t err_code = RUUVI_SUCCESS;

  lis2mdl_odr_t rate;
  err_code |= lis2mdl_data_rate_get(&dev_ctx, &rate);

  switch (rate)
  {
  case LIS2MDL_ODR_10Hz:
    *samplerate = 10;
    break;

  case LIS2MDL_ODR_20Hz:
    *samplerate = 20;
    break;

  case LIS2MDL_ODR_50Hz:
    *samplerate = 50;
    break;

  case LIS2MDL_ODR_100Hz:
    *samplerate = 100;
    break;

  default:
    return RUUVI_ERROR_INTERNAL;
  }
  return RUUVI_SUCCESS;
}

ruuvi_status_t lis2mdl_interface_resolution_set(ruuvi_sensor_resolution_t* resolution)
{
  return RUUVI_ERROR_NOT_SUPPORTED;
}

ruuvi_status_t lis2mdl_interface_resolution_get(ruuvi_sensor_resolution_t* resolution)
{
  return RUUVI_ERROR_NOT_IMPLEMENTED;
}

ruuvi_status_t lis2mdl_interface_scale_set(ruuvi_sensor_scale_t* scale)
{
  return RUUVI_ERROR_NOT_SUPPORTED;
}

ruuvi_status_t lis2mdl_interface_scale_get(ruuvi_sensor_scale_t* scale)
{
  if (NULL == scale) { return RUUVI_ERROR_NULL; }
  *scale = 50;
  return RUUVI_SUCCESS;
}

ruuvi_status_t lis2mdl_interface_dsp_set(ruuvi_sensor_dsp_function_t* dsp, uint8_t* parameter)
{
  return RUUVI_ERROR_NOT_IMPLEMENTED;
}

ruuvi_status_t lis2mdl_interface_dsp_get(ruuvi_sensor_dsp_function_t* dsp, uint8_t* parameter)
{
  return RUUVI_ERROR_NOT_IMPLEMENTED;
}

ruuvi_status_t lis2mdl_interface_mode_set(ruuvi_sensor_mode_t* p_mode)
{
  if (NULL == p_mode) { return RUUVI_ERROR_NULL; }
  ruuvi_status_t err_code = RUUVI_SUCCESS;

  switch (*p_mode)
  {
  case RUUVI_SENSOR_MODE_SLEEP:
    mode = *p_mode;
    err_code |= lis2mdl_operating_mode_set(&dev_ctx, LIS2MDL_POWER_DOWN);
    break;

  case RUUVI_SENSOR_MODE_SINGLE_ASYNCHRONOUS:
    mode = RUUVI_SENSOR_MODE_SLEEP;
    err_code |= lis2mdl_operating_mode_set(&dev_ctx, LIS2MDL_SINGLE_TRIGGER);
    break;

  case RUUVI_SENSOR_MODE_SINGLE_BLOCKING:
    mode = RUUVI_SENSOR_MODE_SLEEP;
    err_code |= lis2mdl_operating_mode_set(&dev_ctx, LIS2MDL_SINGLE_TRIGGER);
    platform_delay_ms(10); //TODO: Verify delay
    break;

  case RUUVI_SENSOR_MODE_CONTINOUS:
    mode = *p_mode;
    err_code |= lis2mdl_operating_mode_set(&dev_ctx, LIS2MDL_CONTINUOUS_MODE);
    break;

  default:
    err_code |= RUUVI_ERROR_INVALID_PARAM;
    break;
  }
  return err_code;
}

ruuvi_status_t lis2mdl_interface_mode_get(ruuvi_sensor_mode_t* p_mode)
{
  if (NULL == p_mode) { return RUUVI_ERROR_NULL; }

  *p_mode = mode;

  return RUUVI_SUCCESS;
}

ruuvi_status_t lis2mdl_interface_interrupt_set(uint8_t number, float* threshold, ruuvi_sensor_trigger_t* trigger, ruuvi_sensor_dsp_function_t* dsp)
{
  return RUUVI_ERROR_NOT_IMPLEMENTED;
}

ruuvi_status_t lis2mdl_interface_interrupt_get(uint8_t number, float* threshold, ruuvi_sensor_trigger_t* trigger, ruuvi_sensor_dsp_function_t* dsp)
{
  return RUUVI_ERROR_NOT_IMPLEMENTED;
}

ruuvi_status_t lis2mdl_interface_data_get(void* p_data)
{
  if (NULL == p_data) { return RUUVI_ERROR_NULL; }
  ruuvi_status_t err_code = RUUVI_SUCCESS;
  ruuvi_magnetism_data_t* data = (ruuvi_magnetism_data_t*)p_data;

  axis3bit16_t data_raw_magnetic;
  memset(data_raw_magnetic.u8bit, 0x00, 3 * sizeof(int16_t));
  err_code |= lis2mdl_magnetic_raw_get(&dev_ctx, data_raw_magnetic.u8bit);
  data->x_mg = LIS2MDL_FROM_LSB_TO_mG( data_raw_magnetic.i16bit[0]);
  data->y_mg = LIS2MDL_FROM_LSB_TO_mG( data_raw_magnetic.i16bit[1]);
  data->z_mg = LIS2MDL_FROM_LSB_TO_mG( data_raw_magnetic.i16bit[2]);

  return err_code;
}

#endif