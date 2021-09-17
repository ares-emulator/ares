auto NSMakeColor(const hiro::Color& color) -> NSColor* {
  return [NSColor colorWithRed:(color.red() / 255.0) green:(color.green() / 255.0) blue:(color.blue() / 255.0) alpha:(color.alpha() / 255.0)];
}

auto NSMakeColor(const hiro::SystemColor color) -> NSColor* {
  switch(color) {
    case hiro::SystemColor::Text: return [NSColor textColor];
    case hiro::SystemColor::Label:
      if([NSColor respondsToSelector:@selector(labelColor)]) return [NSColor labelColor];
      return [NSColor textColor];
    case hiro::SystemColor::Sublabel:
      if([NSColor respondsToSelector:@selector(secondaryLabelColor)]) return [NSColor secondaryLabelColor];
      return [NSColor colorWithWhite:2/3.0 alpha:1.0];
    case hiro::SystemColor::Link:
      if([NSColor respondsToSelector:@selector(linkColor)]) return [NSColor linkColor];
      return [NSColor colorWithRed:0.21 green:0.48 blue:1.0 alpha:1.0];
    case hiro::SystemColor::PlaceholderText:
      if([NSColor respondsToSelector:@selector(placeholderTextColor)]) return [NSColor placeholderTextColor];
      return [NSColor colorWithWhite:0.5 alpha:1.0];
  }
  return nil;
}

auto NSMakeCursor(const hiro::MouseCursor& mouseCursor) -> NSCursor* {
  if(mouseCursor == hiro::MouseCursor::Hand) return [NSCursor pointingHandCursor];
  if(mouseCursor == hiro::MouseCursor::HorizontalResize) return [NSCursor resizeLeftRightCursor];
  if(mouseCursor == hiro::MouseCursor::VerticalResize) return [NSCursor resizeUpDownCursor];
  return nil;
}

auto NSMakeImage(image icon, u32 scaleWidth = 0, u32 scaleHeight = 0) -> NSImage* {
  if(!icon) return nil;

  if(scaleWidth && scaleHeight) icon.scale(scaleWidth, scaleHeight);
  icon.transform(0, 32, 255u << 24, 255u << 0, 255u << 8, 255u << 16);  //Cocoa stores images in ABGR format

  //create NSImage from memory
  NSImage* cocoaImage = [[NSImage alloc] initWithSize:NSMakeSize(icon.width(), icon.height())];
  NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc]
    initWithBitmapDataPlanes:nil
    pixelsWide:icon.width() pixelsHigh:icon.height()
    bitsPerSample:8 samplesPerPixel:4 hasAlpha:YES
    isPlanar:NO colorSpaceName:NSDeviceRGBColorSpace
    bitmapFormat:NSAlphaNonpremultipliedBitmapFormat
    bytesPerRow:(4 * icon.width()) bitsPerPixel:32
  ];
  memory::copy<u32>([bitmap bitmapData], icon.data(), icon.width() * icon.height());
  [cocoaImage addRepresentation:bitmap];
  return cocoaImage;
}

auto NSMakeImage(multiFactorImage icon, u32 scaleWidth = 0, u32 scaleHeight = 0) -> NSImage* {
  if(!icon) return nil;
  NSImage* cocoaImage = NSMakeImage(icon.lowDPI(), scaleWidth, scaleHeight);
  if(!icon.highDPI()) return cocoaImage;
  NSImage* retinaCocoaImage = NSMakeImage(icon.highDPI(), scaleWidth * 2, scaleHeight * 2);
  [cocoaImage addRepresentation:retinaCocoaImage.representations[0]];
  
  return cocoaImage;
}

auto DropPathsOperation(id<NSDraggingInfo> sender) -> NSDragOperation {
  NSPasteboard* pboard = [sender draggingPasteboard];
  if([[pboard types] containsObject:NSFilenamesPboardType]) {
    if([sender draggingSourceOperationMask] & NSDragOperationGeneric) {
      return NSDragOperationGeneric;
    }
  }
  return NSDragOperationNone;
}

auto DropPaths(id<NSDraggingInfo> sender) -> vector<string> {
  vector<string> paths;
  NSPasteboard* pboard = [sender draggingPasteboard];
  if([[pboard types] containsObject:NSFilenamesPboardType]) {
    NSArray* files = [pboard propertyListForType:NSFilenamesPboardType];
    for(u32 n = 0; n < [files count]; n++) {
      string path = [[files objectAtIndex:n] UTF8String];
      if(directory::exists(path) && !path.endsWith("/")) path.append("/");
      paths.append(path);
    }
  }
  return paths;
}
