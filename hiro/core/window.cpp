#if defined(Hiro_Window)

mWindow::mWindow() {
  mObject::state.visible = false;
}

auto mWindow::allocate() -> pObject* {
  return new pWindow(*this);
}

auto mWindow::destruct() -> void {
  if(auto& menuBar = state.menuBar) menuBar->destruct();
  if(auto& sizable = state.sizable) sizable->destruct();
  if(auto& statusBar = state.statusBar) statusBar->destruct();
  mObject::destruct();
}

//

auto mWindow::append(sMenuBar menuBar) -> type& {
  if(auto& menuBar = state.menuBar) remove(menuBar);
  menuBar->setParent(this, 0);
  state.menuBar = menuBar;
  signal(append, menuBar);
  return *this;
}

auto mWindow::append(sSizable sizable) -> type& {
  if(auto& sizable = state.sizable) remove(sizable);
  state.sizable = sizable;
  sizable->setParent(this, 0);
  signal(append, sizable);
  return *this;
}

auto mWindow::append(sStatusBar statusBar) -> type& {
  if(auto& statusBar = state.statusBar) remove(statusBar);
  statusBar->setParent(this, 0);
  state.statusBar = statusBar;
  signal(append, statusBar);
  return *this;
}

auto mWindow::backgroundColor() const -> Color {
  return state.backgroundColor;
}

auto mWindow::dismissable() const -> bool {
  return state.dismissable;
}

auto mWindow::doClose() const -> void {
  if(state.onClose) return state.onClose();
}

auto mWindow::doDrop(vector<string> names) const -> void {
  if(state.onDrop) return state.onDrop(names);
}

auto mWindow::doKeyPress(s32 key) const -> void {
  if(state.onKeyPress) return state.onKeyPress(key);
}

auto mWindow::doKeyRelease(s32 key) const -> void {
  if(state.onKeyRelease) return state.onKeyRelease(key);
}

auto mWindow::doMove() const -> void {
  if(state.onMove) return state.onMove();
}

auto mWindow::doSize() const -> void {
  if(state.onSize) return state.onSize();
}

auto mWindow::droppable() const -> bool {
  return state.droppable;
}

auto mWindow::frameGeometry() const -> Geometry {
  Geometry margin = signal(frameMargin);
  return {
    state.geometry.x() - margin.x(), state.geometry.y() - margin.y(),
    state.geometry.width() + margin.width(), state.geometry.height() + margin.height()
  };
}

auto mWindow::fullScreen() const -> bool {
  return state.fullScreen;
}

auto mWindow::geometry() const -> Geometry {
  return state.geometry;
}

auto mWindow::handle() const -> uintptr_t {
  return signal(handle);
}

auto mWindow::maximized() const -> bool {
  return state.maximized;
}

auto mWindow::maximumSize() const -> Size {
  return state.maximumSize;
}

auto mWindow::menuBar() const -> MenuBar {
  return state.menuBar;
}

auto mWindow::minimized() const -> bool {
  return state.minimized;
}

auto mWindow::minimumSize() const -> Size {
  return state.minimumSize;
}

auto mWindow::modal() const -> bool {
  return state.modal;
}

auto mWindow::monitor() const -> u32 {
  return signal(monitor);
}

auto mWindow::onClose(const function<void ()>& callback) -> type& {
  state.onClose = callback;
  return *this;
}

auto mWindow::onDrop(const function<void (vector<string>)>& callback) -> type& {
  state.onDrop = callback;
  return *this;
}

auto mWindow::onKeyPress(const function<void (s32)>& callback) -> type& {
  state.onKeyPress = callback;
  return *this;
}

auto mWindow::onKeyRelease(const function<void (s32)>& callback) -> type& {
  state.onKeyRelease = callback;
  return *this;
}

auto mWindow::onMove(const function<void ()>& callback) -> type& {
  state.onMove = callback;
  return *this;
}

auto mWindow::onSize(const function<void ()>& callback) -> type& {
  state.onSize = callback;
  return *this;
}

auto mWindow::remove(sMenuBar menuBar) -> type& {
  signal(remove, menuBar);
  menuBar->setParent();
  state.menuBar.reset();
  return *this;
}

auto mWindow::remove(sSizable sizable) -> type& {
  signal(remove, sizable);
  sizable->setParent();
  state.sizable.reset();
  return *this;
}

auto mWindow::remove(sStatusBar statusBar) -> type& {
  signal(remove, statusBar);
  statusBar->setParent();
  state.statusBar.reset();
  return *this;
}

auto mWindow::reset() -> type& {
  if(auto& menuBar = state.menuBar) remove(menuBar);
  if(auto& sizable = state.sizable) remove(sizable);
  if(auto& statusBar = state.statusBar) remove(statusBar);
  return *this;
}

auto mWindow::resizable() const -> bool {
  return state.resizable;
}

auto mWindow::setAlignment(Alignment alignment) -> type& {
  auto workspace = Monitor::workspace();
  auto geometry = frameGeometry();
  auto x = workspace.x() + alignment.horizontal() * (workspace.width() - geometry.width());
  auto y = workspace.y() + alignment.vertical() * (workspace.height() - geometry.height());
  setFramePosition({(s32)x, (s32)y});
  return *this;
}

auto mWindow::setAlignment(sWindow relativeTo, Alignment alignment) -> type& {
  if(!relativeTo) return setAlignment(alignment);
  auto parent = relativeTo->frameGeometry();
  auto window = frameGeometry();
  //+0 .. +1 => within parent window
  auto x = parent.x() + (parent.width() - window.width()) * alignment.horizontal();
  auto y = parent.y() + (parent.height() - window.height()) * alignment.vertical();
  //-1 .. -0 => beyond parent window
  //... I know, relying on -0 IEE754 here is ... less than ideal
  if(signbit(alignment.horizontal())) {
    x = (parent.x() - window.width()) + abs(alignment.horizontal()) * (parent.width() + window.width());
  }
  if(signbit(alignment.vertical())) {
    y = (parent.y() - window.height()) + abs(alignment.vertical()) * (parent.height() + window.height());
  }
  setFramePosition({(s32)x, (s32)y});
  return *this;
}

auto mWindow::setBackgroundColor(Color color) -> type& {
  state.backgroundColor = color;
  signal(setBackgroundColor, color);
  return *this;
}

auto mWindow::setDismissable(bool dismissable) -> type& {
  state.dismissable = dismissable;
  signal(setDismissable, dismissable);
  return *this;
}

auto mWindow::setDroppable(bool droppable) -> type& {
  state.droppable = droppable;
  signal(setDroppable, droppable);
  return *this;
}

auto mWindow::setFrameGeometry(Geometry geometry) -> type& {
  Geometry margin = signal(frameMargin);
  return setGeometry({
    geometry.x() + margin.x(), geometry.y() + margin.y(),
    geometry.width() - margin.width(), geometry.height() - margin.height()
  });
}

auto mWindow::setFramePosition(Position position) -> type& {
  Geometry margin = signal(frameMargin);
  return setGeometry({
    position.x() + margin.x(), position.y() + margin.y(),
    state.geometry.width(), state.geometry.height()
  });
}

auto mWindow::setFrameSize(Size size) -> type& {
  Geometry margin = signal(frameMargin);
  return setGeometry({
    state.geometry.x(), state.geometry.y(),
    size.width() - margin.width(), size.height() - margin.height()
  });
}

auto mWindow::setFullScreen(bool fullScreen) -> type& {
  if(fullScreen != state.fullScreen) {
    state.fullScreen = fullScreen;
    signal(setFullScreen, fullScreen);
  }
  return *this;
}

auto mWindow::setGeometry(Geometry geometry) -> type& {
  //round fractional bits of geometry coordinates that window managers cannot display.
  //the pWindow classes lose this precision and so not doing so here can cause off-by-1 issues.
  geometry.setX(round(geometry.x()));
  geometry.setY(round(geometry.y()));
  geometry.setWidth(round(geometry.width()));
  geometry.setHeight(round(geometry.height()));

  state.geometry = geometry;
  signal(setGeometry, geometry);
  if(auto& sizable = state.sizable) sizable->setGeometry(sizable->geometry());
  return *this;
}

auto mWindow::setGeometry(Alignment alignment, Size size) -> type& {
  auto margin = signal(frameMargin);
  auto width = margin.width() + size.width();
  auto height = margin.height() + size.height();
  auto workspace = Monitor::workspace();
  auto x = workspace.x() + alignment.horizontal() * (workspace.width() - width);
  auto y = workspace.y() + alignment.vertical() * (workspace.height() - height);
  setFrameGeometry({(s32)x, (s32)y, (s32)width, (s32)height});
  return *this;
}

auto mWindow::setMaximized(bool maximized) -> type& {
  state.maximized = maximized;
  signal(setMaximized, maximized);
  return *this;
}

auto mWindow::setMaximumSize(Size size) -> type& {
  state.maximumSize = size;
  signal(setMaximumSize, size);
  return *this;
}

auto mWindow::setMinimized(bool minimized) -> type& {
  state.minimized = minimized;
  signal(setMinimized, minimized);
  return *this;
}

auto mWindow::setMinimumSize(Size size) -> type& {
  state.minimumSize = size;
  signal(setMinimumSize, size);
  return *this;
}

auto mWindow::setModal(bool modal) -> type& {
  if(state.modal == modal) return *this;
  state.modal = modal;
  if(modal) {
    Application::state().modal++;
  } else {
    Application::state().modal--;
    assert(Application::state().modal >= 0);
  }
  signal(setModal, modal);
  return *this;
}

auto mWindow::setPosition(Position position) -> type& {
  return setGeometry({
    position.x(), position.y(),
    state.geometry.width(), state.geometry.height()
  });
}

auto mWindow::setPosition(sWindow relativeTo, Position position) -> type& {
  if(!relativeTo) return setPosition(position);
  auto geometry = relativeTo->frameGeometry();
  return setFramePosition({
    geometry.x() + position.x(),
    geometry.y() + position.y()
  });
}

auto mWindow::setResizable(bool resizable) -> type& {
  state.resizable = resizable;
  signal(setResizable, resizable);
  return *this;
}

auto mWindow::setSize(Size size) -> type& {
  return setGeometry({
    state.geometry.x(), state.geometry.y(),
    size.width(), size.height()
  });
}

auto mWindow::setTitle(const string& title) -> type& {
  state.title = title;
  signal(setTitle, title);
  return *this;
}

auto mWindow::setAssociatedFile(const string& filename) -> type& {
#if defined(PLATFORM_MACOS)
    signal(setAssociatedFile, filename);
#endif
    return *this;
}

auto mWindow::setVisible(bool visible) -> type& {
  mObject::setVisible(visible);
  if(auto& menuBar = state.menuBar) menuBar->setVisible(menuBar->visible());
  if(auto& sizable = state.sizable) sizable->setVisible(sizable->visible());
  if(auto& statusBar = state.statusBar) statusBar->setVisible(statusBar->visible());
  return *this;
}

auto mWindow::sizable() const -> Sizable {
  return state.sizable;
}

auto mWindow::statusBar() const -> StatusBar {
  return state.statusBar;
}

auto mWindow::title() const -> string {
  return state.title;
}

auto mWindow::borderless() const -> bool {
  return state.borderless;
}

auto mWindow::setBorderless(bool borderless) -> type& {
  state.borderless = borderless;
  signal(setBorderless, borderless);
  return *this;
}

#endif
