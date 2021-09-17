#include <nall/platform.hpp>
#include <nall/any.hpp>
#include <nall/chrono.hpp>
#include <nall/directory.hpp>
#include <nall/function.hpp>
#include <nall/image.hpp>
#include <nall/locale.hpp>
#include <nall/maybe.hpp>
#include <nall/path.hpp>
#include <nall/range.hpp>
#include <nall/run.hpp>
#include <nall/set.hpp>
#include <nall/shared-pointer.hpp>
#include <nall/stdint.hpp>
#include <nall/string.hpp>
#include <nall/traits.hpp>
#include <nall/unique-pointer.hpp>
#include <nall/utility.hpp>
#include <nall/vector.hpp>

using nall::any;
using nall::function;
using nall::image;
using nall::multiFactorImage;
using nall::Locale;
using nall::maybe;
using nall::nothing;
using nall::set;
using nall::shared_pointer;
using nall::shared_pointer_weak;
using nall::string;
using nall::unique_pointer;
using nall::vector;

namespace hiro {

struct Font;
struct Keyboard;

#define Declare(Name) \
  struct Name; \
  struct m##Name; \
  struct p##Name; \
  using s##Name = shared_pointer<m##Name>; \
  using w##Name = shared_pointer_weak<m##Name>; \

Declare(Object)
Declare(Group)
Declare(Timer)
Declare(Window)
Declare(StatusBar)
Declare(MenuBar)
Declare(PopupMenu)
Declare(Action)
Declare(Menu)
Declare(MenuSeparator)
Declare(MenuItem)
Declare(MenuCheckItem)
Declare(MenuRadioItem)
Declare(Sizable)
Declare(Widget)
Declare(Button)
Declare(Canvas)
Declare(CheckButton)
Declare(CheckLabel)
Declare(ComboButton)
Declare(ComboButtonItem)
Declare(ComboEdit)
Declare(ComboEditItem)
Declare(Console)
Declare(Frame)
Declare(HexEdit)
Declare(HorizontalScrollBar)
Declare(HorizontalSlider)
Declare(IconView)
Declare(IconViewItem)
Declare(Label)
Declare(LineEdit)
Declare(ProgressBar)
Declare(RadioButton)
Declare(RadioLabel)
Declare(SourceEdit)
Declare(TabFrame)
Declare(TabFrameItem)
Declare(TableView)
Declare(TableViewHeader)
Declare(TableViewColumn)
Declare(TableViewItem)
Declare(TableViewCell)
Declare(TextEdit)
Declare(TreeView)
Declare(TreeViewItem)
Declare(VerticalScrollBar)
Declare(VerticalSlider)
Declare(Viewport)

#undef Declare

enum class Orientation : u32 { Horizontal, Vertical };
enum class Navigation : u32 { Top, Bottom, Left, Right };
enum class Sort : u32 { None, Ascending, Descending };

#include "system-color.hpp"
#include "color.hpp"
#include "gradient.hpp"
#include "alignment.hpp"
#include "text-cursor.hpp"
#include "position.hpp"
#include "size.hpp"
#include "geometry.hpp"
#include "font.hpp"
#include "mouse-cursor.hpp"
#include "hotkey.hpp"
#include "application.hpp"
#include "desktop.hpp"
#include "monitor.hpp"
#include "keyboard.hpp"
#include "mouse.hpp"
#include "browser-window.hpp"
#include "message-window.hpp"
#include "attribute.hpp"

#define Declare(Name) \
  using type = m##Name; \
  operator s##Name() const { return instance; } \
  auto self() -> p##Name* { return (p##Name*)delegate; } \
  auto self() const -> const p##Name* { return (const p##Name*)delegate; } \
  auto bind(const s##Name& instance) -> void { \
    this->instance = instance; \
    setGroup(); \
    if(!abstract()) construct(); \
  } \
  auto unbind() -> void { \
    reset(); \
    destruct(); \
    instance.reset(); \
  } \
  virtual auto allocate() -> pObject*; \

#include "object.hpp"

#undef Declare
#define Declare(Name) \
  using type = m##Name; \
  operator s##Name() const { return instance; } \
  auto self() -> p##Name* { return (p##Name*)delegate; } \
  auto self() const -> const p##Name* { return (const p##Name*)delegate; } \
  auto bind(const s##Name& instance) -> void { \
    this->instance = instance; \
    setGroup(); \
    if(!abstract()) construct(); \
  } \
  auto unbind() -> void { \
    reset(); \
    destruct(); \
    instance.reset(); \
  } \
  auto allocate() -> pObject* override; \

#include "group.hpp"
#include "timer.hpp"
#include "window.hpp"
#include "status-bar.hpp"
#include "menu-bar.hpp"
#include "popup-menu.hpp"

#include "action/action.hpp"
#include "action/menu.hpp"
#include "action/menu-separator.hpp"
#include "action/menu-item.hpp"
#include "action/menu-check-item.hpp"
#include "action/menu-radio-item.hpp"

#include "sizable.hpp"

#include "widget/widget.hpp"
#include "widget/button.hpp"
#include "widget/canvas.hpp"
#include "widget/check-button.hpp"
#include "widget/check-label.hpp"
#include "widget/combo-button.hpp"
#include "widget/combo-button-item.hpp"
#include "widget/combo-edit.hpp"
#include "widget/combo-edit-item.hpp"
#include "widget/console.hpp"
#include "widget/frame.hpp"
#include "widget/hex-edit.hpp"
#include "widget/horizontal-scroll-bar.hpp"
#include "widget/horizontal-slider.hpp"
#include "widget/icon-view.hpp"
#include "widget/icon-view-item.hpp"
#include "widget/label.hpp"
#include "widget/line-edit.hpp"
#include "widget/progress-bar.hpp"
#include "widget/radio-button.hpp"
#include "widget/radio-label.hpp"
#include "widget/source-edit.hpp"
#include "widget/tab-frame.hpp"
#include "widget/tab-frame-item.hpp"
#include "widget/table-view.hpp"
#include "widget/table-view-column.hpp"
#include "widget/table-view-item.hpp"
#include "widget/table-view-cell.hpp"
#include "widget/text-edit.hpp"
#include "widget/tree-view.hpp"
#include "widget/tree-view-item.hpp"
#include "widget/vertical-scroll-bar.hpp"
#include "widget/vertical-slider.hpp"
#include "widget/viewport.hpp"

#undef Declare

#include "lock.hpp"
#include "shared.hpp"

}
