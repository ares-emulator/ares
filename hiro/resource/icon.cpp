#include "icon.hpp"

#define ICON(x) const nall::multiFactorImage x(x##1x, x##2x)
namespace Icon {
namespace Action {
ICON(Add);
ICON(Attach);
ICON(Bookmark);
ICON(Close);
ICON(FullScreen);
ICON(Mute);
ICON(NewFile);
ICON(NewFolder);
ICON(Open);
ICON(Properties);
ICON(Quit);
ICON(Refresh);
ICON(Remove);
ICON(Save);
ICON(Search);
ICON(Settings);
ICON(Stop);
}
namespace Application {
ICON(Browser);
ICON(Calculator);
ICON(Calendar);
ICON(Chat);
ICON(FileManager);
ICON(Mail);
ICON(Monitor);
ICON(Terminal);
ICON(TextEditor);
}
namespace Device {
ICON(Clock);
ICON(Display);
ICON(Joypad);
ICON(Keyboard);
ICON(Microphone);
ICON(Mouse);
ICON(Network);
ICON(Optical);
ICON(Printer);
ICON(Speaker);
ICON(Storage);
}
namespace Edit {
ICON(Clear);
ICON(Copy);
ICON(Cut);
ICON(Delete);
ICON(Find);
ICON(Paste);
ICON(Redo);
ICON(Replace);
ICON(Undo);
}
namespace Emblem {
ICON(Archive);
ICON(Audio);
ICON(Binary);
ICON(File);
ICON(Folder);
ICON(FolderOpen);
ICON(FolderTemplate);
ICON(Font);
ICON(Image);
ICON(Markup);
ICON(Program);
ICON(Script);
ICON(Text);
ICON(Video);
}
namespace Go {
ICON(Down);
ICON(Home);
ICON(Left);
ICON(Right);
ICON(Up);
}
namespace Media {
ICON(Back);
ICON(Eject);
ICON(Flash);
ICON(Floppy);
ICON(Next);
ICON(Optical);
ICON(Pause);
ICON(Play);
ICON(Record);
ICON(Rewind);
ICON(Skip);
ICON(Stop);
}
namespace Place {
ICON(Bookmarks);
ICON(Desktop);
ICON(Home);
ICON(Server);
ICON(Settings);
ICON(Share);
}
namespace Prompt {
ICON(Error);
ICON(Information);
ICON(Question);
ICON(Warning);
}
}
#undef ICON
