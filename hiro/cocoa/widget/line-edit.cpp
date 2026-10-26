#if defined(Hiro_LineEdit)

@implementation CocoaLineEdit

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
  setMasked(state().masked);
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

auto pLineEdit::setMasked(bool masked) -> void {
  auto field = (CocoaLineEdit*)cocoaView;
  auto current = (NSTextFieldCell*)[field cell];
  if([current isKindOfClass:[NSSecureTextFieldCell class]] == masked) return;

  auto replacement = masked ? (NSTextFieldCell*)[[NSSecureTextFieldCell alloc] init] : [[NSTextFieldCell alloc] init];
  [replacement setStringValue:[current stringValue]];
  [replacement setFont:[current font]];
  [replacement setTextColor:[current textColor]];
  [replacement setBackgroundColor:[current backgroundColor]];
  [replacement setAlignment:[current alignment]];
  [replacement setBordered:[current isBordered]];
  [replacement setBezeled:[current isBezeled]];
  [replacement setDrawsBackground:[current drawsBackground]];
  [replacement setEditable:[current isEditable]];
  [replacement setSelectable:[current isSelectable]];
  [replacement setSendsActionOnEndEditing:NO];
  [field setCell:replacement];
}

auto pLineEdit::setForegroundColor(SystemColor color) -> void {
  [[(CocoaLineEdit*)cocoaView cell] setTextColor: NSMakeColor(color)];
}

auto pLineEdit::setForegroundColor(Color color) -> void {
  [[(CocoaLineEdit*)cocoaView cell] setTextColor: color ? NSMakeColor(color) : NSMakeColor(hiro::SystemColor::Text)];
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
