#if defined(Hiro_LineEdit)

@implementation CocoaLineEdit : NSTextField

-(id) initWith:(hiro::mLineEdit&)lineEditReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    lineEdit = &lineEditReference;

    [self setDelegate:self];
    [self setTarget:self];
    [self setAction:@selector(activate:)];

    //prevent focus changes from generating activate event
    [[self cell] setSendsActionOnEndEditing:NO];
  }
  return self;
}

-(void) textDidChange:(NSNotification*)n {
  lineEdit->state.text = [[self stringValue] UTF8String];
  lineEdit->doChange();
}

-(IBAction) activate:(id)sender {
  lineEdit->doActivate();
}

@end

namespace hiro {

auto pLineEdit::construct() -> void {
  cocoaView = cocoaLineEdit = [[CocoaLineEdit alloc] initWith:self()];
  pWidget::construct();

  setBackgroundColor(state().backgroundColor);
  setEditable(state().editable);
  setForegroundColor(state().foregroundColor);
  setText(state().text);
}

auto pLineEdit::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pLineEdit::minimumSize() const -> Size {
  Size size = pFont::size(self().font(true), state().text);
  return {size.width() + 10, size.height() + 8};
}

auto pLineEdit::setBackgroundColor(Color color) -> void {
}

auto pLineEdit::setEditable(bool editable) -> void {
  [(CocoaLineEdit*)cocoaView setEditable:editable];
}

auto pLineEdit::setForegroundColor(SystemColor color) -> void {
  [[(CocoaLineEdit*)cocoaView cell] setTextColor: NSMakeColor(color)?: NSMakeColor(hiro::SystemColor::Text)];
}

auto pLineEdit::setForegroundColor(Color color) -> void {
  [[(CocoaLineEdit*)cocoaView cell] setTextColor: NSMakeColor(color)?: NSMakeColor(hiro::SystemColor::Text)];
}

auto pLineEdit::setText(const string& text) -> void {
  [(CocoaLineEdit*)cocoaView setStringValue:[NSString stringWithUTF8String:text]];
}

/*
auto pLineEdit::text() const -> string {
  return [[cocoaView stringValue] UTF8String];
}
*/

}

#endif
