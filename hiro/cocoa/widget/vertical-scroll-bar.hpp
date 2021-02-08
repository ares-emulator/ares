#if defined(Hiro_VerticalScrollBar)

@interface CocoaVerticalScrollBar : NSScroller {
@public
  hiro::mVerticalScrollBar* verticalScrollBar;
}
-(id) initWith:(hiro::mVerticalScrollBar&)verticalScrollBar;
-(void) update;
-(IBAction) scroll:(id)sender;
@end

namespace hiro {

struct pVerticalScrollBar : pWidget {
  Declare(VerticalScrollBar, Widget)

  auto minimumSize() const -> Size override;
  auto setLength(u32 length) -> void;
  auto setPosition(u32 position) -> void;

  CocoaVerticalScrollBar* cocoaVerticalScrollBar = nullptr;
};

}

#endif
