
#include "sdk_application_config.h"
#ifdef NRF_SDK15_YIELD
#include "yield.h"
#include "ruuvi_error.h"
#include "nrf_delay.h"
#include "nrf_pwr_mgmt.h"

#ifndef NULL
#ifdef __cplusplus
#define NULL   0
#else
#define NULL   ((void *) 0)
#endif
#endif

/** Handles softdevice being present and initialized, works around FPU bug **/
static ruuvi_status_t default_yield(void)
{
  nrf_pwr_mgmt_run();
  return RUUVI_SUCCESS;
}

static yield_fptr_t yield = default_yield;

ruuvi_status_t platform_yield_init(void)
{
  return nrf_pwr_mgmt_init();
}

/** Call function which will release execution / go to sleep **/
ruuvi_status_t platform_yield(void)
{
  if(NULL == yield) { return RUUVI_ERROR_NULL; }
  return yield();
}

/** Setup yield function, for example sd_app_evt_wait() with SD **/
void yield_set(yield_fptr_t yield_ptr)
{
  yield = yield_ptr;
}

/** delay given number of milliseconds **/
void platform_delay_ms(uint32_t time)
{
  nrf_delay_ms(time);
}

/** delay given number of microseconds **/
void platform_delay_us(uint32_t time)
{
  nrf_delay_us(time);
}

#endif