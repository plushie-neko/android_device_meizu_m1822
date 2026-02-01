#define LOG_TAG "CameraShim"

#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/camera.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

static const char* VENDOR_LIBRARY_PATH = "/vendor/lib/hw/camera.msm8953_vendor.so";
static camera_module_t *gVendorModule = nullptr;
static void *gVendorHandle = nullptr;

typedef int (*get_cam_info_t)(uint32_t cameraId, struct camera_info *info);
static get_cam_info_t gVendorGetCamInfo = nullptr;

typedef void (*qcamera3_constructor_t)(void* this_ptr, uint32_t cameraId, const camera_module_callbacks_t *callbacks);
static qcamera3_constructor_t gVendorConstructor = nullptr;

typedef int (*qcamera3_open_t)(void* this_ptr, hw_device_t** device);
static qcamera3_open_t gVendorOpen = nullptr;

static int check_vendor_module() {
    if (gVendorModule) {
        return 0;
    }

    ALOGI("Opening vendor camera module: %s", VENDOR_LIBRARY_PATH);
    gVendorHandle = dlopen(VENDOR_LIBRARY_PATH, RTLD_NOW);
    if (!gVendorHandle) {
        ALOGE("Failed to open vendor camera module: %s", dlerror());
        return -EINVAL;
    }

    gVendorModule = (camera_module_t *)dlsym(gVendorHandle, HAL_MODULE_INFO_SYM_AS_STR);
    if (!gVendorModule) {
        ALOGE("Failed to find HAL_MODULE_INFO_SYM in vendor module");
        dlclose(gVendorHandle);
        gVendorHandle = nullptr;
        return -EINVAL;
    }

    gVendorGetCamInfo = (get_cam_info_t)dlsym(gVendorHandle, "_ZN7qcamera25QCamera3HardwareInterface10getCamInfoEjP11camera_info");
    gVendorConstructor = (qcamera3_constructor_t)dlsym(gVendorHandle, "_ZN7qcamera25QCamera3HardwareInterfaceC1EjPK23camera_module_callbacks");
    gVendorOpen = (qcamera3_open_t)dlsym(gVendorHandle, "_ZN7qcamera25QCamera3HardwareInterface10openCameraEPP11hw_device_t");

    if (!gVendorGetCamInfo || !gVendorConstructor || !gVendorOpen) {
        ALOGE("Failed to find required QCamera3HardwareInterface symbols in vendor module");
        dlclose(gVendorHandle);
        gVendorHandle = nullptr;
        gVendorModule = nullptr;
        return -EINVAL;
    }

    ALOGI("Successfully loaded vendor camera module");
    return 0;
}

static int shim_get_number_of_cameras(void) {
    if (check_vendor_module() != 0) {
        return 0;
    }
    return 3;
}

static int shim_get_camera_info(int camera_id, struct camera_info *info) {
    if (check_vendor_module() != 0) {
        return -EINVAL;
    }
    
    if (camera_id == 0 || camera_id == 1) {
        return gVendorModule->get_camera_info(camera_id, info);
    } else if (camera_id == 2) {
        ALOGI("Intercepting get_camera_info for Aux camera (id 2) -> hardware id 1");
        int rc = gVendorGetCamInfo(1, info);
        if (rc == 0) {
            info->facing = CAMERA_FACING_BACK;
            info->device_version = CAMERA_DEVICE_API_VERSION_3_0;
        } else {
            ALOGE("Failed to get info for Aux camera (rc = %d)", rc);
        }
        return rc;
    }
    return -EINVAL;
}

static const camera_module_callbacks_t* g_callbacks = nullptr;

static int shim_set_callbacks(const camera_module_callbacks_t *callbacks) {
    g_callbacks = callbacks;
    if (check_vendor_module() != 0) {
        return -EINVAL;
    }
    if (gVendorModule->set_callbacks) {
        return gVendorModule->set_callbacks(callbacks);
    }
    return 0;
}

static void shim_get_vendor_tag_ops(vendor_tag_ops_t* ops) {
    if (check_vendor_module() != 0) {
        return;
    }
    if (gVendorModule->get_vendor_tag_ops) {
        gVendorModule->get_vendor_tag_ops(ops);
    }
}

static int shim_open(const hw_module_t* module, const char* id, hw_device_t** device) {
    if (check_vendor_module() != 0) {
        return -EINVAL;
    }

    int camera_id = atoi(id);
    ALOGI("shim_open camera %d", camera_id);
    
    if (camera_id == 2) {
        ALOGI("Opening Aux camera directly using internal symbols...");
        
        // Allocate a large buffer for the QCamera3HardwareInterface object
        // 16KB is extremely safe (the object is usually ~1KB or so).
        void* hw_obj = malloc(16384);
        if (!hw_obj) {
            ALOGE("Failed to allocate memory for QCamera3HardwareInterface");
            return -ENOMEM;
        }
        
        // Call the constructor: QCamera3HardwareInterface(cameraId = 1, callbacks)
        gVendorConstructor(hw_obj, 1, g_callbacks);
        
        // Call openCamera
        int rc = gVendorOpen(hw_obj, device);
        if (rc != 0) {
            ALOGE("Vendor openCamera failed for Aux camera (rc = %d)", rc);
            free(hw_obj);
            return rc;
        }
        
        ALOGI("Successfully opened Aux camera");
        return 0;
    }

    char vendor_id[10];
    snprintf(vendor_id, sizeof(vendor_id), "%d", camera_id);
    return gVendorModule->common.methods->open(&gVendorModule->common, vendor_id, device);
}

static hw_module_methods_t shim_module_methods = {
    .open = shim_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = CAMERA_MODULE_API_VERSION_2_4,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = CAMERA_HARDWARE_MODULE_ID,
        .name = "Meizu M1822 Camera Wrapper",
        .author = "Antigravity",
        .methods = &shim_module_methods,
        .dso = NULL,
        .reserved = {0},
    },
    .get_number_of_cameras = shim_get_number_of_cameras,
    .get_camera_info = shim_get_camera_info,
    .set_callbacks = shim_set_callbacks,
    .get_vendor_tag_ops = shim_get_vendor_tag_ops,
    .open_legacy = nullptr,
    .set_torch_mode = nullptr,
    .init = nullptr,
    .reserved = {0},
};
