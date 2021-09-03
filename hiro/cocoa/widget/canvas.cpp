#if defined(Hiro_Canvas)

@implementation CocoaCanvas : NSView

-(id) initWith:(hiro::mCanvas&)canvasReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    canvas = &canvasReference;
    NSTrackingArea* area = [[NSTrackingArea alloc] initWithRect:[self frame]
      options:NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveInKeyWindow | NSTrackingInVisibleRect
      owner:self userInfo:nil
    ];
    [self addTrackingArea:area];
  }
  return self;
}

-(void) resetCursorRects {
  if(auto mouseCursor = NSMakeCursor(canvas->mouseCursor())) {
    [self addCursorRect:self.bounds cursor:mouseCursor];
  }
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
  return DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
  auto paths = DropPaths(sender);
  if(!paths) return NO;
  canvas->doDrop(paths);
  return YES;
}

-(void) mouseButton:(NSEvent*)event down:(BOOL)isDown {
  if(isDown) {
    switch([event buttonNumber]) {
    case 0: return canvas->doMousePress(hiro::Mouse::Button::Left);
    case 1: return canvas->doMousePress(hiro::Mouse::Button::Right);
    case 2: return canvas->doMousePress(hiro::Mouse::Button::Middle);
    }
  } else {
    switch([event buttonNumber]) {
    case 0: return canvas->doMouseRelease(hiro::Mouse::Button::Left);
    case 1: return canvas->doMouseRelease(hiro::Mouse::Button::Right);
    case 2: return canvas->doMouseRelease(hiro::Mouse::Button::Middle);
    }
  }
}

-(void) mouseEntered:(NSEvent*)event {
  canvas->doMouseEnter();
}

-(void) mouseExited:(NSEvent*)event {
  canvas->doMouseLeave();
}

-(void) mouseMove:(NSEvent*)event {
  if([event window] == nil) return;
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  canvas->doMouseMove({(s32)location.x, (s32)([self frame].size.height - 1 - location.y)});
}

-(void) mouseDown:(NSEvent*)event {
  [self mouseButton:event down:YES];
}

-(void) mouseUp:(NSEvent*)event {
  [self mouseButton:event down:NO];
}

-(void) mouseDragged:(NSEvent*)event {
  [self mouseMove:event];
}

-(void) rightMouseDown:(NSEvent*)event {
  [self mouseButton:event down:YES];
}

-(void) rightMouseUp:(NSEvent*)event {
  [self mouseButton:event down:NO];
}

-(void) rightMouseDragged:(NSEvent*)event {
  [self mouseMove:event];
}

-(void) otherMouseDown:(NSEvent*)event {
  [self mouseButton:event down:YES];
}

-(void) otherMouseUp:(NSEvent*)event {
  [self mouseButton:event down:NO];
}

-(void) otherMouseDragged:(NSEvent*)event {
  [self mouseMove:event];
}

-(void) drawRect:(NSRect)dirtyRect {
  NSRect frame = self.bounds;
  s32 width = frame.size.width;
  s32 height = frame.size.height;
  if(auto icon = canvas->state.icon) {
    NSRect alignedFrame = NSMakeRect(canvas->state.alignment.horizontal() * (width - (s32)icon.width()),
                                     (1 - canvas->state.alignment.vertical()) * (height - (s32)icon.height()),
                                     icon.width(), icon.height());
    [NSGraphicsContext currentContext].imageInterpolation = NSImageInterpolationNone;
    [NSMakeImage(icon) drawInRect:alignedFrame];
  } else if(auto& gradient = canvas->state.gradient) {
    auto& colors = gradient.state.colors;
    image fill;
    fill.allocate(width, height);
    fill.gradient(colors[0].value(), colors[1].value(), colors[2].value(), colors[3].value());
    [NSMakeImage(fill) drawInRect:frame];
  } else {
    [NSMakeColor(canvas->state.color) set];
    NSRectFill(dirtyRect);
  }
}

@end

namespace hiro {

auto pCanvas::construct() -> void {
  cocoaView = cocoaCanvas = [[CocoaCanvas alloc] initWith:self()];
  pWidget::construct();
}

auto pCanvas::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pCanvas::minimumSize() const -> Size {
  if(auto& icon = state().icon) return {(s32)icon.width(), (s32)icon.height()};
  return {0, 0};
}

auto pCanvas::setAlignment(Alignment) -> void {
  update();
}

auto pCanvas::setColor(Color color) -> void {
  update();
}

auto pCanvas::setDroppable(bool droppable) -> void {
  if(droppable) {
    [cocoaCanvas registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
  } else {
    [cocoaCanvas unregisterDraggedTypes];
  }
}

auto pCanvas::setFocusable(bool focusable) -> void {
  //TODO
}

auto pCanvas::setGeometry(Geometry geometry) -> void {
  pWidget::setGeometry(geometry);
  update();
}

auto pCanvas::setGradient(Gradient gradient) -> void {
  update();
}

auto pCanvas::setIcon(const image& icon) -> void {
  update();
}

auto pCanvas::update() -> void {
  [cocoaView setNeedsDisplay:YES];
}

}

#endif
