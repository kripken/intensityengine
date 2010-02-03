/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// This file implements the platform specific parts of the plugin for
// the Linux platform.

#include <X11/keysym.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "plugin/cross/main.h"
#include "plugin/cross/out_of_memory.h"
#include "plugin/linux/envvars.h"

using glue::_o3d::PluginObject;
using glue::StreamManager;
//using o3d::DisplayWindowLinux;
//using o3d::Event;

namespace {
// We would normally make this a stack variable in main(), but in a
// plugin, that's not possible, so we make it a global. When the DLL is loaded
// this it gets constructed and when it is unlooaded it is destructed. Note
// that this cannot be done in NP_Initialize and NP_Shutdown because those
// calls do not necessarily signify the DLL being loaded and unloaded. If the
// DLL is not unloaded then the values of global variables are preserved.
base::AtExitManager g_at_exit_manager;

bool g_xembed_support = false;

#ifdef O3D_PLUGIN_ENV_VARS_FILE
static const char *kEnvVarsFilePath = O3D_PLUGIN_ENV_VARS_FILE;
#endif

static void DrawPlugin(PluginObject *obj) {
  // Limit drawing to no more than once every timer tick.
  if (!obj->draw_) return;
//  obj->client()->RenderClient(true);
  obj->draw_ = false;
}

// Xt support functions

void LinuxTimer(XtPointer data, XtIntervalId* id) {
}

void LinuxExposeHandler(Widget w,
                        XtPointer user_data,
                        XEvent *event,
                        Boolean *cont) {
}

static int KeySymToDOMKeyCode(KeySym key_sym) {
  // See https://developer.mozilla.org/en/DOM/Event/UIEvent/KeyEvent for the
  // DOM values.
  // X keycodes are not useful, because they describe the geometry, not the
  // associated symbol, so a 'Q' on a QWERTY (US) keyboard has the same keycode
  // as a 'A' on an AZERTY (french) one.
  // Key symbols are closer to what the DOM expects, but they depend on the
  // shift/control/alt combination - the same key has several symbols ('a' vs
  // 'A', '1' vs '!', etc.), so we do extra work so that 'a' and 'A' both
  // generate the same DOM keycode.
  if ((key_sym >= XK_0 && key_sym <= XK_Z)) {
    // DOM keycode matches ASCII value, as does the keysym.
    return key_sym;
  } else if ((key_sym >= XK_a && key_sym <= XK_z)) {
    return key_sym - XK_a + XK_A;
  } else if ((key_sym >= XK_KP_0 && key_sym <= XK_KP_9)) {
    return 0x60 + key_sym - XK_KP_0;
  } else if ((key_sym >= XK_F1 && key_sym <= XK_F24)) {
    return 0x70 + key_sym - XK_F1;
  }
  switch (key_sym) {
    case XK_Cancel:
      return 0x03;
    case XK_Help:
      return 0x06;
    case XK_BackSpace:
      return 0x08;
    case XK_Tab:
      return 0x09;
    case XK_Clear:
      return 0x0C;
    case XK_Return:
      return 0x0D;
    case XK_KP_Enter:
      return 0x0E;
    case XK_Shift_L:
    case XK_Shift_R:
      return 0x10;
    case XK_Control_L:
    case XK_Control_R:
      return 0x11;
    case XK_Alt_L:
    case XK_Alt_R:
      return 0x12;
    case XK_Pause:
      return 0x13;
    case XK_Caps_Lock:
      return 0x14;
    case XK_Escape:
      return 0x1B;
    case XK_space:
      return 0x20;
    case XK_Page_Up:
    case XK_KP_Page_Up:
      return 0x21;
    case XK_Page_Down:
    case XK_KP_Page_Down:
      return 0x22;
    case XK_End:
    case XK_KP_End:
      return 0x23;
    case XK_Home:
    case XK_KP_Home:
      return 0x24;
    case XK_Left:
    case XK_KP_Left:
      return 0x25;
    case XK_Up:
    case XK_KP_Up:
      return 0x26;
    case XK_Right:
    case XK_KP_Right:
      return 0x27;
    case XK_Down:
    case XK_KP_Down:
      return 0x28;
    case XK_Print:
      return 0x2C;
    case XK_Insert:
    case XK_KP_Insert:
      return 0x2D;
    case XK_Delete:
    case XK_KP_Delete:
      return 0x2E;
    case XK_Menu:
      return 0x5D;
    case XK_asterisk:
    case XK_KP_Multiply:
      return 0x6A;
    case XK_plus:
    case XK_KP_Add:
      return 0x6B;
    case XK_underscore:
      return 0x6C;
    case XK_minus:
    case XK_KP_Subtract:
      return 0x6E;
    case XK_KP_Decimal:
      return 0x6E;
    case XK_KP_Divide:
      return 0x6F;
    case XK_Num_Lock:
      return 0x90;
    case XK_Scroll_Lock:
      return 0x91;
    case XK_comma:
      return 0xBC;
    case XK_period:
      return 0xBE;
    case XK_slash:
      return 0xBF;
    case XK_grave:
      return 0xC0;
    case XK_bracketleft:
      return 0xDB;
    case XK_backslash:
      return 0xDC;
    case XK_bracketright:
      return 0xDD;
    case XK_apostrophe:
      return 0xDE;
    case XK_Meta_L:
    case XK_Meta_R:
      return 0xE0;
    default:
      return 0;
  }
}

static int GetXModifierState(int x_state) {
    return 0;
}

void LinuxKeyHandler(Widget w,
                     XtPointer user_data,
                     XEvent *xevent,
                     Boolean *cont) {
}

// TODO: Any way to query the system for the correct value ? According to
// http://library.gnome.org/devel/gdk/stable/gdk-Event-Structures.html GTK uses
// 250ms.
const unsigned int kDoubleClickTime = 250;  // in ms

void LinuxMouseButtonHandler(Widget w,
                             XtPointer user_data,
                             XEvent *xevent,
                             Boolean *cont) {
}

void LinuxMouseMoveHandler(Widget w,
                           XtPointer user_data,
                           XEvent *xevent,
                           Boolean *cont) {
}

void LinuxEnterLeaveHandler(Widget w,
                            XtPointer user_data,
                            XEvent *xevent,
                            Boolean *cont) {
}

// XEmbed / GTK support functions
static int GetGtkModifierState(int gtk_state) {
  int modifier_state = 0;
  if (gtk_state & GDK_CONTROL_MASK) {
//    modifier_state |= Event::MODIFIER_CTRL;
  }
  if (gtk_state & GDK_SHIFT_MASK) {
//    modifier_state |= Event::MODIFIER_SHIFT;
  }
  if (gtk_state & GDK_MOD1_MASK) {
//    modifier_state |= Event::MODIFIER_ALT;
  }
#if 0
  // TODO: This code is temporarily disabled until we figure out which exact
  // version of GTK to test for: GDK_META_MASK doesn't exist in older (e.g. 2.8)
  // versions.
  if (gtk_state & GDK_META_MASK) {
    modifier_state |= Event::MODIFIER_META;
  }
#endif
  return modifier_state;
}

static gboolean GtkHandleMouseMove(GtkWidget *widget,
                                   GdkEventMotion *motion_event,
                                   PluginObject *obj) {
  obj->intensityObject->onMouseMove(motion_event->x, motion_event->y);
  return TRUE;
}

static gboolean GtkHandleMouseButton(GtkWidget *widget,
                                     GdkEventButton *button_event,
                                     PluginObject *obj) {
  obj->intensityObject->onMouseButton(button_event->button, button_event->type == GDK_BUTTON_PRESS);
  return TRUE;
}

static gboolean GtkHandleKey(GtkWidget *widget,
                             GdkEventKey *key_event,
                             PluginObject *obj) {
    obj->intensityObject->onKeyboard(
        KeySymToDOMKeyCode(key_event->keyval),
        gdk_keyval_to_unicode(key_event->keyval),
        key_event->type == GDK_KEY_PRESS
    );
    return TRUE;
}

static gboolean GtkHandleScroll(GtkWidget *widget,
                                GdkEventScroll *scroll_event,
                                PluginObject *obj) {
  return TRUE;
}

void GtkHandleEventCrossing(GtkWidget *widget, GdkEventCrossing *crossing_event)
{
printf("\r\n\r\ gtk FFFFFFFFFFFFFFFFFFFOCUS\r\n\r\n");
//    gdk_window_focus(gtk_widget_get_window(widget), crossing_event->time);
    gtk_widget_set_can_focus(widget, true);
    gtk_widget_grab_focus(widget);
}

static gboolean GtkEventCallback(GtkWidget *widget,
                                 GdkEvent *event,
                                 gpointer user_data) {
printf("Event: %d\r\n", event->type);
  PluginObject *obj = static_cast<PluginObject *>(user_data);
  DLOG_ASSERT(widget == obj->gtk_event_source_);
  switch (event->type) {
    case GDK_EXPOSE:
      if (GTK_WIDGET_DRAWABLE(widget)) {
        obj->draw_ = true;
        DrawPlugin(obj);
      }
      return TRUE;
    case GDK_ENTER_NOTIFY:
      GtkHandleEventCrossing(widget, &event->crossing);
      obj->set_in_plugin(true);
      return TRUE;
    case GDK_LEAVE_NOTIFY:
      obj->set_in_plugin(false);
      return TRUE;
    case GDK_MOTION_NOTIFY:
      return GtkHandleMouseMove(widget, &event->motion, obj);
    case GDK_BUTTON_PRESS:
    case GDK_2BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      return GtkHandleMouseButton(widget, &event->button, obj);
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      return GtkHandleKey(widget, &event->key, obj);
    case GDK_SCROLL:
      return GtkHandleScroll(widget, &event->scroll, obj);
    default:
      return FALSE;
  }
}

static gboolean GtkConfigureEventCallback(GtkWidget *widget,
                                          GdkEventConfigure *configure_event,
                                          gpointer user_data) {
  PluginObject *obj = static_cast<PluginObject *>(user_data);
  return obj->OnGtkConfigure(widget, configure_event);
}

static gboolean GtkDeleteEventCallback(GtkWidget *widget,
                                        GdkEvent *event,
                                        gpointer user_data) {
  PluginObject *obj = static_cast<PluginObject *>(user_data);
  return obj->OnGtkDelete(widget, event);
}

static gboolean GtkTimeoutCallback(gpointer user_data) {
  PluginObject *obj = static_cast<PluginObject *>(user_data);
      GtkWidget *widget;
      widget = obj->gtk_container_;
      gtk_widget_queue_draw(widget);
  return TRUE;
}

NPError InitializePlugin() {
  CommandLine::Init(0, NULL);
  InitLogging("debug.log",
              logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG,
              logging::DONT_LOCK_LOG_FILE,
              logging::APPEND_TO_OLD_LOG_FILE);

  DLOG(INFO) << "NP_Initialize";

#ifdef O3D_PLUGIN_ENV_VARS_FILE
  // Before doing anything more, we first load our environment variables file.
  // This file is a newline-delimited list of any system-specific environment
  // variables that need to be set in the browser. Since we are a shared library
  // and not an executable, we can't set them at browser start time, so we
  // instead set them in every process that loads our shared library. It is
  // important that we do this as early as possible so that any relevant
  // variables are already set when we initialize our shared library
  // dependencies.
  o3d::LoadEnvironmentVariablesFile(kEnvVarsFilePath);
#endif

  // Check for XEmbed support in the browser.
  NPBool xembed_support = 0;
  NPError err = NPN_GetValue(NULL, NPNVSupportsXEmbedBool, &xembed_support);
  if (err != NPERR_NO_ERROR)
    xembed_support = 0;

  if (xembed_support) {
    // Check for Gtk2 toolkit support in the browser.
    NPNToolkitType toolkit = static_cast<NPNToolkitType>(0);
    err = NPN_GetValue(NULL, NPNVToolkit, &toolkit);
    if (err != NPERR_NO_ERROR || toolkit != NPNVGtk2)
      xembed_support = 0;
  }
  g_xembed_support = xembed_support != 0;

  return NPERR_NO_ERROR;
}

}  // end anonymous namespace

#if defined(O3D_INTERNAL_PLUGIN)
namespace o3d {
#else
extern "C" {
#endif

NPError EXPORT_SYMBOL OSCALL NP_Initialize(NPNetscapeFuncs *browserFuncs,
                                           NPPluginFuncs *pluginFuncs) {
printf("Init\r\n");
  NPError retval = InitializeNPNApi(browserFuncs);
  if (retval != NPERR_NO_ERROR) return retval;
  NP_GetEntryPoints(pluginFuncs);
  return InitializePlugin();
}

NPError EXPORT_SYMBOL OSCALL NP_Shutdown(void) {
printf("Shutdown\r\n");

  HANDLE_CRASHES;
  DLOG(INFO) << "NP_Shutdown";

  CommandLine::Reset();

  return NPERR_NO_ERROR;
}

}  // namespace o3d / extern "C"

namespace o3d {

NPError PlatformNPPGetValue(NPP instance, NPPVariable variable, void *value) {
  switch (variable) {
    case NPPVpluginNeedsXEmbed:
      *static_cast<NPBool *>(value) = g_xembed_support;
      return NPERR_NO_ERROR;
    default:
      return NPERR_INVALID_PARAM;
  }
  return NPERR_NO_ERROR;
}

NPError NPP_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc,
                char *argn[], char *argv[], NPSavedData *saved) {
  HANDLE_CRASHES;

  PluginObject* pluginObject = glue::_o3d::PluginObject::Create(
      instance);
  instance->pdata = pluginObject;
  glue::_o3d::InitializeGlue(instance);
  pluginObject->Init(argc, argn, argv);

  // Get the metrics for the system setup
//  GetUserConfigMetrics();
  return NPERR_NO_ERROR;
}

NPError NPP_Destroy(NPP instance, NPSavedData **save) {
printf("Destroy\r\n");
  HANDLE_CRASHES;
  PluginObject *obj = static_cast<PluginObject*>(instance->pdata);
  if (obj) {
    obj->TearDown();

    if (obj->xt_widget_) {
      // NOTE: This crashes. Not sure why, possibly the widget has
      // already been destroyed, but we haven't received a SetWindow(NULL).
      // XtRemoveEventHandler(obj->xt_widget_, ExposureMask, False,
      //                     LinuxExposeHandler, obj);
      obj->xt_widget_ = NULL;
    }
    if (obj->xt_interval_) {
      XtRemoveTimeOut(obj->xt_interval_);
      obj->xt_interval_ = 0;
    }
    if (obj->timeout_id_) {
      g_source_remove(obj->timeout_id_);
      obj->timeout_id_ = 0;
    }
    if (obj->gtk_container_) {
      gtk_widget_destroy(obj->gtk_container_);
      obj->gtk_container_ = NULL;
    }
    if (obj->gtk_fullscreen_container_) {
      gtk_widget_destroy(obj->gtk_fullscreen_container_);
      obj->gtk_fullscreen_container_ = NULL;
    }
    if (obj->gdk_display_) {
      gdk_display_close(obj->gdk_display_);
      obj->gdk_display_ = NULL;
    }
    obj->gtk_event_source_ = NULL;
    obj->event_handler_id_ = 0;
    obj->window_ = 0;
    obj->drawable_ = 0;

    NPN_ReleaseObject(obj);
    instance->pdata = NULL;
  }
  return NPERR_NO_ERROR;
}

NPError NPP_SetWindow(NPP instance, NPWindow *window) {
  HANDLE_CRASHES;
  PluginObject *obj = static_cast<PluginObject*>(instance->pdata);
//  obj->intensityObject->setWindow(window);
  NPSetWindowCallbackStruct *cb_struct =
      static_cast<NPSetWindowCallbackStruct *>(window->ws_info);
  Window xwindow = reinterpret_cast<Window>(window->window);
  if (xwindow != obj->window_) {
    Display *display = cb_struct->display;
    Window drawable = xwindow;
    if (g_xembed_support) {
      // We asked for a XEmbed plugin, the xwindow is a GtkSocket, we create
      // a GtkPlug to go into it.
      obj->gdk_display_ = gdk_display_open(XDisplayString(display));
      LOG_ASSERT(obj->gdk_display_) << "Unable to open X11 display";
      display = GDK_DISPLAY_XDISPLAY(obj->gdk_display_);
      obj->gtk_container_ =
          gtk_plug_new_for_display(obj->gdk_display_, xwindow);
      // Firefox has a bug where it sometimes destroys our parent widget before
      // calling NPP_Destroy. We handle this by hiding our X window instead of
      // destroying it. Without this, future OpenGL calls can raise a
      // GLXBadDrawable error and kill the browser process.
      g_signal_connect(G_OBJECT(obj->gtk_container_), "delete-event",
                       G_CALLBACK(gtk_widget_hide_on_delete), NULL);
      gtk_widget_set_double_buffered(obj->gtk_container_, FALSE);
      if (!obj->fullscreen()) {
        obj->SetGtkEventSource(obj->gtk_container_);
      }
      gtk_widget_show(obj->gtk_container_);
      drawable = GDK_WINDOW_XID(obj->gtk_container_->window);

        void *save = window->window;
        window->window = reinterpret_cast<void*>(drawable);
        obj->intensityObject->setWindow(window);
        window->window = save;

//      obj->timeout_id_ = g_timeout_add(10, GtkTimeoutCallback, obj);
    } else {
      // No XEmbed support, the xwindow is a Xt Widget.
      Widget widget = XtWindowToWidget(display, xwindow);
      if (!widget) {
        DLOG(ERROR) << "window is not a Widget";
        return NPERR_MODULE_LOAD_FAILED_ERROR;
      }
      obj->xt_widget_ = widget;
      XtAddEventHandler(widget, ExposureMask, 0, LinuxExposeHandler, obj);
      XtAddEventHandler(widget, KeyPressMask|KeyReleaseMask, 0,
                        LinuxKeyHandler, obj);
      XtAddEventHandler(widget, ButtonPressMask|ButtonReleaseMask, 0,
                        LinuxMouseButtonHandler, obj);
      XtAddEventHandler(widget, PointerMotionMask, 0,
                        LinuxMouseMoveHandler, obj);
      XtAddEventHandler(widget, EnterWindowMask|LeaveWindowMask, 0,
                        LinuxEnterLeaveHandler, obj);
      obj->xt_app_context_ = XtWidgetToApplicationContext(widget);
      obj->xt_interval_ =
          XtAppAddTimeOut(obj->xt_app_context_, 10, LinuxTimer, obj);
    }

    // Create and assign the graphics context.
// XXX    o3d::DisplayWindowLinux default_display;
//    default_display.set_display(display);
//    default_display.set_window(drawable);

//    obj->CreateRenderer(default_display);
//    obj->client()->Init();
    obj->SetDisplay(display);
    obj->window_ = xwindow;
    obj->drawable_ = drawable;
  }
  obj->Resize(window->width, window->height);

  return NPERR_NO_ERROR;
}

// Called when the browser has finished attempting to stream data to
// a file as requested. If fname == NULL the attempt was not successful.
void NPP_StreamAsFile(NPP instance, NPStream *stream, const char *fname) {
  HANDLE_CRASHES;
}

int16 NPP_HandleEvent(NPP instance, void *event) {
  HANDLE_CRASHES;
  return 0;
}
}  // namespace o3d

namespace glue {
namespace _o3d {

void PluginObject::SetGtkEventSource(GtkWidget *widget) {
  if (gtk_event_source_) {
    g_signal_handler_disconnect(G_OBJECT(gtk_event_source_),
                                event_handler_id_);
  }
  gtk_event_source_ = widget;
  if (gtk_event_source_) {
    gtk_widget_add_events(gtk_event_source_,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_SCROLL_MASK |
                          GDK_KEY_PRESS_MASK |
                          GDK_KEY_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK |
                          GDK_EXPOSURE_MASK |
                          GDK_ENTER_NOTIFY_MASK |
                          GDK_LEAVE_NOTIFY_MASK);
    event_handler_id_ = g_signal_connect(G_OBJECT(gtk_event_source_), "event",
                                         G_CALLBACK(GtkEventCallback), this);
  }
}

gboolean PluginObject::OnGtkConfigure(GtkWidget *widget,
                                      GdkEventConfigure *configure_event) {
assert(0);
  return TRUE;
}

gboolean PluginObject::OnGtkDelete(GtkWidget *widget,
                                   GdkEvent *event) {
  DLOG_ASSERT(widget == gtk_fullscreen_container_);
  CancelFullscreenDisplay();
  return TRUE;
}

bool PluginObject::GetDisplayMode(int id, o3d::DisplayMode *mode) {
  return true; // XXX renderer()->GetDisplayMode(id, mode);
}

// TODO: Where should this really live?  It's platform-specific, but in
// PluginObject, which mainly lives in cross/o3d_glue.h+cc.
bool PluginObject::RequestFullscreenDisplay() {
  if (fullscreen_ || fullscreen_pending_) {
    return false;
  }
  if (!g_xembed_support) {
    // I tested every Linux browser I could that worked with our plugin and not
    // a single one lacked XEmbed/Gtk2 support, so I don't think that case is
    // worth implementing.
    DLOG(ERROR) << "Fullscreen not supported without XEmbed/Gtk2; please use a "
        "modern web browser";
    return false;
  }
  GtkWidget *widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  // The returned object counts as both a widget and a window.
  GtkWindow *window = GTK_WINDOW(widget);
  // The window title shouldn't normally be visible, but the user will see it
  // if they Alt+Tab to another app.
  gtk_window_set_title(window, "O3D Application");
  // Suppresses the title bar, resize controls, etc.
  gtk_window_set_decorated(window, FALSE);
  // Stops Gtk from writing an off-screen buffer to the display, which conflicts
  // with our GL rendering.
  gtk_widget_set_double_buffered(widget, FALSE);
  gtk_window_set_keep_above(window, TRUE);
  GdkScreen *screen = gtk_window_get_screen(window);
  // In the case of Xinerama or TwinView, these will be the dimensions of the
  // whole desktop, which is wrong, but the window manager is smart enough to
  // restrict our size to that of the main screen.
  gint width = gdk_screen_get_width(screen);
  gint height = gdk_screen_get_height(screen);
  gtk_window_set_default_size(window, width, height);
  // This is probably superfluous since we have already set an appropriate
  // size, but let's do it anyway. It could still be relevant for some window
  // managers.
  gtk_window_fullscreen(window);
  g_signal_connect(G_OBJECT(window), "configure-event",
                   G_CALLBACK(GtkConfigureEventCallback), this);
  g_signal_connect(G_OBJECT(window), "delete-event",
                   G_CALLBACK(GtkDeleteEventCallback), this);
  gtk_fullscreen_container_ = widget;
  gtk_widget_show(widget);
  // We defer switching to the new window until it gets displayed and assigned
  // it's final dimensions in the configure-event.
  fullscreen_pending_ = true;
  return true;
}

void PluginObject::CancelFullscreenDisplay() {
assert(0);
}
}  // namespace _o3d
}  // namespace glue
