#if defined(Hiro_PopupMenu)

@interface CocoaPopupMenu : NSMenuItem {
@public
  hiro::mPopupMenu* popupMenu;
  NSMenu* cocoaPopupMenu;
}
-(id) initWith:(hiro::mPopupMenu&)popupMenu;
-(NSMenu*) cocoaPopupMenu;
@end

namespace hiro {

struct pPopupMenu : pObject {
  Declare(PopupMenu, Object)

  auto append(sAction action) -> void;
  auto remove(sAction action) -> void;
  auto setVisible(bool visible) -> void;

  CocoaPopupMenu* cocoaPopupMenu = nullptr;
};

}

#endif
