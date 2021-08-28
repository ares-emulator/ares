#if defined(Hiro_BrowserWindow)

namespace hiro {

auto pBrowserWindow::directory(BrowserWindow::State& state) -> string {
  string result;

  @autoreleasepool {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    if(state.title) [panel setTitle:[NSString stringWithUTF8String:state.title]];
    panel.canChooseDirectories = YES;
    panel.canChooseFiles = NO;
    panel.directory = [NSString stringWithUTF8String:state.path];
    if([panel runModal] == NSOKButton) {
      NSArray* names = [panel filenames];
      const char* name = [[names objectAtIndex:0] UTF8String];
      if(name) result = name;
    }
  }

  return result;
}

auto pBrowserWindow::open(BrowserWindow::State& state) -> string {
  string result;

  @autoreleasepool {
    NSMutableArray* filters = [[NSMutableArray alloc] init];
    for(auto& rule : state.filters) {
      string pattern = rule.split("|", 1L)(1).replace("*", "").replace(".", "");
      if (pattern) {
        for(auto& extension : pattern.split(":")) {
          [filters addObject:[NSString stringWithUTF8String:extension]];
        }
      }
    }
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    if(state.title) [panel setTitle:[NSString stringWithUTF8String:state.title]];
    panel.canChooseDirectories = state.allowsFolders;
    panel.canChooseFiles = YES;
    panel.allowedFileTypes = filters;
    panel.allowsOtherFileTypes = NO;
    panel.directory = [NSString stringWithUTF8String:state.path];
    if([panel runModal] == NSOKButton) {
      NSString* name = panel.filenames.firstObject;
      BOOL isDirectory = NO;
      [[NSFileManager defaultManager] fileExistsAtPath:name isDirectory:&isDirectory];
      if(isDirectory) name = [name stringByAppendingString:@"/"];
      if(name) result = name.UTF8String;
    }
  }

  return result;
}

auto pBrowserWindow::save(BrowserWindow::State& state) -> string {
  string result;

  @autoreleasepool {
    NSMutableArray* filters = [[NSMutableArray alloc] init];
    for(auto& rule : state.filters) {
      string pattern = rule.split("|", 1L)(1).replace("*", "").replace(".", "");
      if (pattern) {
        for(auto& extension : pattern.split(":")) {
          [filters addObject:[NSString stringWithUTF8String:extension]];
        }
      }
    }
    NSSavePanel* panel = [NSSavePanel savePanel];
    if(state.title) [panel setTitle:[NSString stringWithUTF8String:state.title]];
    panel.allowedFileTypes = filters;
    panel.directory = [NSString stringWithUTF8String:state.path];
    if([panel runModal] == NSOKButton) {
      const char* name = panel.URL.path.UTF8String;
      if(name) result = name;
    }
  }

  return result;
}

}

#endif
