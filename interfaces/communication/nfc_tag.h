/* 
 * NFC tag is dependend on external reader establishing contact and polling for data, i.e. transfers can be processed only when
 * reader polls for new data. 
 * Additionally NFC tag has several records which must be serialized into one string. 
 *
 */

// text_record device info
// uri_record  URL to be written
// app_record  App to be launched
// communication record RX/TX 

#include "sdk_application_config.h"
#ifndef NFC_TAG_H
#define NFC_TAG_H
#include "communication.h"

#include <stdbool.h>

// Setup constant records
ruuvi_status_t nfc_text_record_set(const uint8_t* text, size_t length);
ruuvi_status_t nfc_uri_record_set(const uint8_t* uri, size_t length);
ruuvi_status_t nfc_app_record_set(const uint8_t* app, size_t length);


// Functions for implementing communication api
ruuvi_status_t nfc_init(ruuvi_communication_channel_t* nfc_comms);
// These should be called only via NFC comms api
// ruuvi_status_t nfc_uninit(ruuvi_communication_channel_t* nfc_comms);
// bool nfc_is_connected(void);
// ruuvi_status_t nfc_process_asynchronous(void);
// ruuvi_status_t nfc_process_synchronous(void);
// ruuvi_status_t nfc_flush_tx(void);
// ruuvi_status_t nfc_flush_rx(void);
// ruuvi_status_t nfc_message_put(ruuvi_communication_message_t* msg);
// ruuvi_status_t nfc_message_get(ruuvi_communication_message_t* msg);

#endif