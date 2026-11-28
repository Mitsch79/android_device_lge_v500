// SPDX-License-Identifier: Apache-2.0

#include <aidl/android/hardware/vibrator/BnVibrator.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

namespace aidl::android::hardware::vibrator {
namespace {

using ::android::base::unique_fd;
using ndk::ScopedAStatus;

constexpr char kInputPath[] = "/sys/devices/platform/soc/500000.qcom,ssbi/"
                              "500000.qcom,ssbi:pmic@0/"
                              "500000.qcom,ssbi:pmic@0:vibrator@4a/input/input*/event*";
constexpr char kInputName[] = "pm8xxx_vib_ffmemless";
constexpr int32_t kMaxDurationMs = 30000;

unique_fd openVibrator() {
    glob_t matches{};
    if (glob(kInputPath, GLOB_NOSORT, nullptr, &matches) != 0 || matches.gl_pathc != 1) {
        globfree(&matches);
        errno = ENODEV;
        return {};
    }
    const std::string path = "/dev/input/" + ::android::base::Basename(matches.gl_pathv[0]);
    globfree(&matches);

    unique_fd fd(TEMP_FAILURE_RETRY(open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW)));
    if (fd.ok()) {
        char name[sizeof(kInputName)]{};
        unsigned char bits[(FF_MAX + 8) / 8]{};
        if (ioctl(fd.get(), EVIOCGNAME(sizeof(name)), name) < 0 ||
            strncmp(name, kInputName, sizeof(name)) != 0 ||
            ioctl(fd.get(), EVIOCGBIT(EV_FF, sizeof(bits)), bits) < 0 ||
            !(bits[FF_RUMBLE / 8] & (1 << (FF_RUMBLE % 8)))) {
            fd.reset();
            errno = ENODEV;
        }
    }
    return fd;
}

ScopedAStatus unsupported() {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

class Vibrator final : public BnVibrator {
  public:
    ScopedAStatus getCapabilities(int32_t* capabilities) override {
        *capabilities = 0;
        return ScopedAStatus::ok();
    }

    ScopedAStatus on(int32_t timeoutMs, const std::shared_ptr<IVibratorCallback>&) override {
        if (timeoutMs <= 0) {
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }

        std::lock_guard lock(mutex_);
        if (!fd_.ok()) {
            fd_ = openVibrator();
            if (!fd_.ok()) {
                return ioError("open PM8921 vibrator");
            }
        }

        ff_effect effect{};
        effect.type = FF_RUMBLE;
        effect.id = effectId_;
        effect.replay.length = std::min(timeoutMs, kMaxDurationMs);
        effect.u.rumble.strong_magnitude = UINT16_MAX;
        if (TEMP_FAILURE_RETRY(ioctl(fd_.get(), EVIOCSFF, &effect)) < 0) {
            return ioError("upload PM8921 vibration effect");
        }

        effectId_ = effect.id;
        return play(true);
    }

    ScopedAStatus off() override {
        std::lock_guard lock(mutex_);
        return effectId_ < 0 ? ScopedAStatus::ok() : play(false);
    }

    ScopedAStatus getSupportedEffects(std::vector<Effect>* supported) override {
        supported->clear();
        return ScopedAStatus::ok();
    }
    ScopedAStatus perform(Effect, EffectStrength, const std::shared_ptr<IVibratorCallback>&,
                          int32_t*) override {
        return unsupported();
    }
    ScopedAStatus setAmplitude(float) override { return unsupported(); }
    ScopedAStatus setExternalControl(bool) override { return unsupported(); }
    ScopedAStatus getCompositionDelayMax(int32_t*) override { return unsupported(); }
    ScopedAStatus getCompositionSizeMax(int32_t*) override { return unsupported(); }
    ScopedAStatus getSupportedPrimitives(std::vector<CompositePrimitive>*) override {
        return unsupported();
    }
    ScopedAStatus getPrimitiveDuration(CompositePrimitive, int32_t*) override {
        return unsupported();
    }
    ScopedAStatus compose(const std::vector<CompositeEffect>&,
                          const std::shared_ptr<IVibratorCallback>&) override {
        return unsupported();
    }
    ScopedAStatus getSupportedAlwaysOnEffects(std::vector<Effect>*) override {
        return unsupported();
    }
    ScopedAStatus alwaysOnEnable(int32_t, Effect, EffectStrength) override { return unsupported(); }
    ScopedAStatus alwaysOnDisable(int32_t) override { return unsupported(); }
    ScopedAStatus getResonantFrequency(float*) override { return unsupported(); }
    ScopedAStatus getQFactor(float*) override { return unsupported(); }
    ScopedAStatus getFrequencyResolution(float*) override { return unsupported(); }
    ScopedAStatus getFrequencyMinimum(float*) override { return unsupported(); }
    ScopedAStatus getBandwidthAmplitudeMap(std::vector<float>*) override { return unsupported(); }
    ScopedAStatus getPwlePrimitiveDurationMax(int32_t*) override { return unsupported(); }
    ScopedAStatus getPwleCompositionSizeMax(int32_t*) override { return unsupported(); }
    ScopedAStatus getSupportedBraking(std::vector<Braking>*) override { return unsupported(); }
    ScopedAStatus composePwle(const std::vector<PrimitivePwle>&,
                              const std::shared_ptr<IVibratorCallback>&) override {
        return unsupported();
    }

  private:
    ScopedAStatus play(bool enabled) {
        input_event event{};
        event.type = EV_FF;
        event.code = effectId_;
        event.value = enabled;
        const ssize_t written = TEMP_FAILURE_RETRY(write(fd_.get(), &event, sizeof(event)));
        if (written != static_cast<ssize_t>(sizeof(event))) {
            if (written >= 0) errno = EIO;
            return ioError(enabled ? "start PM8921 vibration" : "stop PM8921 vibration");
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus ioError(const char* operation) {
        const int error = errno;
        PLOG(ERROR) << operation;
        fd_.reset();
        effectId_ = -1;
        return ScopedAStatus::fromServiceSpecificError(error);
    }

    std::mutex mutex_;
    unique_fd fd_;
    int16_t effectId_ = -1;
};

}  // namespace
}  // namespace aidl::android::hardware::vibrator

int main() {
    using aidl::android::hardware::vibrator::Vibrator;
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    const auto vibrator = ndk::SharedRefBase::make<Vibrator>();
    const std::string instance = std::string(Vibrator::descriptor) + "/default";
    if (AServiceManager_addService(vibrator->asBinder().get(), instance.c_str()) != STATUS_OK) {
        LOG(ERROR) << "Cannot register " << instance;
        return EXIT_FAILURE;
    }
    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;
}
