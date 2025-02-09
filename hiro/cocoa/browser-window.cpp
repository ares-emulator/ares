#if defined(Hiro_BrowserWindow)

namespace hiro {

auto pBrowserWindow::directory(BrowserWindow::State& state) -> string {
  string result;

  @autoreleasepool {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    if(state.title) [panel setTitle:[NSString stringWithUTF8String:state.title]];
    panel.canChooseDirectories = YES;
    panel.canChooseFiles = NO;
    panel.canCreateDirectories = YES;
    panel.directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:state.path]];
    if([panel runModal] == NSModalResponseOK) {
      NSString* path = panel.URLs.firstObject.path;
      if(path) {
        path = [path stringByAppendingString:@"/"];
        result = path.UTF8String;
      }
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
    if([filters count] > 0) panel.allowedFileTypes = filters;
    panel.allowsOtherFileTypes = NO;
    panel.directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:state.path]];
    if([panel runModal] == NSModalResponseOK) {
      NSString* path = panel.URLs.firstObject.path;
      BOOL isDirectory = NO;
      [[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDirectory];
      if(isDirectory) path = [path stringByAppendingString:@"/"];
      if(path) result = path.UTF8String;
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
    if([filters count] > 0) panel.allowedFileTypes = filters;
    panel.directoryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:state.path]];
    if([panel runModal] == NSModalResponseOK) {
      const char* path = panel.URL.path.UTF8String;
      if(path) result = path;
    }
  }

  return result;
}

}

#endif
