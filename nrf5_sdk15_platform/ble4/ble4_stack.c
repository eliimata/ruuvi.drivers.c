#include "sdk_application_config.h"
#if NRF5_SDK15_BLE4_STACK

#include "ble4_stack.h"
#include "ruuvi_error.h"

#include <stdbool.h>
#include <stdint.h>
#include "nordic_common.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"
#include "sdk_errors.h"

#define PLATFORM_LOG_MODULE_NAME ble4_stack
#if BLE4_LOG_ENABLED
#define PLATFORM_LOG_LEVEL       BLE4_LOG_LEVEL
#define PLATFORM_LOG_INFO_COLOR  BLE4_INFO_COLOR
#else
#define PLATFORM_LOG_LEVEL       0
#endif
#include "platform_log.h"
PLATFORM_LOG_MODULE_REGISTER();

static bool ble_stack_is_init = false;

ruuvi_status_t ble4_stack_init(void)
{
    ret_code_t err_code = NRF_SUCCESS;

    err_code = nrf_sdh_enable_request();
    if (NRF_SUCCESS != err_code) { PLATFORM_LOG_ERROR("SDH Enable request error: %X", err_code); }
    // APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code |= nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG_DEFAULT, &ram_start);
    if (NRF_SUCCESS != err_code) { PLATFORM_LOG_ERROR("Setting default configuration error: %X", err_code); }
    // APP_ERROR_CHECK(err_code);
    PLATFORM_LOG_INFO("RAM starts at %d", ram_start);

    // Enable BLE stack.
    err_code |= nrf_sdh_ble_enable(&ram_start);
    if (NRF_SUCCESS != err_code) { PLATFORM_LOG_ERROR("Enabling SDH error: %X", err_code); }

    // APP_ERROR_CHECK(err_code);
    if (NRF_SUCCESS == err_code)
    {
        ble_stack_is_init = true;
        ble_version_t version;
        sd_ble_version_get(&version);
        PLATFORM_LOG_INFO("Enabled SD version 0x%04X", version.subversion_number);
    }
    return platform_to_ruuvi_error(&err_code);
}

ruuvi_status_t ble4_set_name(uint8_t* name, uint8_t name_length, bool include_serial)
{
    ble_gap_conn_sec_mode_t security;
    security.sm = 1;
    security.lv = 1;
    uint8_t len = name_length;
    char name_serial[27] = { 0 };
    memcpy(name_serial, name, name_length);

    if (include_serial)
    {
        unsigned int mac0 =  NRF_FICR->DEVICEADDR[0];
        // space + 4 hex chars
        sprintf(name_serial + name_length, "%04X", mac0 & 0xFFFF);
        len += 4;
    }
    ret_code_t err_code = sd_ble_gap_device_name_set (&security, (uint8_t*)name_serial, len);
    return platform_to_ruuvi_error(&err_code);
}

#endif