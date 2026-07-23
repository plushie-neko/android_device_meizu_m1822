#define LOG_TAG "FingerprintWrapper"

#include <cstdint>
#include <dlfcn.h>
#include <fcntl.h>
#include <hardware/fingerprint.h>
#include <hardware/hardware.h>
#include <log/log.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SUNWAVE_CHIP_INFO "/sys/devices/virtual/misc/sunwave_fp/chip_info"
#define GOODIX_LIB "fingerprint.goodix.so"
#define SUNWAVE_LIB "fingerprint.sunwave.so"

static bool is_sunwave() {
    int fd = open(SUNWAVE_CHIP_INFO, O_RDONLY);
    if (fd < 0) {
        ALOGE("Failed to open %s", SUNWAVE_CHIP_INFO);
        return false;
    }

    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n > 0) {
        buf[n] = '\0';
        // If id is 0x0, sunwave is not present
        if (strstr(buf, "id : 0x0") || strstr(buf, "id: 0x0")) {
            return false;
        }
        return true;
    }
    return false;
}

static int fingerprint_open(const struct hw_module_t* module, const char* id,
                            struct hw_device_t** device) {
    (void)module;

    if (device == nullptr) {
        ALOGE("NULL device on open");
        return -EINVAL;
    }

    bool sunwave = is_sunwave();
    const char* lib_name = sunwave ? SUNWAVE_LIB : GOODIX_LIB;
    ALOGI("Loading fingerprint library: %s", lib_name);

    void* handle = dlopen(lib_name, RTLD_NOW);
    if (!handle) {
        ALOGE("Failed to dlopen %s: %s", lib_name, dlerror());
        return -EINVAL;
    }

    struct hw_module_t* hmi = (struct hw_module_t*)dlsym(handle, HAL_MODULE_INFO_SYM_AS_STR);
    if (!hmi) {
        ALOGE("Failed to dlsym %s: %s", HAL_MODULE_INFO_SYM_AS_STR, dlerror());
        dlclose(handle);
        return -EINVAL;
    }

    int status = hmi->methods->open(hmi, id, device);
    if (status != 0) {
        ALOGE("Failed to open %s fingerprint device: %d", lib_name, status);
        dlclose(handle);
    }

    return status;
}

static struct hw_module_methods_t fingerprint_module_methods = {
    .open = fingerprint_open,
};

hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = FINGERPRINT_MODULE_API_VERSION_2_1,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = FINGERPRINT_HARDWARE_MODULE_ID,
    .name = "Meizu M1822 Fingerprint Wrapper",
    .author = "The LineageOS Project",
    .methods = &fingerprint_module_methods,
    .dso = nullptr,
    .reserved = {0},
};
