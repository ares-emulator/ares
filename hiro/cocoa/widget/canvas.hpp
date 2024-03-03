#if defined(Hiro_Canvas)

@interface CocoaCanvas : NSView {
@public
  hiro::mCanvas* canvas;
}
@property BOOL clipsToBounds;
-(id) initWith:(hiro::mCanvas&)canvas;
-(void) resetCursorRects;
-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender;
-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender;
-(void) mouseButton:(NSEvent*)event down:(BOOL)isDown;
-(void) mouseEntered:(NSEvent*)event;
-(void) mouseExited:(NSEvent*)event;
-(void) mouseMove:(NSEvent*)event;
-(void) mouseDown:(NSEvent*)event;
-(void) mouseUp:(NSEvent*)event;
-(void) mouseDragged:(NSEvent*)event;
-(void) rightMouseDown:(NSEvent*)event;
-(void) rightMouseUp:(NSEvent*)event;
-(void) rightMouseDragged:(NSEvent*)event;
-(void) otherMouseDown:(NSEvent*)event;
-(void) otherMouseUp:(NSEvent*)event;
-(void) otherMouseDragged:(NSEvent*)event;
@end

namespace hiro {

struct pCanvas : pWidget {
  Declare(Canvas, Widget)

  auto minimumSize() const -> Size;
  auto setAlignment(Alignment) -> void;
  auto setColor(Color color) -> void;
  auto setDroppable(bool droppable) -> void override;
  auto setFocusable(bool focusable) -> void override;
  auto setGeometry(Geometry geometry) -> void override;
  auto setGradient(Gradient gradient) -> void;
  auto setIcon(const image& icon) -> void;
  auto update() -> void;

  CocoaCanvas* cocoaCanvas = nullptr;
  NSImage* surface = nullptr;
  NSBitmapImageRep* bitmap = nullptr;
  u32 surfaceWidth = 0;
  u32 surfaceHeight = 0;
};

}

#endif
