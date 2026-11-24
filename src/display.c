/* display.c */

#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "terminal.h"
#include "display.h"

//////// DATA ////////
static DisplayBackend d_backend = DISPLAY_UNKNOWN;
int isUsingDirectDisplay = FALSE; //Should use this set of functions?
//////// DATA ////////


//////// WAYLAND ////////
//////////// HELPERS ////////////
#define _GNU_SOURCE

#include <wayland-client.h>
#include ".xdg/xdg-shell-client-protocol.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static struct {
	struct wl_display*   display;
	struct wl_registry*  registry;

	struct wl_compositor* compositor;
	struct wl_shm*        shm;

	struct xdg_wm_base*  wm_base;
	struct wl_surface*   surface;
	struct xdg_surface*  xdg_surface;
	struct xdg_toplevel* toplevel;

	struct wl_buffer* buffer;

	void* shm_data;
	size_t shm_size;

	int width;
	int height;
	int stride;

	int configured;
	int running;
	int buffer_busy;

} wl = {0};



static void wl_registry_global(
	void* data, struct wl_registry* registry,
	uint32_t name, const char* interface, uint32_t version
) {
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		uint32_t bindVersion = version < 4 ? version : 4;
		wl.compositor = wl_registry_bind(
		    registry, name, &wl_compositor_interface, bindVersion
		);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		wl.shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		wl.wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
	}
}

static void wl_registry_remove(void* data, struct wl_registry* registry, uint32_t name){/* Not necessary to implement */}
static const struct wl_registry_listener registry_listener = {.global=wl_registry_global, .global_remove=wl_registry_remove};



static void wl_ping(void* data, struct xdg_wm_base* wmBase, uint32_t serial){xdg_wm_base_pong(wmBase, serial);}
static const struct xdg_wm_base_listener wmBaseListener = {.ping=wl_ping};


static void wl_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial) {
	xdg_surface_ack_configure(surface, serial);
	wl.configured = 1;
}

static const struct xdg_surface_listener xdg_surface_listener = {.configure=wl_surface_configure};

static void wl_toplevel_configure(
	void* data, struct xdg_toplevel* toplevel,
	int32_t width, int32_t height, struct wl_array* states
) {
	if (width > 0) {wl.width = width;}
	if (height > 0) {wl.height = height;}
}

static void wl_toplevel_close(void* data, struct xdg_toplevel* toplevel) {wl.running = 0;}
static const struct xdg_toplevel_listener xdg_toplevel_listener = {.configure=wl_toplevel_configure, .close=wl_toplevel_close};


static int wl_createFramebuffer(void) {
	wl.stride = wl.width * 4;
	wl.shm_size = (size_t)(wl.stride) * wl.height;

	int fd = memfd_create("my-framebuffer", MFD_CLOEXEC);
	if (fd < 0) {return FALSE;}

	if (ftruncate(fd, (off_t)wl.shm_size) < 0) {
		close(fd);
		return FALSE;
	}

	wl.shm_data = mmap(
		NULL, wl.shm_size,
		PROT_READ | PROT_WRITE, MAP_SHARED,
		fd, 0
	);

	if (wl.shm_data == MAP_FAILED) {
		wl.shm_data = NULL;
		close(fd);
		return FALSE;
	}

	struct wl_shm_pool* pool = wl_shm_create_pool(wl.shm, fd, (int32_t)(wl.shm_size));

	if (!pool) {
		munmap(wl.shm_data, wl.shm_size);
		wl.shm_data = NULL;
		close(fd);
		return FALSE;
	}

	wl.buffer = wl_shm_pool_create_buffer(
		pool, 0,
		wl.width, wl.height, wl.stride,
		WL_SHM_FORMAT_XRGB8888 //32b, _/R/G/B data. unused 8b at start.
	);

	wl_shm_pool_destroy(pool);
	close(fd);

	if (!wl.buffer) {
		munmap(wl.shm_data, wl.shm_size);
		wl.shm_data = NULL;
		return FALSE;
	}

	wl_buffer_add_listener(wl.buffer, &buffer_listener, NULL);

	return TRUE;
}


static void wl_buffer_release(void* data, struct wl_buffer* buffer) {wl.buffer_busy = 0;}
static const struct wl_buffer_listener buffer_listener = {.release=wl_buffer_release};
//////////// HELPERS ////////////


//Func namedef thingys before they're actually defined. I forget the real name for this.
int d_shutdownWayland(void);
int d_destroyWindowWayland(void);

int d_initWayland(void) {
	wl.display = wl_display_connect(NULL);

	if (!wl.display) {
		terminal_error("Failed to connect to Wayland");
		return FALSE;
	}

	wl.registry = wl_display_get_registry(wl.display);
	wl_registry_add_listener(wl.registry, &registry_listener, NULL);

	if (wl_display_roundtrip(wl.display) < 0) {goto fail;}
	if ((!wl.compositor) || (!wl.shm) || (!wl.wm_base)) {goto fail;}

	xdg_wm_base_add_listener(wl.wm_base, &wmBaseListener, NULL);

	wl.running = 1;

	return TRUE;

fail:
	d_shutdownWayland();
	terminal_error("Failed to initialise Wayland interfaces");
	return FALSE;
}


int d_isRunningWayland(void) {
	return (wl.display && wl.compositor && wl.running);
}


int d_shutdownWayland(void) {
	d_destroyWindowWayland();

	if (wl.display) {wl_display_disconnect(wl.display);}
	wl.display = NULL;

	if (wl.shm_data) {
		munmap(wl.shm_data, wl.shm_size);
		wl.shm_data = NULL;
	}

	if (wl.registry) {wl_registry_destroy(wl.registry);}
	if (wl.wm_base) {xdg_wm_base_destroy(wl.wm_base);}
	if (wl.shm) {wl_shm_destroy(wl.shm);}
	if (wl.compositor) {wl_compositor_destroy(wl.compositor);}

	memset(&wl, 0, sizeof(wl));

	return TRUE;
}


int d_createWindowWayland(const Vec2i_t resolution, const char* title) {
	wl.width = resolution.x; wl.height = resolution.y;

	wl.surface = wl_compositor_create_surface(wl.compositor);
	if (!wl.surface) {return FALSE;}

	wl.xdg_surface = xdg_wm_base_get_xdg_surface(wl.wm_base, wl.surface);
	if (!wl.xdg_surface) {return FALSE;}

	xdg_surface_add_listener(wl.xdg_surface, &xdg_surface_listener, NULL);

	wl.toplevel = xdg_surface_get_toplevel(wl.xdg_surface);
	if (!wl.toplevel) {return FALSE;}

	xdg_toplevel_add_listener(wl.toplevel, &xdg_toplevel_listener, NULL);
	xdg_toplevel_set_title(wl.toplevel, title);

	//Trigger initial config with empty surface
	wl_surface_commit(wl.surface);
	//Wait for the thingy's config event to happen
	while (!wl.configured) {
		if (wl_display_roundtrip(wl.display) < 0) {return FALSE;}
	}
	if (!wl_createFramebuffer()) {return FALSE;}

	return TRUE;
}


int d_destroyWindowWayland(void) {
	if (wl.surface) {wl_surface_attach(wl.surface, NULL, 0, 0);}
	if (wl.surface) {wl_surface_commit(wl.surface);}
	
	if (wl.toplevel) {
		xdg_toplevel_destroy(wl.toplevel);
		wl.toplevel = NULL;
	}

	if (wl.xdg_surface) {
		xdg_surface_destroy(wl.xdg_surface);
		wl.xdg_surface = NULL;
	}

	if (wl.surface) {
		wl_surface_destroy(wl.surface);
		wl.surface = NULL;
	}

	if (wl.buffer) {
		wl_buffer_destroy(wl.buffer);
		wl.buffer = NULL;
	}

	wl.configured = 0;
	wl.buffer_busy = 0;

	return TRUE;
}


int d_pollWindowWayland(void) {
	if (!wl.display) {return FALSE;}
	if (wl_display_dispatch_pending(wl.display) < 0) {return FALSE; /* Process events already received */} 

	//Prepare to read
	while (wl_display_prepare_read(wl.display) != 0) {
		if (wl_display_dispatch_pending(wl.display) < 0) {return FALSE;}
	}

	wl_display_flush(wl.display);

	struct pollfd pfd = {
		.fd = wl_display_get_fd(wl.display), .events = POLLIN, .revents = 0
	};

	int result = poll(&pfd, 1, 0);
	if ((result > 0) && (pfd.revents & POLLIN) && (wl_display_read_events(wl.display) < 0)) {return FALSE;}
	else {wl_display_cancel_read(wl.display);}

	if (wl_display_dispatch_pending(wl.display) < 0) {return FALSE;}
	return TRUE;
}


int d_updateWindowWayland(void)
{
	if ((!wl.configured) || (!wl.buffer)) {return FALSE;}

	//Is the compositor still using the buffer?
	if (wl.buffer_busy) {return TRUE;}

	RGB_t* src = t_getFramebufferPTR();
	if (!src) {return FALSE;}

	uint32_t* dst = (uint32_t* )wl.shm_data;
	int pixel_count = wl.width*  wl.height;

	for (int i = 0; i < pixel_count; i++) {
		dst[i] = (
			((uint32_t)(src[i].r) << 16) |
			((uint32_t)(src[i].g) << 8)  |
			((uint32_t)(src[i].b))
		)
	}

	wl.buffer_busy = 1;
	wl_surface_attach(wl.surface, wl.buffer, 0, 0);
	wl_surface_damage_buffer(wl.surface, 0, 0, wl.width, wl.height);

	wl_surface_commit(wl.surface);
	wl_display_flush(wl.display);

	return TRUE;
}


int d_resizeWindowWayland(const Vec2i_t resolution) {
	//Wayland compositor resizes window for me
	//Resize the framebuffer with configure events [!!!]
	return TRUE;
}
//////// WAYLAND ////////






//////// X11 ////////
int d_initX11(void) {
	//TBA
	return TRUE;
}


int d_isRunningX11(void) {
	//TBA
	return TRUE;
}


int d_shutdownX11(void) {
	//TBA
	return TRUE;
}


int d_createWindowX11(const Vec2i_t resolution, const char* title) {
	//TBA
	return TRUE;
}


int d_destroyWindowX11(void) {
	//TBA
	return TRUE;
}


int d_pollWindowX11(void) {
	//TBA
	return TRUE;
}


int d_updateWindowX11(void) {
	//TBA
	RGB_t* t_fbPTR = t_getFramebufferPTR();
	return TRUE;
}


int d_resizeWindowX11(const Vec2i_t resolution) {
	//TBA
	return TRUE;
}
//////// X11 ////////






//////// EXTERN ////////
#ifndef _WIN32 //Linux only!
DisplayBackend _getBackendHLPR(void) {
	//Finds the current display backend (Wayland/X11)
	const char* session = getenv("XDG_SESSION_TYPE");

	if (session) {
		if (strcmp(session, "wayland") == 0) {return DISPLAY_WAYLAND;}
		if (strcmp(session, "x11") == 0) {return DISPLAY_X11;}
	}

	if (getenv("WAYLAND_DISPLAY")) {return DISPLAY_WAYLAND;}
	if (getenv("DISPLAY")) {return DISPLAY_X11;}
	
	return DISPLAY_UNKNOWN;	
}

DisplayBackend d_getBackend(void) {
	d_backend = _getBackendHLPR();
	return d_backend;
}


#else //Add dummy func that does nothing, because windows doesn't support this entire system of window control. (?)
DisplayBackend d_getBackend(void) {return DISPLAY_WIN32;}
#endif





#define SELECT_MGR(WAYL, X11, ...) \
	switch (d_backend) { \
		case DISPLAY_WAYLAND: {return WAYL(__VA_ARGS__);} \
		case DISPLAY_X11: {return X11(__VA_ARGS__);} \
		case DISPLAY_WIN32: {return FALSE;} \
		case DISPLAY_UNKNOWN: default: {return FALSE;} \
	}

int d_init(void) { //Initialises the display manager.
	d_backend = _getBackendHLPR(); //Initialise the backend enum.
	SELECT_MGR(d_initWayland, d_initX11);
} 
int d_isRunning(void) {SELECT_MGR(d_isRunningWayland, d_isRunningX11);} //Checks if display manager is running.
int d_shutdown(void) {SELECT_MGR(d_shutdownWayland, d_shutdownX11);} //Shuts down the display manager.

int d_createWindow(const Vec2i_t resolution, const char* title) {SELECT_MGR(d_createWindowWayland, d_createWindowX11, resolution, title);} //Creates a window in this given display manager.
int d_destroyWindow(void) {SELECT_MGR(d_destroyWindowWayland, d_destroyWindowX11);} //Kills the display manager's window.

int d_pollWindow(void) {SELECT_MGR(d_pollWindowWayland, d_pollWindowX11);} //Polls the display manager's window.
int d_updateWindow(void) {SELECT_MGR(d_updateWindowWayland, d_updateWindowX11);} //Updates the display manager's window.
int d_resizeWindow(const Vec2i_t resolution) {SELECT_MGR(d_resizeWindowWayland, d_resizeWindowX11, resolution);} //Resizes the display manager's window.
//////// EXTERN ////////

