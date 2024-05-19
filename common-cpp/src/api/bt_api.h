#ifndef PAM_PCBIOUNLOCK_BT_API_H
#define PAM_PCBIOUNLOCK_BT_API_H

#include "api.h"

extern "C" {
    struct BluetoothDevice {
        char name[255]{};
        char address[18]{};
    };

    API bool bt_is_available();
    API BluetoothDevice *bt_scan_devices(int *count);
    API BluetoothDevice *bt_get_paired_devices(int *count);
    API bool bt_pair_device(BluetoothDevice *device);
}

#endif //PAM_PCBIOUNLOCK_BT_API_H
