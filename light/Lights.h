/*
 * Copyright (C) 2021-2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/light/BnLights.h>
#include <mutex>
#include <vector>

using ::aidl::android::hardware::light::HwLight;
using ::aidl::android::hardware::light::HwLightState;

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class Lights : public BnLights {
  public:
    Lights();

    ndk::ScopedAStatus setLightState(int32_t id, const HwLightState& state) override;
    ndk::ScopedAStatus getLights(std::vector<HwLight>* _aidl_return) override;

  private:
    void handleBacklight(uint8_t brightness);
    void handleNotification(const HwLightState& state);

    std::mutex mLock;
    HwLightState mBatteryState;
    HwLightState mNotificationState;
    HwLightState mAttentionState;
    std::vector<HwLight> mLights;
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
