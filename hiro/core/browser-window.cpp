#if defined(Hiro_BrowserWindow)

auto BrowserWindow::directory() -> string {
  return pBrowserWindow::directory(state);
}

auto BrowserWindow::open() -> string {
  return pBrowserWindow::open(state);
}

auto BrowserWindow::save() -> string {
  return pBrowserWindow::save(state);
}

auto BrowserWindow::setFilters(const std::vector<string>& filters) -> type& {
  state.filters = filters;
  return *this;
}

auto BrowserWindow::setParent(shared_pointer<mWindow> parent) -> type& {
  state.parent = parent;
  return *this;
}

auto BrowserWindow::setPath(const string& path) -> type& {
  state.path = path;
  return *this;
}

auto BrowserWindow::setTitle(const string& title) -> type& {
  state.title = title;
  return *this;
}

auto BrowserWindow::setAllowsFolders(bool allows) -> type& {
  state.allowsFolders = allows;
  return *this;
}

auto BrowserWindow::allowsFolders() const -> bool {
  return state.allowsFolders;
}

#endif
