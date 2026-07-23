#define LOG_TAG "FingerprintWrapper"

#include <fcntl.h>
#include <log/log.h>
#include <string.h>
#include <unistd.h>
#include <android-base/properties.h>

int main() {
    int fd = open("/sys/devices/virtual/misc/sunwave_fp/chip_info", O_RDONLY);
    bool is_sunwave = false;
    if (fd >= 0) {
        char buf[256];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (n > 0) {
            buf[n] = '\0';
            if (!strstr(buf, "id     : 0x0") && !strstr(buf, "id: 0x0") && !strstr(buf, "0x0 lib:")) {
                is_sunwave = true;
            }
        }
    }
    
    if (is_sunwave) {
        android::base::SetProperty("ro.hardware.fingerprint", "sunwave");
        android::base::SetProperty("persist.sys.fp.vendor", "sunwave");
        ALOGI("Detected Sunwave fingerprint sensor");
    } else {
        android::base::SetProperty("ro.hardware.fingerprint", "goodix");
        android::base::SetProperty("persist.sys.fp.vendor", "goodix");
        ALOGI("Detected Goodix fingerprint sensor");
    }
    return 0;
}
