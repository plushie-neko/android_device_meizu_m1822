/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "android.hardware.light-service.meizu_m1721"

#include "Lights.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <fstream>

namespace {

using ::android::base::WriteStringToFile;

constexpr const char* kLcdBacklightPath = "/sys/class/leds/lcd-backlight/brightness";
constexpr const char* kLedBrightnessPath = "/sys/class/leds/mx-led/brightness";
constexpr const char* kLedBlinkPath = "/sys/class/leds/mx-led/blink";

constexpr uint32_t kMaxLcdBrightness = 255;

static uint32_t rgbToBrightness(uint32_t color) {
    uint8_t alpha = (color >> 24) & 0xFF;
    uint8_t red = (color >> 16) & 0xFF;
    uint8_t green = (color >> 8) & 0xFF;
    uint8_t blue = color & 0xFF;

    red = red * alpha / 0xFF;
    green = green * alpha / 0xFF;
    blue = blue * alpha / 0xFF;

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

static uint32_t scaleBrightness(uint32_t brightness, uint32_t maxBrightness) {
    if (brightness == 0) return 0;
    return (brightness - 1) * (maxBrightness - 1) / (0xFF - 1) + 1;
}

static void writeInt(const std::string& path, uint32_t value) {
    if (!WriteStringToFile(std::to_string(value), path)) {
        LOG(WARNING) << "failed to write " << value << " to " << path;
    }
}

#define AutoHwLight(light) \
    { .id = (int32_t)light, .type = light, .ordinal = 0 }

}  // anonymous namespace

namespace aidl {
namespace android {
namespace hardware {
namespace light {

Lights::Lights() {
    mLights.push_back(AutoHwLight(LightType::BACKLIGHT));
    mLights.push_back(AutoHwLight(LightType::BATTERY));
    mLights.push_back(AutoHwLight(LightType::NOTIFICATIONS));
    mLights.push_back(AutoHwLight(LightType::ATTENTION));
}

ndk::ScopedAStatus Lights::setLightState(int32_t id, const HwLightState& state) {
    LightType type = static_cast<LightType>(id);

    std::lock_guard<std::mutex> lock(mLock);

    switch (type) {
        case LightType::BACKLIGHT:
            handleBacklight(rgbToBrightness(state.color));
            break;
        case LightType::BATTERY:
            mBatteryState = state;
            break;
        case LightType::NOTIFICATIONS:
            mNotificationState = state;
            break;
        case LightType::ATTENTION:
            mAttentionState = state;
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    handleNotification(mNotificationState.color ? mNotificationState
                       : mAttentionState.color  ? mAttentionState
                                                : mBatteryState);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* _aidl_return) {
    *_aidl_return = mLights;
    return ndk::ScopedAStatus::ok();
}

void Lights::handleBacklight(uint8_t brightness) {
    writeInt(kLcdBacklightPath, scaleBrightness(brightness, kMaxLcdBrightness));
}

void Lights::handleNotification(const HwLightState& state) {
    uint32_t brightness = rgbToBrightness(state.color);
    uint8_t blink = 0;

    if (state.flashMode == FlashMode::TIMED && state.flashOnMs > 0 && state.flashOffMs > 0) {
        blink = (state.flashOnMs == state.flashOffMs) ? 2 : 1;
    }

    writeInt(kLedBlinkPath, 0);

    if (blink) {
        if (brightness) writeInt(kLedBlinkPath, blink);
    } else {
        writeInt(kLedBrightnessPath, brightness);
    }
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
