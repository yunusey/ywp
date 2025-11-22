#include "platform.h"
#include "wlr-layer-shell-client-protocol.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdbool.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <xkbcommon/xkbcommon.h>

PlatformData platform = {0};

static void HandleSurfaceEnter(void *data, struct wl_surface *surface, struct wl_output *output) {
    for (int i = 0; i < platform.monitorCount; i++) {
        if (platform.monitors[i].output == output) {
            platform.currentMonitorIndex = i;
            return;
        }
    }
}

static void HandleSurfaceLeave(void *data, struct wl_surface *surface, struct wl_output *output) {
    for (int i = 0; i < platform.monitorCount; i++) {
        if (platform.monitors[i].output == output) {
            platform.currentMonitorIndex = -1;
            return;
        }
    }
}

static const struct wl_surface_listener surfaceListener = {.enter = &HandleSurfaceEnter, .leave = &HandleSurfaceLeave};

static void LayerSurfaceConfigure(
    void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1, uint32_t serial, uint32_t width, uint32_t height) {
    zwlr_layer_surface_v1_ack_configure(platform.layer_surface, serial);
    wl_egl_window_resize(platform.egl_window, width, height, 0, 0);
}
static void LayerSurfaceClosed(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface_v1) {
}

static const struct zwlr_layer_surface_v1_listener layerSurfaceListener
    = {.configure = &LayerSurfaceConfigure, .closed = &LayerSurfaceClosed};

// --- Pointer Handlers ---
static void HandlePointerEnter(
    void *data, struct wl_pointer *poiner, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
    platform.cursor.input_serial = serial;
}
static void HandlePointerLeave(void *data, struct wl_pointer *p, uint32_t s, struct wl_surface *surf) {
}
static void HandlePointerMotion(
    void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    int x = wl_fixed_to_int(surface_x);
    int y = wl_fixed_to_int(surface_y);
}
static void HandlePointerButton(
    void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    platform.cursor.input_serial = serial;
}
static void
HandlePointerAxis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    // The 'value' is a wl_fixed_t (24.8 fixed-point number).
    // A standard mouse wheel tick is often reported as 10.0 (or 2560 in integer form).
    // We convert it to a float and normalize it so one "tick" becomes 1.0f.
    float scroll_amount     = (float) wl_fixed_to_double(value);
    float normalized_scroll = scroll_amount / 10.0f;

    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
    } else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
    }
}
static void HandlePointerFrame(void *data, struct wl_pointer *wl_pointer) {
}

static void HandlePointerAxisSource(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source) {
}

static void HandlePointerAxisStop(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis) {
}

static void HandlePointerAxisDiscrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete) {
}
// --- Pointer Listener Definition ---
static const struct wl_pointer_listener pointerListener = {
    .enter         = HandlePointerEnter,
    .leave         = HandlePointerLeave,
    .motion        = HandlePointerMotion,
    .button        = HandlePointerButton,
    .axis          = HandlePointerAxis,
    .frame         = HandlePointerFrame,
    .axis_source   = HandlePointerAxisSource,
    .axis_stop     = HandlePointerAxisStop,
    .axis_discrete = HandlePointerAxisDiscrete,
};

// --- Keyboard Handlers ---
static void HandleKeyboardKeymap(void *data, struct wl_keyboard *kb, uint32_t format, int32_t fd, uint32_t size) {
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    char *map_str = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_str == MAP_FAILED) {
        close(fd);
        return;
    }
    // Initialize the xkb context and state
    platform.xkb.context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    platform.xkb.keymap  = xkb_keymap_new_from_string(
        platform.xkb.context, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

    munmap(map_str, size);
    close(fd);

    platform.xkb.state = xkb_state_new(platform.xkb.keymap);
}
static void
HandleKeyboardEnter(void *data, struct wl_keyboard *kb, uint32_t s, struct wl_surface *surf, struct wl_array *keys) {
}
static void HandleKeyboardLeave(void *data, struct wl_keyboard *kb, uint32_t s, struct wl_surface *surf) {
}
static void
HandleKeyboardKey(void *data, struct wl_keyboard *kb, uint32_t s, uint32_t time, uint32_t key, uint32_t state) {
}
static void HandleKeyboardModifiers(
    void *data, struct wl_keyboard *kb, uint32_t s, uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group) {
    if (!platform.xkb.state)
        return;
    xkb_state_update_mask(platform.xkb.state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}
static void HandleKeyboardRepeatInfo(void *data, struct wl_keyboard *wl_keyboard, int32_t rate, int32_t delay) {
}

// --- Keyboard Listener Definition ---
static const struct wl_keyboard_listener keyboardListener = {
    .keymap      = HandleKeyboardKeymap,
    .enter       = HandleKeyboardEnter,
    .leave       = HandleKeyboardLeave,
    .key         = HandleKeyboardKey,
    .modifiers   = HandleKeyboardModifiers,
    .repeat_info = HandleKeyboardRepeatInfo,
};

// --- Seat Handler ---
static void HandleSeatCapabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !platform.pointer) {
        platform.pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(platform.pointer, &pointerListener, NULL);
    }
    if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && platform.pointer) {
        wl_pointer_release(platform.pointer);
        platform.pointer = NULL;
    }
    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !platform.keyboard) {
        platform.keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(platform.keyboard, &keyboardListener, NULL);
    }
    if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && platform.keyboard) {
        wl_keyboard_release(platform.keyboard);
        platform.keyboard = NULL;
    }
}
static void HandleSeatName(void *data, struct wl_seat *seat, const char *name) {
}

// --- Seat Listener Definition ---
static const struct wl_seat_listener seatListener = {
    .capabilities = HandleSeatCapabilities,
    .name         = HandleSeatName,
};

static void
HandleOutputMode(void *data, struct wl_output *output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
    MonitorData *monitor = data;
    if (flags & WL_OUTPUT_MODE_CURRENT) {
        monitor->width   = width;
        monitor->height  = height;
        monitor->refresh = refresh;
    }
}
static void HandleOutputScale(void *data, struct wl_output *output, int32_t factor) {
    MonitorData *monitor = data;
    monitor->scale       = factor;
}
static void HandleOutputGeometry(
    void *data, struct wl_output *output, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height,
    int32_t subpixel, const char *make, const char *model, int32_t transform) {
}
static void HandleOutputDone(void *data, struct wl_output *output) {
}
static void HandleOutputName(void *data, struct wl_output *output, const char *name) {
}
static void HandleOutputDescription(void *data, struct wl_output *output, const char *description) {
}

static const struct wl_output_listener outputListener
    = {.mode        = HandleOutputMode,
       .scale       = HandleOutputScale,
       .geometry    = HandleOutputGeometry,
       .done        = HandleOutputDone,
       .name        = HandleOutputName,
       .description = HandleOutputDescription};

static void
RegistryHandleGlobal(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        platform.compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        platform.layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        platform.seat = wl_registry_bind(registry, name, &wl_seat_interface, 7);
        wl_seat_add_listener(platform.seat, &seatListener, NULL);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        platform.cursor.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, wl_output_interface.name) == 0) {
        if (platform.monitorCount < MAX_MONITORS) {
            MonitorData *monitor = &platform.monitors[platform.monitorCount];
            monitor->output      = wl_registry_bind(registry, name, &wl_output_interface, 4);
            wl_output_add_listener(monitor->output, &outputListener, monitor);
            platform.monitorCount++;
        } else {
        }
    }
}
static void RegistryHandleGlobalRemove(void *data, struct wl_registry *registry, uint32_t name) {
}

// --- Registry Listener Definition ---
static const struct wl_registry_listener registryListener = {
    .global        = RegistryHandleGlobal,
    .global_remove = RegistryHandleGlobalRemove,
};

bool init_platform(void) {
    const EGLint framebufferAttribs[]
        = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
           EGL_NONE};
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    EGLint numConfigs = 0;

    platform.display = wl_display_connect(NULL);

    platform.registry = wl_display_get_registry(platform.display);
    wl_registry_add_listener(platform.registry, &registryListener, NULL);
    wl_display_roundtrip(platform.display);

    platform.egl.device = eglGetDisplay((EGLNativeDisplayType) platform.display);

    if (eglInitialize(platform.egl.device, NULL, NULL) == EGL_FALSE) {
        return false;
    }
    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        return false;
    }
    if (eglChooseConfig(platform.egl.device, framebufferAttribs, &platform.egl.config, 1, &numConfigs) == EGL_FALSE
        || numConfigs == 0) {
        return false;
    }
    platform.egl.context = eglCreateContext(platform.egl.device, platform.egl.config, EGL_NO_CONTEXT, contextAttribs);
    if (platform.egl.context == EGL_NO_CONTEXT) {
        return -1;
    }

    platform.surface = wl_compositor_create_surface(platform.compositor);
    wl_surface_add_listener(platform.surface, &surfaceListener, NULL);
    if (platform.surface == NULL) {
        return -1;
    }

    platform.cursor.theme = wl_cursor_theme_load(NULL, CURSOR_SIZE, platform.cursor.shm);
    if (!platform.cursor.theme) {
    } else {
        platform.cursor.surface = wl_compositor_create_surface(platform.compositor);
    }
    platform.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        platform.layer_shell, platform.surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND, "wayland_app");

    int screen_width = 800, screen_height = 600;

    zwlr_layer_surface_v1_set_size(platform.layer_surface, screen_width, screen_height);
    zwlr_layer_surface_v1_add_listener(platform.layer_surface, &layerSurfaceListener, &platform);
    zwlr_layer_surface_v1_set_keyboard_interactivity(
        platform.layer_surface,
        ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);  // No keyboard interactivity--for now ;)
    zwlr_layer_surface_v1_set_exclusive_zone(platform.layer_surface, -1);

    platform.egl_window  = wl_egl_window_create(platform.surface, screen_width, screen_height);
    platform.egl.surface = eglCreateWindowSurface(
        platform.egl.device, platform.egl.config, (EGLNativeWindowType) platform.egl_window, NULL);
    wl_surface_commit(platform.surface);
    wl_display_roundtrip(platform.display);

    eglSwapInterval(platform.egl.device, 1);

    EGLBoolean result
        = eglMakeCurrent(platform.egl.device, platform.egl.surface, platform.egl.surface, platform.egl.context);
    if (result != EGL_FALSE) {
    } else {
        return -1;
    }

    return 0;
}

// Close platform
bool close_platform(void) {
    if (platform.xkb.state)
        xkb_state_unref(platform.xkb.state);
    if (platform.xkb.context)
        xkb_context_unref(platform.xkb.context);
    if (platform.xkb.keymap)
        xkb_keymap_unref(platform.xkb.keymap);

    if (platform.keyboard)
        wl_keyboard_release(platform.keyboard);
    if (platform.pointer)
        wl_pointer_release(platform.pointer);
    if (platform.seat)
        wl_seat_release(platform.seat);

    if (platform.layer_surface)
        zwlr_layer_surface_v1_destroy(platform.layer_surface);

    if (platform.egl.device) {
        eglMakeCurrent(platform.egl.device, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (platform.egl.surface)
            eglDestroySurface(platform.egl.device, platform.egl.surface);

        if (platform.egl.context)
            eglDestroyContext(platform.egl.device, platform.egl.context);

        eglTerminate(platform.egl.device);
    }

    if (platform.egl_window)
        wl_egl_window_destroy(platform.egl_window);

    if (platform.surface)
        wl_surface_destroy(platform.surface);

    if (platform.layer_shell)
        zwlr_layer_shell_v1_destroy(platform.layer_shell);
    if (platform.compositor)
        wl_compositor_destroy(platform.compositor);
    if (platform.registry)
        wl_registry_destroy(platform.registry);

    if (platform.display) {
        wl_display_flush(platform.display);
        wl_display_disconnect(platform.display);
    }

    return true;
}
