#if defined(Hiro_Viewport)

@implementation CocoaViewport : NSView

-(id) initWith:(hiro::mViewport&)viewportReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    viewport = &viewportReference;
  }
  self.clipsToBounds = true;
  return self;
}

-(void) resetCursorRects {
  if(auto mouseCursor = NSMakeCursor(viewport->mouseCursor())) {
    [self addCursorRect:self.bounds cursor:mouseCursor];
  }
}

-(void) drawRect:(NSRect)rect {
  [[NSColor blackColor] setFill];
  NSRectFillUsingOperation(rect, NSCompositeSourceOver);
}

-(BOOL) acceptsFirstResponder {
  return YES;
}

-(NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender {
  return DropPathsOperation(sender);
}

-(BOOL) performDragOperation:(id<NSDraggingInfo>)sender {
  auto paths = DropPaths(sender);
  if(!paths) return NO;
  viewport->doDrop(paths);
  return YES;
}

-(void) keyDown:(NSEvent*)event {
}

-(void) keyUp:(NSEvent*)event {
}

@end

namespace hiro {

auto pViewport::construct() -> void {
  cocoaView = cocoaViewport = [[CocoaViewport alloc] initWith:self()];
  pWidget::construct();
}

auto pViewport::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pViewport::handle() const -> uintptr_t {
  return (uintptr_t)cocoaViewport;
}

auto pViewport::setDroppable(bool droppable) -> void {
  if(droppable) {
    [cocoaViewport registerForDraggedTypes:[NSArray arrayWithObject:NSFilenamesPboardType]];
  } else {
    [cocoaViewport unregisterDraggedTypes];
  }
}

auto pViewport::setFocusable(bool focusable) -> void {
  //TODO
}

}

#endif
