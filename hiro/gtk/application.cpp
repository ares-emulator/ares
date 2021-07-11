#if defined(Hiro_Application)

#include <nall/terminal.hpp>

namespace hiro {

auto Log_Ignore(const char* logDomain, GLogLevelFlags logLevel, const char* message, void* userData) -> void {
}

auto Log_Filter(const char* logDomain, GLogLevelFlags logLevel, const char* message, void* userData) -> void {
  //FreeBSD 12.0: caused by gtk_combo_box_size_allocate() internal function being defective
  if(string{message}.find("gtk_widget_size_allocate():")) return;

  //print all other messages
  print(terminal::color::yellow("hiro: "), logDomain, "::", message, "\n");
}

auto pApplication::exit() -> void {
  quit();
  ::exit(EXIT_SUCCESS);
}

auto pApplication::modal() -> bool {
  return Application::state().modal > 0;
}

auto pApplication::run() -> void {
  while(!Application::state().quit) {
    Application::doMain();
    processEvents();
    //avoid spinlooping the thread when there is no main loop ...
    //when there is one, Application::onMain() is expected to sleep when possible instead
    if(!Application::state().onMain) usleep(2000);
  }
}

auto pApplication::pendingEvents() -> bool {
  return gtk_events_pending();
}

auto pApplication::processEvents() -> void {
  //GTK can sometimes return gtk_pending_events() == true forever,
  //no matter how many times gtk_main_iteration_do() is called.
  //implement a timeout to prevent hiro from hanging forever in this case.
  auto time = chrono::millisecond();
  while(pendingEvents() && chrono::millisecond() - time < 50) {
    gtk_main_iteration_do(false);
  }
  for(auto& window : state().windows) window->_synchronizeGeometry();
}

auto pApplication::quit() -> void {
  //if gtk_main() was invoked, call gtk_main_quit()
  if(gtk_main_level()) gtk_main_quit();

  #if defined(DISPLAY_XORG)
  if(state().display) {
    if(state().screenSaverXDG && state().screenSaverWindow) {
      //this needs to run synchronously, so that XUnmapWindow() won't happen before xdg-screensaver is finished
      execute("xdg-screensaver", "resume", string{"0x", hex(state().screenSaverWindow)});
      XUnmapWindow(state().display, state().screenSaverWindow);
      state().screenSaverWindow = 0;
    }
    XCloseDisplay(state().display);
    state().display = nullptr;
  }
  #endif
}

auto pApplication::setScreenSaver(bool screenSaver) -> void {
  #if defined(DISPLAY_XORG)
  if(state().screenSaverXDG && state().screenSaverWindow) {
    //when invoking this command on Linux under Xfce, the follow message is written to the terminal:
    //"org.freedesktop.DBus.Error.ServiceUnknown: The name org.gnome.SessionManager was not provided by any .service files"
    //to silence this message, stdout and stderr are redirected to /dev/null while invoking this command.
    auto fd = open("/dev/null", O_NONBLOCK);
    auto fo = dup(STDOUT_FILENO);
    auto fe = dup(STDERR_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    invoke("xdg-screensaver", screenSaver ? "resume" : "suspend", string{"0x", hex(state().screenSaverWindow)});
    dup2(fo, STDOUT_FILENO);
    dup2(fe, STDERR_FILENO);
    close(fd);
    close(fo);
    close(fe);
  }
  #endif
}

auto pApplication::state() -> State& {
  static State state;
  return state;
}

auto pApplication::initialize() -> void {
  #if defined(DISPLAY_XORG)
  // If running on Wayland, force usage of XWayland
  setenv("GDK_BACKEND", "x11", 1);
  XInitThreads();
  state().display = XOpenDisplay(nullptr);
  state().screenSaverXDG = (bool)execute("xdg-screensaver", "--version").output.find("xdg-screensaver");

  if(state().screenSaverXDG) {
    auto screen = DefaultScreen(state().display);
    auto rootWindow = RootWindow(state().display, screen);
    XSetWindowAttributes attributes{};
    attributes.background_pixel = BlackPixel(state().display, screen);
    attributes.border_pixel = 0;
    attributes.override_redirect = true;
    state().screenSaverWindow = XCreateWindow(state().display, rootWindow,
      0, 0, 1, 1, 0, DefaultDepth(state().display, screen),
      InputOutput, DefaultVisual(state().display, screen),
      CWBackPixel | CWBorderPixel | CWOverrideRedirect, &attributes
    );
    XStoreName(state().display, state().screenSaverWindow, "hiro-screen-saver-window");
    XFlush(state().display);
  }
  #endif

  //prevent useless terminal messages:
  //GVFS-RemoteVolumeMonitor: "invoking List() failed for type GProxyVolumeMonitorHal: method not implemented"
  g_log_set_handler("GVFS-RemoteVolumeMonitor", G_LOG_LEVEL_MASK, Log_Ignore, nullptr);
  //GLib-GIO: "excluding {path} from kernel notification, falling back to poll
  g_log_set_handler("GLib-GIO", G_LOG_LEVEL_MASK, Log_Ignore, nullptr);
  //Gtk: "gtk_widget_size_allocate(): attempt to allocate widget with (width or height < 1)"
  g_log_set_handler("Gtk", G_LOG_LEVEL_MASK, Log_Filter, nullptr);

  //set WM_CLASS to Application::name()
  auto name = Application::state().name ? Application::state().name : string{"hiro"};
  gdk_set_program_class(name);

  #if 0 && defined(BUILD_DEBUG)
  //force a trap on Gtk-CRITICAL and Gtk-WARNING messages
  //this allows gdb to perform a backtrace to find an error's origin point
  int argc = 3;
  char* argv[] = {name.get(), new char[7], new char[19], nullptr};
  strcpy(argv[1], "--sync");
  strcpy(argv[2], "--g-fatal-warnings");
  g_log_set_always_fatal(GLogLevelFlags(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING));
  #else
  int argc = 1;
  char* argv[] = {name.get(), nullptr};
  #endif
  char** argvp = argv;

  gtk_init(&argc, &argvp);
  GtkSettings* gtkSettings = gtk_settings_get_default();

  //allow buttons to show icons
  g_type_class_unref(g_type_class_ref(GTK_TYPE_BUTTON));
  g_object_set(gtkSettings, "gtk-button-images", true, nullptr);

  #if defined(DISPLAY_WINDOWS)
  //there is a serious bug in GTK 2.24 for Windows with the "ime" (Windows IME) input method:
  //by default, it will be impossible to type in text fields at all.
  //there are various tricks to get around this; but they are unintuitive and unreliable.
  //the "ime" method is chosen when various international system locales (eg Japanese) are selected.
  //here, we override the default input method to use the "Simple" type instead to avoid the bug.
  //obviously, this has a drawback: in-place editing for IMEs will not work in this mode.
  g_object_set(gtkSettings, "gtk-im-module", "gtk-im-context-simple", nullptr);
  #endif

  #if HIRO_GTK==2
  gtk_rc_parse_string(R"(
    style "HiroWindow"
    {
      GtkWindow::resize-grip-width = 0
      GtkWindow::resize-grip-height = 0
    }
    class "GtkWindow" style "HiroWindow"

    style "HiroTreeView"
    {
      GtkTreeView::vertical-separator = 0
    }
    class "GtkTreeView" style "HiroTreeView"

    style "HiroTabFrameCloseButton"
    {
      GtkWidget::focus-line-width = 0
      GtkWidget::focus-padding = 0
      GtkButton::default-border = {0, 0, 0, 0}
      GtkButton::default-outer-border = {0, 0, 0, 0}
      GtkButton::inner-border = {0, 1, 0, 0}
    }
    widget_class "*.<GtkNotebook>.<GtkHBox>.<GtkButton>" style "HiroTabFrameCloseButton"
  )");
  #elif HIRO_GTK==3
  GtkCssProvider* provider = gtk_css_provider_new();
  gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider), R"(
    * { font-size: 8pt; padding-top: 0px; padding-bottom: 0px; }
    menu { border-width: 0px; }
    menuitem { padding: 3px 5px; }
    entry { min-height: 0px; }
  )", -1, nullptr);
  GdkScreen* screen = gdk_screen_get_default();
  gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  #endif

  pKeyboard::initialize();
}

}

#endif
