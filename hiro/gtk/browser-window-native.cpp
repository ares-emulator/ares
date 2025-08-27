#if defined(Hiro_BrowserWindow)

namespace hiro {

static auto BrowserWindow_addFilters(GtkFileChooserNative* dialog, std::vector<string> filters) -> void {
  for(auto& filter : filters) {
    auto part = filter.split("|", 1L);
    if(part.size() != 2) continue;

    GtkFileFilter* gtkFilter = gtk_file_filter_new();
    gtk_file_filter_set_name(gtkFilter, part[0]);
    auto patterns = part[1].split(":");
    for(auto& pattern : patterns) gtk_file_filter_add_pattern(gtkFilter, pattern);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), gtkFilter);
  }
}

auto pBrowserWindow::directory(BrowserWindow::State& state) -> string {
  string name;

  GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
    state.title ? state.title : "Select Directory"_s,
    state.parent && state.parent->self() ? GTK_WINDOW(state.parent->self()->widget) : (GtkWindow*)nullptr,
    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    "_Open",
    "_Cancel"
  );

  if(state.path) gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.path);

  if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    name = temp;
    g_free(temp);
  }

  g_object_unref(dialog);
  if(name && !name.endsWith("/")) name.append("/");
  return name;
}

auto pBrowserWindow::open(BrowserWindow::State& state) -> string {
  string name;

  GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
    state.title ? state.title : "Open File"_s,
    state.parent && state.parent->self() ? GTK_WINDOW(state.parent->self()->widget) : (GtkWindow*)nullptr,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    "_Open",
    "_Cancel"
  );

  if(state.path) gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.path);
  BrowserWindow_addFilters(dialog, state.filters);

  if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    name = temp;
    g_free(temp);
  }

  g_object_unref(dialog);
  return name;
}

auto pBrowserWindow::save(BrowserWindow::State& state) -> string {
  string name;

  GtkFileChooserNative* dialog = gtk_file_chooser_native_new(
    state.title ? state.title : "Save File"_s,
    state.parent && state.parent->self() ? GTK_WINDOW(state.parent->self()->widget) : (GtkWindow*)nullptr,
    GTK_FILE_CHOOSER_ACTION_SAVE,
    "_Save",
    "_Cancel"
  );

  if(state.path) gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.path);
  BrowserWindow_addFilters(dialog, state.filters);

  if(gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char* temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    name = temp;
    g_free(temp);
  }

  g_object_unref(dialog);
  return name;
}

}

#endif
