#if defined(Hiro_Label)

@implementation CocoaLabel : NSView
{
  NSColor *_foregroundColor;
}

-(id) initWith:(hiro::mLabel&)labelReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    label = &labelReference;
  }
  self.clipsToBounds = true;
  return self;
}

-(void) resetCursorRects {
  if(auto mouseCursor = NSMakeCursor(label->mouseCursor())) {
    [self addCursorRect:self.bounds cursor:mouseCursor];
  }
}

-(void) drawRect:(NSRect)dirtyRect {
  if(auto backgroundColor = label->backgroundColor()) {
    NSColor* color = NSMakeColor(backgroundColor);
    [color setFill];
    NSRectFill(dirtyRect);
  }

  NSFont* font = hiro::pFont::create(label->font(true));
  NSColor* color = _foregroundColor;
  if(!color) {
    color = NSMakeColor(hiro::SystemColor::Label);
  }
  
  NSMutableParagraphStyle* paragraphStyle = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
  paragraphStyle.alignment = NSTextAlignmentLeft;
  paragraphStyle.lineBreakMode = NSLineBreakByClipping;
  NSDictionary* attributes = @{
    NSFontAttributeName:font,
    NSForegroundColorAttributeName:color,
    NSParagraphStyleAttributeName:paragraphStyle
  };

  auto size = hiro::pFont::size(label->font(true), label->text());
  auto alignment = label->alignment();
  if(!alignment) alignment = {0.0, 0.5};

  auto geometry = label->geometry();
  NSRect rect = {{geometry.x(), geometry.y()}, {geometry.width(), geometry.height()}};
  rect.origin.x = max(0, (geometry.width() - size.width()) * alignment.horizontal());
  rect.origin.y = max(0, (geometry.height() - size.height()) * alignment.vertical());
  rect.size.width = min(geometry.width(), size.width());
  rect.size.height = min(geometry.height(), size.height());

  NSString* string = [NSString stringWithUTF8String:label->text()];
  [string drawInRect:rect withAttributes:attributes];
}

-(void) mouseButton:(NSEvent*)event down:(BOOL)isDown {
  if(isDown) {
    switch([event buttonNumber]) {
    case 0: return label->doMousePress(hiro::Mouse::Button::Left);
    case 1: return label->doMousePress(hiro::Mouse::Button::Right);
    case 2: return label->doMousePress(hiro::Mouse::Button::Middle);
    }
  } else {
    switch([event buttonNumber]) {
    case 0: return label->doMouseRelease(hiro::Mouse::Button::Left);
    case 1: return label->doMouseRelease(hiro::Mouse::Button::Right);
    case 2: return label->doMouseRelease(hiro::Mouse::Button::Middle);
    }
  }
}

-(void) mouseEntered:(NSEvent*)event {
  label->doMouseEnter();
}

-(void) mouseExited:(NSEvent*)event {
  label->doMouseLeave();
}

-(void) mouseMove:(NSEvent*)event {
  if([event window] == nil) return;
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  label->doMouseMove({(s32)location.x, (s32)([self frame].size.height - 1 - location.y)});
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

namespace hiro {

auto pLabel::construct() -> void {
  cocoaView = cocoaLabel = [[CocoaLabel alloc] initWith:self()];
  pWidget::construct();

  setAlignment(state().alignment);
  setBackgroundColor(state().backgroundColor);
  setForegroundColor(state().foregroundColor);
  setText(state().text);
}

auto pLabel::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pLabel::minimumSize() const -> Size {
  return pFont::size(self().font(true), state().text);
}

auto pLabel::setAlignment(Alignment alignment) -> void {
  [cocoaView setNeedsDisplay:YES];
}

auto pLabel::setBackgroundColor(Color color) -> void {
  [cocoaView setNeedsDisplay:YES];
}

auto pLabel::setForegroundColor(Color color) -> void {
  ((CocoaLabel *)cocoaView)->_foregroundColor = color? NSMakeColor(color): nil;
  [cocoaView setNeedsDisplay:YES];
}

auto pLabel::setForegroundColor(SystemColor color) -> void {
  ((CocoaLabel *)cocoaView)->_foregroundColor = NSMakeColor(color);
  [cocoaView setNeedsDisplay:YES];
}

auto pLabel::setText(const string& text) -> void {
  [cocoaView setNeedsDisplay:YES];
}

}

@end
#endif
