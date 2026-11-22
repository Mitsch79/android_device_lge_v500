$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)
$(call inherit-product, vendor/lineage/config/common_mini_tablet_wifionly.mk)
$(call inherit-product, frameworks/native/build/tablet-7in-xhdpi-2048-dalvik-heap.mk)

PRODUCT_NAME := lineage_v500
PRODUCT_DEVICE := v500
PRODUCT_BRAND := lge
PRODUCT_MANUFACTURER := LG
PRODUCT_MODEL := LG-V500
PRODUCT_CHARACTERISTICS := tablet
DEVICE_PACKAGE_OVERLAYS += vendor/lge/v500/overlay
PRODUCT_PACKAGES += V500NavigationDefaults
PRODUCT_PACKAGES += lights.v500 android.hardware.light@2.0-service android.hardware.light@2.0-impl
PRODUCT_USE_DYNAMIC_PARTITIONS := false
PRODUCT_SHIPPING_API_LEVEL := 28
PRODUCT_BUILD_BOOT_IMAGE := false
PRODUCT_BUILD_VENDOR_BOOT_IMAGE := false
PRODUCT_BUILD_USERDATA_IMAGE := false
PRODUCT_BUILD_RECOVERY_IMAGE := false
# Match the tested ARMv7 kernel, which reports no userfaultfd GC support.
PRODUCT_ENABLE_UFFD_GC := false
# Keep APEX payloads ready to mount during first boot on SDCC1 PIO.
PRODUCT_COMPRESSED_APEX := false
# Settings supplies the lock chooser and other pages shown during first-time setup.
PRODUCT_DEXPREOPT_SPEED_APPS += LineageSetupWizard Settings
PRODUCT_PACKAGES += init_first_stage
PRODUCT_VENDOR_PROPERTIES += ro.config.low_ram=true

PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator@4.0-service.minigbm_msm \
    android.hardware.graphics.mapper@4.0-impl.minigbm_msm \
    android.hardware.graphics.composer@2.1-service \
    gralloc.minigbm_msm \
    hwcomposer.drm_minigbm

ifneq ($(wildcard device/lge/v500/mesa23.mk),)
PRODUCT_PACKAGES += libgallium_dri libglapi libEGL_mesa libGLESv1_CM_mesa libGLESv2_mesa
else
PRODUCT_SOONG_NAMESPACES += external/mesa3d
PRODUCT_PACKAGES += libGLES_mesa
endif

# Software services for Android 14 bring-up.
PRODUCT_PACKAGES += \
    android.hardware.security.keymint-service \
    android.hardware.gatekeeper@1.0-service.software \
    android.hardware.health-service.example

# Generic audio HAL for boot until the V500 audio hardware is integrated.
PRODUCT_PACKAGES += \
    android.hardware.audio.service \
    android.hardware.audio@6.0-impl:32 \
    android.hardware.audio.effect@6.0-impl:32 \
    audio.primary.default \
    audio.r_submix.default

PRODUCT_COPY_FILES += \
    frameworks/av/services/audiopolicy/config/audio_policy_configuration_generic.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/primary_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/primary_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/services/audiopolicy/config/surround_sound_configuration_5_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/surround_sound_configuration_5_0.xml \
    device/lge/v500/fstab.v500:$(TARGET_COPY_OUT_RAMDISK)/fstab.v500 \
    device/lge/v500/fstab.v500:$(TARGET_COPY_OUT_VENDOR)/etc/fstab.v500 \
    device/lge/v500/init.v500.rc:$(TARGET_COPY_OUT_VENDOR)/etc/init/init.v500.rc \
    vendor/lge/v500/sysconfig/v500-setupwizard.xml:$(TARGET_COPY_OUT_VENDOR)/etc/sysconfig/v500-setupwizard.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.software.app_widgets.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.app_widgets.xml \
    frameworks/native/data/etc/android.software.secure_lock_screen.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.secure_lock_screen.xml

# Keep the initial SystemUI dexopt within the recovery timer on SDCC1 PIO.
PRODUCT_VENDOR_PROPERTIES += \
    dalvik.vm.systemuicompilerfilter=verify

# The optional training pack is generated with the product's HWUI and Mesa.
ifneq ($(wildcard device/lge/v500/.v500-wizard-shaders),)
PRODUCT_COPY_FILES += \
    device/lge/v500/.v500-wizard-shaders/com.android.opengl.shaders_cache:$(TARGET_COPY_OUT_SYSTEM_EXT)/etc/setupwizard-shaders/com.android.opengl.shaders_cache \
    device/lge/v500/.v500-wizard-shaders/com.android.skia.shaders_cache:$(TARGET_COPY_OUT_SYSTEM_EXT)/etc/setupwizard-shaders/com.android.skia.shaders_cache
endif

# MDP4 requires linear scanout buffers on the V500.
PRODUCT_VENDOR_PROPERTIES += \
    ro.hardware.egl=mesa \
    ro.hardware.gralloc=minigbm_msm \
    ro.hardware.hwcomposer=drm_minigbm \
    vendor.minigbm.debug=nocompression \
    persist.sys.usb.config=adb

ifneq ($(wildcard device/lge/v500/.v500-half-client),)
PRODUCT_VENDOR_PROPERTIES += vendor.hwc.backend_override=v500-half-client ro.sf.lcd_density=140
else
# The 8.3-inch panel is approximately 273 ppi.
PRODUCT_VENDOR_PROPERTIES += vendor.hwc.backend_override=client ro.sf.lcd_density=280
endif
