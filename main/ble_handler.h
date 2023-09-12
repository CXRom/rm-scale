#ifndef __BLE_HANDLER_H__
#define __BLE_HANDLER_H__

typedef void (*CommandHandler)(const char *command);

void ble_init(CommandHandler command_handler);

#endif