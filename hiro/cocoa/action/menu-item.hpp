#if defined(Hiro_MenuItem)

@interface CocoaMenuItem : NSMenuItem<CocoaMenu> {
@public
  hiro::mMenuItem* menuItem;
}
-(id) initWith:(hiro::mMenuItem&)menuItem;
-(void) activate;
@end

namespace hiro {

struct pMenuItem : pAction {
  Declare(MenuItem, Action)

  auto setIcon(const multiFactorImage& icon, bool force = false) -> void;
  auto setIconForFile(const string& filename) -> void;
  auto setText(const string& text) -> void;

  CocoaMenuItem* cocoaMenuItem = nullptr;
};

}

#endif
