# LG G Pad 8.3 Wi-Fi (V500)

LineageOS 21 ARMv7 product configuration for the APQ8064 tablet. The product uses the LineageOS setup wizard and Trebuchet. The manifest pins the launcher and documents the upstream unlock callback backport. The display target is 1200 × 1920 with Mesa Freedreno on Adreno 320.

Physical touch setup has reached HOME, and the launcher has started again with the same data after a reboot. The V500 tablet profile uses 280 logical dpi for its 8.3-inch panel, which has a physical density of about 273 ppi. The vendor overlay adjusts portrait setup margins. Frame times and display transitions remain under evaluation. M6 uses loop-backed test images and a timed return to recovery.

Use the [V500 manifest and build instructions](https://github.com/Mitsch79/android_local_manifests_v500/tree/lineage-21.0).

### Precompile the setup wizard's graphics programs

Build the training image from the target system and vendor revisions before creating `.v500-wizard-shaders`. On a V500 with fresh, isolated setup data, open the language picker from Welcome and follow the normal wizard pages through navigation and the final page. Leave the wizard running for several more seconds so HWUI and EGL can finish saving their caches. The native OpenGL ES driver builds its variants during this run.

On a userdebug training build, copy the two generated files into the device checkout before building the distribution image. Set `ANDROID_SERIAL` to the training tablet. Run these commands from the Android source root.

```sh
adb root
adb wait-for-device
test "$(adb shell getprop ro.product.device | tr -d '\r')" = v500
seed=device/lge/v500/.v500-wizard-shaders
mkdir -p "$seed"
for file in com.android.opengl.shaders_cache com.android.skia.shaders_cache; do
    adb pull "/data/user_de/0/org.lineageos.setupwizard/cache/$file" "$seed/$file"
done
sha256sum "$seed"/*
m systemimage
```

The product installs the pair under `/system_ext/etc/setupwizard-shaders/`. On first launch the V500 wizard initializes missing cache files in its own device-protected cache directory. Skia loads GL program binaries and the driver validates their compatibility. Record the system and vendor revisions with the hashes and regenerate the pack when either graphics implementation changes. The generated files are excluded from Git.
