#include <stdint.h>
#include <sys/types.h>

namespace android {
    // libandroid.so
    extern "C" void ALooper_forThread() {}
    extern "C" void ALooper_pollOnce() {}
    extern "C" void ALooper_prepare() {}
    extern "C" void ALooper_wake() {}
    extern "C" void ASensor_getMaxRange() {}
    extern "C" void ASensor_getMinDelay() {}
    extern "C" void ASensor_getName() {}
    extern "C" void ASensor_getVendor() {}
    extern "C" void ASensorEventQueue_disableSensor() {}
    extern "C" void ASensorEventQueue_enableSensor() {}
    extern "C" void ASensorEventQueue_getEvents() {}
    extern "C" void ASensorEventQueue_setEventRate() {}
    extern "C" void ASensorManager_createEventQueue() {}
    extern "C" void ASensorManager_destroyEventQueue() {}
    extern "C" void ASensorManager_getDefaultSensor() {}
    extern "C" void ASensorManager_getInstanceForPackage() {}
    // libandroid_runtime.so
    extern "C" void _ZNK7android6Sensor11getMaxValueEv() {}

    // camera HAL
    extern "C" uint8_t get_num_of_cameras_to_expose() {
        return 2;
    }

    // fingerprint HAL
    class String16;
    class PermissionCache {
    public:
        static bool checkPermission(const String16&, pid_t, uid_t) {
            return true;
        }
    };
}
