/**
 * Abstraction for a communication channel
 * Each communication channel channel can have input and output directions, ie. UART RX/TX
 *
 * Channel must implement FIFO which stores messages while communication is not possible
 * Channel should implement on_connect and on_diconnect, i.e. when NFC field is detected or lost
 * Channel must implement asynchronous process-function which moves data between HW and SW buffers
 * Channel should implement blocking process-function which sends data until queue is empty.
 *   - Blocking TX and a repeating message will transmit all chuncks and return
 * Channel must implement function for retrieving number of elements in SW queue
 * Channel must implement function for retrieving first element of SW queue
 * Channel should implement a function for polling connection status, i.e. is BLE GATT connecction established
 *
 * Data is divided into messages. 
 * Messages have a length of payload
 * Messages have binary payload. Payload might contain further metadata, such as endpoints of ruuvi_standard_message_t.
 * Messages have a "repeat" flag for outgoing transfers, which indicates that message is placed back into TX queue
 * if Message does not fit into MTU of channel, i.e. over 20 bytes for BLE GATT, communication channel is allowed
 * to split message into payload-sized chunks. 
 */

#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <stdlib.h>
#include <stdint.h>

#include "ruuvi_error.h"

typedef struct //XXX Packed?
{
  size_t   payload_length; // as input: maximum length of data. as output: length of data written to payload
  uint8_t  repeat; // Number of times message should be repeated. UINT8_MAX for "forever" 
  uint8_t* payload;
}ruuvi_communication_message_t;

typedef struct ruuvi_communication_channel_t ruuvi_communication_channel_t;          // forward declaration *and* typedef

// Init functions
typedef ruuvi_status_t(*ruuvi_communication_channel_init_fp)(ruuvi_communication_channel_t*);

// Generic query status, process
typedef ruuvi_status_t(*ruuvi_communication_fp)(void);

//Get / put messages from queue
typedef ruuvi_status_t(*ruuvi_communication_xfer_fp)(ruuvi_communication_message_t*);

// Callbacks after operation
typedef ruuvi_status_t(*ruuvi_communication_cb_fp)(ruuvi_communication_fp cb);

struct ruuvi_communication_channel_t
{
  ruuvi_communication_channel_init_fp init;
  ruuvi_communication_channel_init_fp uninit;

  // Return RUUVI_SUCCESS if communication channel is ready for use,
  // Return RUUVI_ERROR_INVALID_STATE if communication channel cannot be used.
  // Returning more specific error code is allowed if channel cannot be used.
  ruuvi_communication_fp is_connected;

  // Return RUUVI_SUCCESS if data was placed on HW buffer.
  // Return error code detailing why data could not be queued if there was an error
  ruuvi_communication_fp process_asynchronous;

  // return RUUVI_SUCCESS when data has been sent and acknowledged if applicable.
  // return RUUVI_ERROR_INVALID_STATE if connection is not established.
  ruuvi_communication_fp process_synchronous;

  //Clear TX queue
  ruuvi_communication_fp flush_tx;

  //Clear RX queue
  ruuvi_communication_fp flush_rx;

  // return RUUVI SUCCESS on successful queuing, err_code on fail. 
  // Can success even if there is not connection, check transmitting message with process() function
  ruuvi_communication_xfer_fp message_put;

  // Return RUUVI_ERROR_NOT_FOUND if queue is empty, RUUVI SUCCESS if data could be read
  ruuvi_communication_xfer_fp message_get;

  // Called when connection has been established
  ruuvi_communication_cb_fp set_on_connect;

  // Called when connection has been lost
  ruuvi_communication_cb_fp set_on_disconnect;

  // Called after message has been received
  ruuvi_communication_cb_fp set_on_rx;

  // Called after message has been sent
  ruuvi_communication_cb_fp set_on_tx;
};

#endif