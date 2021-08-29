#if defined(Hiro_Window)

@interface CocoaWindow : NSWindow <NSWindowDelegate> {
@public
  hiro::mWindow* window;
  NSMenu* menuBar;
  NSMenu* rootMenu;
  NSMenuItem* disableGatekeeper;
  NSTextField* statusBar;
}
-(id) initWith:(hiro::mWindow&)window;
-(BOOL) canBecomeKeyWindow;
-(BOOL) canBecomeMainWindow;
-(void) windowDidBecomeMain:(NSNotification*)notification;
-(void) windowDidMove:(NSNotification*)notification;
-(void) windowDidResize:(NSNotification*)notification;
-(BOOL) windowShouldClose:(id)sender;
-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender;
-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender;
-(NSMenu*) menuBar;
-(void) menuAbout;
-(void) menuPreferences;
-(void) menuDisableGatekeeper;
-(void) menuQuit;
-(NSTextField*) statusBar;
@end

namespace hiro {

struct pWindow : pObject {
  Declare(Window, Object)

  auto append(sMenuBar menuBar) -> void;
  auto append(sSizable sizable) -> void;
  auto append(sStatusBar statusBar) -> void;
  auto focused() const -> bool override;
  auto frameMargin() const -> Geometry;
  auto handle() const -> uintptr_t;
  auto monitor() const -> u32;
  auto remove(sMenuBar menuBar) -> void;
  auto remove(sSizable sizable) -> void;
  auto remove(sStatusBar statusBar) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setDismissable(bool dismissable) -> void;
  auto setDroppable(bool droppable) -> void;
  auto setFocused() -> void override;
  auto setFullScreen(bool fullScreen) -> void;
  auto setGeometry(Geometry geometry) -> void;
  auto setMaximized(bool maximized) -> void;
  auto setMaximumSize(Size size) -> void;
  auto setMinimized(bool minimized) -> void;
  auto setMinimumSize(Size size) -> void;
  auto setModal(bool modal) -> void;
  auto setResizable(bool resizable) -> void;
  auto setTitle(const string& text) -> void;
  auto setAssociatedFile(const string& filename) -> void;
  auto setVisible(bool visible) -> void;

  auto moveEvent() -> void;
  auto sizeEvent() -> void;
  auto statusBarHeight() -> u32;
  auto statusBarReposition() -> void;

  auto _append(mWidget& widget) -> void;
  auto _geometry() -> Geometry;

  CocoaWindow* cocoaWindow = nullptr;
  Geometry windowedGeometry;
};

}

#endif
