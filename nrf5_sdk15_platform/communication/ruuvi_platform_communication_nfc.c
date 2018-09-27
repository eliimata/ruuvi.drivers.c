/**
 * Ruuvi NFC functions
 *
 * License: BSD-3
 * Author: Otso Jousimaa <otso@ojousima.net>
 */

#include "ruuvi_platform_external_includes.h"
#if NRF5_SDK15_COMMUNICATION_NFC_ENABLED
#include "ruuvi_driver_error.h"
#include "ruuvi_interface_communication.h"
#include "ruuvi_interface_communication_nfc.h"
#include <stdint.h>

#include "nfc_t4t_lib.h"
#include "nfc_ndef_msg.h"
#include "nfc_uri_rec.h"
#include "nfc_text_rec.h"
#include "nfc_launchapp_rec.h"
#include "nfc_ndef_msg_parser.h"

static struct
{
  volatile uint8_t connected;    //NFC field is active
  uint8_t initialized;
  volatile uint8_t rx_updated;   // New data received
  volatile uint8_t tx_updated;   // New data should be written to buffer
  volatile uint8_t configurable; // Allow NFC to write configuration
  uint8_t nfc_ndef_msg[NRF5_SDK15_COMMUNICATION_NFC_NDEF_FILE_SIZE];
  volatile size_t  nfc_ndef_msg_len;
  ruuvi_interface_communication_evt_handler_fp_t on_nfc_evt;
}nrf5_sdk15_nfc_state;

/**
 * @brief Callback function for handling NFC events.
 */
static void nfc_callback(void * context,
                         nfc_t4t_event_t event,
                         const uint8_t * data,
                         size_t          dataLength,
                         uint32_t        flags)
{
  (void)context;

  switch (event)
  {
  case NFC_T4T_EVENT_FIELD_ON:
    nrf5_sdk15_nfc_state.connected = true;
    if (NULL != nrf5_sdk15_nfc_state.on_nfc_evt) { nrf5_sdk15_nfc_state.on_nfc_evt(RUUVI_INTERFACE_COMMUNICATION_CONNECTED); }
    //PLATFORM_LOG_DEBUG("Field detected, do not process data");
    break;

  case NFC_T4T_EVENT_FIELD_OFF:
    nrf5_sdk15_nfc_state.connected = false;
    if (NULL != nrf5_sdk15_nfc_state.on_nfc_evt) { nrf5_sdk15_nfc_state.on_nfc_evt(RUUVI_INTERFACE_COMMUNICATION_DISCONNECTED); }
    //PLATFORM_LOG_DEBUG("Field lost, ok to process data now");
    break;

  case NFC_T4T_EVENT_NDEF_READ:
    if (NULL != nrf5_sdk15_nfc_state.on_nfc_evt) { nrf5_sdk15_nfc_state.on_nfc_evt(RUUVI_INTERFACE_COMMUNICATION_SENT); }
    // PLATFORM_LOG_DEBUG("Data chunk sent");
    break;

  // Update process generally sets length of field to 0 and
  // then updates. Once done, length is updated again.
  case NFC_T4T_EVENT_NDEF_UPDATED:
    if (dataLength > 0)
    {
      // PLATFORM_LOG_INFO("Got new message with %d bytes", dataLength);
      nrf5_sdk15_nfc_state.nfc_ndef_msg_len = dataLength;
      nrf5_sdk15_nfc_state.rx_updated = true;
      // If tag is not configurable by NFC, set flag to overwrite received data.
      if (!nrf5_sdk15_nfc_state.configurable) { nrf5_sdk15_nfc_state.tx_updated = true;}
      // Do not process data in interrupt context, you should rather schedule data processing
      if (NULL != nrf5_sdk15_nfc_state.on_nfc_evt) { nrf5_sdk15_nfc_state.on_nfc_evt(RUUVI_INTERFACE_COMMUNICATION_RECEIVED); }
    }
    break;

  default:
    break;
  }
}

NFC_NDEF_MSG_DEF(nfc_ndef_msg, NRF5_SDK15_COMMUNICATION_NFC_MAX_RECORDS); // max NFC_MAX_NUMBER_OF_RECORDS records

// Setup constant records
static uint8_t nfc_fw_buf[NRF5_SDK15_COMMUNICATION_NFC_TEXT_BUFFER_SIZE];
static size_t nfc_fw_length = 0;
static uint8_t nfc_addr_buf[NRF5_SDK15_COMMUNICATION_NFC_TEXT_BUFFER_SIZE];
static size_t nfc_addr_length = 0;
static uint8_t nfc_id_buf[NRF5_SDK15_COMMUNICATION_NFC_TEXT_BUFFER_SIZE];
static size_t nfc_id_length = 0;
static uint8_t nfc_tx_buf[NRF5_SDK15_COMMUNICATION_NFC_DATA_BUFFER_SIZE];
static size_t nfc_tx_length = 0;

ruuvi_driver_status_t ruuvi_interface_communication_nfc_init(ruuvi_interface_communication_t* const channel)
{
  if (NULL == channel)                  { return RUUVI_DRIVER_ERROR_NULL; }
  if (nrf5_sdk15_nfc_state.initialized) { return RUUVI_DRIVER_ERROR_INVALID_STATE; }

  /* Set up NFC */
  ret_code_t err_code = NRF_SUCCESS;
  err_code |= nfc_t4t_setup(nfc_callback, NULL);

  memset(nrf5_sdk15_nfc_state.nfc_ndef_msg, 0, sizeof(nrf5_sdk15_nfc_state.nfc_ndef_msg));

  /* Run Read-Write mode for Type 4 Tag platform */
  err_code |= nfc_t4t_ndef_rwpayload_set(nrf5_sdk15_nfc_state.nfc_ndef_msg, sizeof(nrf5_sdk15_nfc_state.nfc_ndef_msg));

  /* Start sensing NFC field */
  err_code |= nfc_t4t_emulation_start();

  nrf5_sdk15_nfc_state.initialized = true;

  // Setup communication abstraction fps
  channel->init   = ruuvi_interface_communication_nfc_init;
  channel->uninit = ruuvi_interface_communication_nfc_uninit;
  channel->read   = ruuvi_interface_communication_nfc_receive;
  channel->send   = ruuvi_interface_communication_nfc_send;
  channel->on_evt = NULL;

  return ruuvi_platform_to_ruuvi_error(&err_code);
}

ruuvi_driver_status_t ruuvi_interface_communication_nfc_uninit(ruuvi_interface_communication_t* const channel)
{
  ret_code_t err_code = nfc_t4t_emulation_stop();
  err_code |= nfc_t4t_done();
  nrf5_sdk15_nfc_state.initialized = false;
  memset(channel, 0, sizeof(ruuvi_interface_communication_t));

  return ruuvi_platform_to_ruuvi_error(&err_code);
}

ruuvi_driver_status_t ruuvi_interface_communication_nfc_data_set(void)
{
  // State check
  if (!nrf5_sdk15_nfc_state.initialized) { return RUUVI_DRIVER_ERROR_INVALID_STATE; }
  if ( nrf5_sdk15_nfc_state.connected)   { return RUUVI_DRIVER_ERROR_INVALID_STATE; }

  // Return success if there is nothing to do
  if (!(nrf5_sdk15_nfc_state.tx_updated)) { return RUUVI_DRIVER_SUCCESS; }
  ret_code_t err_code = NRF_SUCCESS;

  /* Create NFC NDEF text record description in English */
  uint8_t fw_code[] = {'f', 'w'}; // Firmware
  NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_fw_rec,
                                UTF_8,
                                fw_code,
                                sizeof(fw_code),
                                nfc_fw_buf,
                                nfc_fw_length);

  uint8_t addr_code[] = {'a', 'd'}; // Address
  NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_addr_rec,
                                UTF_8,
                                addr_code,
                                sizeof(addr_code),
                                nfc_addr_buf,
                                nfc_addr_length);

  uint8_t id_code[] = {'i', 'd'}; // ID
  NFC_NDEF_TEXT_RECORD_DESC_DEF(nfc_id_rec,
                                UTF_8,
                                id_code,
                                sizeof(id_code),
                                nfc_id_buf,
                                nfc_id_length);

  NFC_NDEF_RECORD_BIN_DATA_DEF(nfc_bin_rec,                                       \
                               TNF_MEDIA_TYPE,                                    \
                               NULL, 0,                                           \
                               NULL,                                              \
                               0,                                                 \
                               nfc_tx_buf,                                        \
                               nfc_tx_length);

  //Clear our record
  nfc_ndef_msg_clear(&NFC_NDEF_MSG(nfc_ndef_msg));
  // Add new records if applicable
  if (nfc_fw_length)
  {
    err_code |= nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_ndef_msg),
                                        &NFC_NDEF_TEXT_RECORD_DESC(nfc_fw_rec));
  }
  if (nfc_addr_length)
  {
    err_code |= nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_ndef_msg),
                                        &NFC_NDEF_TEXT_RECORD_DESC(nfc_addr_rec));
  }

  if (nfc_id_length)
  {
    err_code |= nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_ndef_msg),
                                        &NFC_NDEF_TEXT_RECORD_DESC(nfc_id_rec));
  }

  if (nfc_tx_length)
  {
    err_code |= nfc_ndef_msg_record_add(&NFC_NDEF_MSG(nfc_ndef_msg),
                                        &NFC_NDEF_RECORD_BIN_DATA(nfc_bin_rec));
  }

  // Encode data to NFC buffer. NFC will transmit the buffer, i.e. data is updated immediately.
  uint32_t msg_len = sizeof(nrf5_sdk15_nfc_state.nfc_ndef_msg);
  err_code |= nfc_ndef_msg_encode(&NFC_NDEF_MSG(nfc_ndef_msg),
                                  nrf5_sdk15_nfc_state.nfc_ndef_msg,
                                  &msg_len);

  // TX Data processed, set update status to false
  nrf5_sdk15_nfc_state.tx_updated = false;

  return ruuvi_platform_to_ruuvi_error(&err_code);
}

ruuvi_driver_status_t ruuvi_interface_communication_nfc_send(ruuvi_interface_communication_message_t* message)
{
  if (NULL == message)                                                       { return RUUVI_DRIVER_ERROR_NULL; }
  if (message->data_length >= NRF5_SDK15_COMMUNICATION_NFC_DATA_BUFFER_SIZE) { return RUUVI_DRIVER_ERROR_INVALID_LENGTH; }
  nfc_tx_length = message->data_length;
  memcpy(nfc_tx_buf, message->data, nfc_tx_length);
  nrf5_sdk15_nfc_state.tx_updated = true;
  ruuvi_interface_communication_nfc_data_set();
  return RUUVI_DRIVER_SUCCESS;
}

ruuvi_driver_status_t ruuvi_interface_communication_nfc_fw_version_set(const uint8_t* const version, const uint8_t length)
{
  if (NULL == version && length)                               { return RUUVI_DRIVER_ERROR_NULL; }
  if (length >= NRF5_SDK15_COMMUNICATION_NFC_TEXT_BUFFER_SIZE) { return RUUVI_DRIVER_ERROR_INVALID_LENGTH; }
  nfc_fw_length = length;
  memcpy(nfc_fw_buf, version, nfc_fw_length);
  nrf5_sdk15_nfc_state.tx_updated = true;
  return RUUVI_DRIVER_SUCCESS;
}

ruuvi_driver_status_t ruuvi_interface_communication_nfc_address_set(const uint8_t* const address, const uint8_t length)
{
  if (NULL == address && length)                               { return RUUVI_DRIVER_ERROR_NULL; }
  if (length >= NRF5_SDK15_COMMUNICATION_NFC_TEXT_BUFFER_SIZE) { return RUUVI_DRIVER_ERROR_INVALID_LENGTH; }
  nfc_addr_length = length;
  memcpy(nfc_addr_buf, address, nfc_addr_length);
  nrf5_sdk15_nfc_state.tx_updated = true;
  return RUUVI_DRIVER_SUCCESS;
}

ruuvi_driver_status_t ruuvi_interface_communication_nfc_id_set(const uint8_t* const id, const uint8_t length)
{
  if (NULL == id && length)                                    { return RUUVI_DRIVER_ERROR_NULL; }
  if (length >= NRF5_SDK15_COMMUNICATION_NFC_TEXT_BUFFER_SIZE) { return RUUVI_DRIVER_ERROR_INVALID_LENGTH; }
  nfc_id_length = length;
  memcpy(nfc_id_buf, id, nfc_id_length);
  nrf5_sdk15_nfc_state.tx_updated = true;
  return RUUVI_DRIVER_SUCCESS;
}

// Not implemented
ruuvi_driver_status_t ruuvi_interface_communication_nfc_receive(ruuvi_interface_communication_message_t* messge)
{
  return RUUVI_DRIVER_ERROR_NOT_IMPLEMENTED;
}
#endif