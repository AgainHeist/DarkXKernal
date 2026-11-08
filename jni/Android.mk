LOCAL_PATH := $(call my-dir)

# =========================================================
# 1. EXTERNAL LIBRARIES (CURL, SSL, CRYPTO)
# =========================================================

# --- CURL ---
include $(CLEAR_VARS)
LOCAL_MODULE := curl_prebuilt
LOCAL_SRC_FILES := includes/external/curl-android-$(TARGET_ARCH_ABI)/lib/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/includes/external/curl-android-$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)

# --- SSL ---
include $(CLEAR_VARS)
LOCAL_MODULE := ssl_prebuilt
LOCAL_SRC_FILES := includes/external/openssl-android-$(TARGET_ARCH_ABI)/lib/libssl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/includes/external/openssl-android-$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)

# --- CRYPTO ---
include $(CLEAR_VARS)
LOCAL_MODULE := crypto_prebuilt
LOCAL_SRC_FILES := includes/external/openssl-android-$(TARGET_ARCH_ABI)/lib/libcrypto.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/includes/external/openssl-android-$(TARGET_ARCH_ABI)/include
include $(PREBUILT_STATIC_LIBRARY)

# =========================================================
# 2. MAIN EXECUTABLE (MASKED AS SYSTEM SERVICE)
# =========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := android.system.svc

# 🔥 FIX 1: Removed '../' because includes is now inside jni/
LOCAL_SRC_FILES := \
    src/main.cpp \
    src/ui/menu.cpp \
    src/ui/bar.cpp \
    src/ui/widgets/widgets.cpp \
    src/protect/oxorany.cpp \
    src/game/data.cpp \
    src/func/icons.cpp \
    src/func/KernelTools.cpp \
    includes/draw/Android_draw/draw.cpp \
    includes/draw/Android_touch/Touch.cpp \
    includes/draw/ImGui/imgui.cpp \
    includes/draw/ImGui/imgui_draw.cpp \
    includes/draw/ImGui/imgui_tables.cpp \
    includes/draw/ImGui/imgui_widgets.cpp \
    includes/draw/ImGui/backends/imgui_impl_android.cpp \
    includes/draw/ImGui/backends/imgui_impl_opengl3.cpp

# 🔥 FIX 2: Added ALL ImGui folders so imgui.h is found correctly
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/includes \
    $(LOCAL_PATH)/includes/fonts \
    $(LOCAL_PATH)/includes/internal \
    $(LOCAL_PATH)/includes/internal/ImGui \
    $(LOCAL_PATH)/includes/internal/ImGui/backends \
    $(LOCAL_PATH)/includes/draw/ImGui \
    $(LOCAL_PATH)/includes/draw/ImGui/backends \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/src/ui \
    $(LOCAL_PATH)/includes/external/curl-android-$(TARGET_ARCH_ABI)/include \
    $(LOCAL_PATH)/includes/external/openssl-android-$(TARGET_ARCH_ABI)/include

# 🔥 FIX: Removed '-flto' from here
# 🟢 NEW FIX: Added '-fexceptions' at the end of LOCAL_CPPFLAGS
LOCAL_CPPFLAGS := -std=c++17 -fpermissive -Wno-unknown-warning-option -Wno-error -fPIE \
    -fvisibility=hidden -fvisibility-inlines-hidden \
    -Oz -ffunction-sections -fdata-sections -fomit-frame-pointer \
    -fno-unwind-tables -fno-asynchronous-unwind-tables -fmerge-all-constants \
    -fno-ident -fno-rtti -Wno-error=format-security \
    -Wno-nontrivial-memcall -Wno-deprecated-literal-operator \
    -fexceptions

# 🟢 NEW FIX: Enabled C++ Exceptions feature for JSON parsing
LOCAL_CPP_FEATURES += exceptions

# 🔥 FIX: Removed '-flto' from here too
LOCAL_LDFLAGS := -pie -Wl,--gc-sections -Wl,--strip-all -Wl,--build-id=none \
    -Wl,--hash-style=gnu -Wl,--no-undefined -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

# Libraries ko connect karna
LOCAL_STATIC_LIBRARIES := curl_prebuilt ssl_prebuilt crypto_prebuilt
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3 -lz -lm -ldl -lc

include $(BUILD_EXECUTABLE)