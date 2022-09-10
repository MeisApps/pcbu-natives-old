#ifndef PAM_PCBIOUNLOCK_BT_API_H
#define PAM_PCBIOUNLOCK_BT_API_H

#include "api.h"

extern "C" {
    struct BluetoothDevice {
        char name[255];
        char address[19];
    };

    API bool bt_is_available();
    API BluetoothDevice *bt_scan_devices(int *count);
}

#endif //PAM_PCBIOUNLOCK_BT_API_H
