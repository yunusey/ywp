#ifndef PLATFORM_H
#define PLATFORM_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdint.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-egl.h>
#include <xkbcommon/xkbcommon.h>

#define NUM_CURSORS 11
#define MAX_MONITORS 4
#define CURSOR_SIZE 24

typedef struct
{
    struct wl_output *output;
    int width;
    int height;
    int refresh;
    int scale;
} MonitorData;

typedef struct
{
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_egl_window *egl_window;
    struct wl_registry *registry;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    struct wl_keyboard *keyboard;

    struct
    {
        struct wl_shm *shm;
        struct wl_cursor_theme *theme;
        struct wl_surface *surface;
        struct wl_cursor *cursors[NUM_CURSORS]; // The number of `MouseCursor`s
        uint32_t input_serial;
    } cursor;

    int monitorCount;
    int currentMonitorIndex;
    MonitorData monitors[MAX_MONITORS];

    struct zwlr_layer_shell_v1 *layer_shell;
    struct zwlr_layer_surface_v1 *layer_surface;

    struct
    {
        struct xkb_context *context;
        struct xkb_keymap *keymap;
        struct xkb_state *state;
    } xkb;

    struct
    {
        EGLDisplay device;  // Native display device (physical screen connection)
        EGLSurface surface; // Surface to draw on, framebuffers (connected to context)
        EGLContext context; // Graphic context, mode in which drawing can be done
        EGLConfig config;   // Graphic config
    } egl;
} PlatformData;

typedef struct CoreData
{
    struct window_size
    {
        int width;
        int height;
    } window_size;
} CoreData;

// Similar to rayib's pattern
extern PlatformData platform;
extern CoreData core;

bool init_platform();
bool close_platform();

#endif // PLATFORM_H
