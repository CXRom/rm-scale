// #include "ble_gatt.h"

// #define DEVICE_INFO_MANUFACTURER_NAME_UUID 0x2A29

// struct ble_gatt_chr_def ble_chr_general = {
//     .uuid = BLE_UUID16_DECLARE(DEVICE_INFO_MANUFACTURER_NAME_UUID), // Manufacturer Name String
//     .flags = BLE_GATT_CHR_F_READ,
//     .access_cb = device_info_callback,
// };

// static int device_info_callback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
// {
//   const char *value = "Rocket Monsters Scale";
//   return os_mbuf_append(ctxt->om, value, strlen(value));
// }