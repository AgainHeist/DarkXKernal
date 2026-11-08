#include "Android_draw/draw.h"
#include "ui/Font.h"
#include "ui/Icon.h"
#include "fonts/FontAwesome6_solid.h"

EGLDisplay display = EGL_NO_DISPLAY;
EGLConfig config;
EGLSurface surface = EGL_NO_SURFACE;
EGLContext context = EGL_NO_CONTEXT;

ANativeWindow *native_window;
ImFont* fontBold;
ImFont* fontMedium;
ImFont* fontDesc;
ImFont* espFont;

int native_window_screen_x = 0;
int native_window_screen_y = 0;
android::ANativeWindowCreator::DisplayInfo displayInfo{0};
uint32_t orientation = 0;
bool g_Initialized = false;
ImGuiWindow *g_window = nullptr;

static ImVector<ImWchar> mega_ranges;

void screen_config() {
    displayInfo = android::ANativeWindowCreator::GetDisplayInfo();
}

bool initGUI_draw(uint32_t _screen_x, uint32_t _screen_y, bool log) {
    orientation = displayInfo.orientation;
    if (!init_egl(_screen_x, _screen_y, log)) return false;
    if (!ImGui_init()) return false;
    return true;
}

bool init_egl(uint32_t _screen_x, uint32_t _screen_y, bool log) {
    // Generate a random name for the overlay to avoid detection via 'dumpsys SurfaceFlinger'
    char randomName[16];
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand(time(NULL));
    for (int i = 0; i < 12; i++) {
        randomName[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    randomName[12] = '\0';

    // Native Surface Drawing - Bypass SYSTEM_ALERT_WINDOW detection
    // Works on Magisk, APatch, KernelSU etc.
    ::native_window = android::ANativeWindowCreator::Create(randomName, _screen_x, _screen_y, true);

    ANativeWindow_acquire(native_window);
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) return false;
    if (eglInitialize(display, 0, 0) != EGL_TRUE) return false;

    EGLint num_config = 0;
    const EGLint attribList[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_BLUE_SIZE, 5, EGL_GREEN_SIZE, 6, EGL_RED_SIZE, 5,
        EGL_BUFFER_SIZE, 32, EGL_DEPTH_SIZE, 16, EGL_STENCIL_SIZE, 8, EGL_NONE
    };
    const EGLint attrib_list[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };

    if (eglChooseConfig(display, attribList, &config, 1, &num_config) != EGL_TRUE) return false;

    EGLint egl_format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &egl_format);
    ANativeWindow_setBuffersGeometry(native_window, 0, 0, egl_format);
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrib_list);
    if (context == EGL_NO_CONTEXT) return false;

    surface = eglCreateWindowSurface(display, config, (EGLNativeWindowType)native_window, nullptr);
    if (surface == EGL_NO_SURFACE) return false;

    if (!eglMakeCurrent(display, surface, surface, context)) return false;
    return true;
}

bool ImGui_init() {
    if (g_Initialized) return true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplAndroid_Init(native_window);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.Fonts->Clear(); 

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false; 

    // 🔥 MEGA RANGES (Duniya bhar ke saare symbols aur Thai letters)
    mega_ranges.clear();
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
    builder.AddRanges(io.Fonts->GetGlyphRangesThai());
    builder.AddRanges(io.Fonts->GetGlyphRangesVietnamese());
    
    static const ImWchar custom_symbols[] = { 
        0x0100, 0x024F, 0x2000, 0x206F, 0x2100, 0x218F, 
        0x2500, 0x257F, 0x2600, 0x26FF, 0x2700, 0x27BF, 
        0x2800, 0x28FF, 0 
    };
    builder.AddRanges(custom_symbols);
    builder.BuildRanges(&mega_ranges); 

    ImFontConfig icons_config;
    icons_config.MergeMode = true;         
    icons_config.PixelSnapH = true;
    icons_config.FontDataOwnedByAtlas = false; 
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 }; 

    // Menu Fonts Load (UI ke liye Oppo Font)
    fontBold = io.Fonts->AddFontFromMemoryTTF((void*)OPPOSans_H, OPPOSans_H_size, 32.0f, &font_cfg, mega_ranges.Data);
    if (fontBold != nullptr) {
        io.Fonts->AddFontFromMemoryCompressedTTF((void*)fa_solid_compressed_data, fa_solid_compressed_size, 32.0f, &icons_config, icons_ranges);
    }

    fontMedium = io.Fonts->AddFontFromMemoryTTF((void*)OPPOSans_H, OPPOSans_H_size, 28.0f, &font_cfg, mega_ranges.Data);
    if (fontMedium != nullptr) {
        io.Fonts->AddFontFromMemoryCompressedTTF((void*)fa_solid_compressed_data, fa_solid_compressed_size, 28.0f, &icons_config, icons_ranges);
    }

    fontDesc = io.Fonts->AddFontFromMemoryTTF((void*)OPPOSans_H, OPPOSans_H_size, 22.0f, &font_cfg, mega_ranges.Data);
    
    // =========================================================================
    // 🔥 THE REAL FIX: Yahan 'Arial_Unicode_MS_ttf' load ho raha hai jo aapne dala hai
    // =========================================================================
    ImFontConfig esp_cfg;
    esp_cfg.FontDataOwnedByAtlas = false;
   // espFont = io.Fonts->AddFontFromMemoryTTF((void*)Arial_Unicode_MS_ttf, sizeof(Arial_Unicode_MS_ttf), 18.0f, &esp_cfg, mega_ranges.Data);

    if (fontMedium == nullptr) {
        fontMedium = io.Fonts->AddFontDefault();
        fontBold = io.Fonts->AddFontDefault();
        fontDesc = io.Fonts->AddFontDefault();
        if (espFont == nullptr) espFont = io.Fonts->AddFontDefault();
    }
    
    io.FontDefault = fontMedium;
    io.Fonts->Build(); 

    ImGui::GetStyle().ScaleAllSizes(3.0f);
    ::g_Initialized = true;
    return true;
}

void drawBegin() {
    screen_config();
    if (::orientation != displayInfo.orientation) {
        ::orientation = displayInfo.orientation;
        touch::update(displayInfo.width, displayInfo.height, displayInfo.orientation);
        if (g_window) {
            g_window->Pos.x = 100;
            g_window->Pos.y = 125;
        }
    }
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(native_window_screen_x, native_window_screen_y);
    ImGui::NewFrame();
}

void drawEnd() {
    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(display, surface);
}

void shutdown() {
    if (!g_Initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;
    ANativeWindow_release(native_window);
    android::ANativeWindowCreator::Destroy(native_window);
    ::g_Initialized = false;
}