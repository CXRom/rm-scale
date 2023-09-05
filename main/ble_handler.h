#ifndef __BLE_HANDLER_H__
#define __BLE_HANDLER_H__

#include <stdint.h>

typedef int (*command_request_handler)(const char *);

void ble_init();
void ble_config_request_handler(command_request_handler *handler);

static int command_request_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);
static int command_response_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg);

#endif