auto GraphicsViewer::construct() -> void {
  setCollapsible();
  setVisible(false);

  frameStatus.setCollapsible();
  commandLayout.setCollapsible();
  tickLayout.setCollapsible();
  viewLayout.setCollapsible();
  stateTable.setCollapsible();
  graphicsView.setCollapsible();
  exportButton.setCollapsible();
  exportStatus.setCollapsible();
  liveOption.setCollapsible();
  refreshButton.setCollapsible();
  captureButton.setCollapsible();
  exportFrameButton.setCollapsible();
  exportVideoButton.setCollapsible();

  graphicsLabel.setText("Graphics Viewer").setFont(Font().setBold());
  graphicsList.onChange([&] { eventChange(); });
  graphicsView.setAlignment({0.0, 0.0});
  exportButton.setText("Export").onActivate([&] {
    Program::Guard guard;
    eventExport();
  });
  liveOption.setText("Live");
  refreshButton.setText("Refresh").onActivate([&] {
    refresh();
  });
  captureButton.setText("Capture Frame").onActivate([&] {
    eventCapture();
  });
  exportFrameButton.setText("Export Text").onActivate([&] {
    eventExportFrame();
  });
  exportVideoButton.setText("Export MP4").onActivate([&] {
    eventExportVideo();
  });
  commandList.onChange([&] { eventCommand(); });
  //Monospaced so the decoded operands line up column-wise between rows.
  commandList.setFont(Font().setFamily(Font::Mono));
  commandDetail.setEditable(false).setFont(Font().setFamily(Font::Mono));

  tickLabel.setText("Ticks");
  tickSlider.setLength(1).onChange([&] { eventTick(); });

  resetStateTable();
  stateTable.setFont(Font().setFamily(Font::Mono));
  for(u32 index : range(FrameViewCount)) {
    viewPanes[index]->setCollapsible();
    viewLabels[index]->setFont(Font().setBold());
    viewCanvases[index]->setAlignment({0.0, 0.0});
  }

  frameStatus.setVisible(false);
  commandLayout.setVisible(false);
  tickLayout.setVisible(false);
  viewLayout.setVisible(false);
  stateTable.setVisible(false);
  captureButton.setVisible(false);
  exportFrameButton.setVisible(false);
  exportVideoButton.setVisible(false);
}

auto GraphicsViewer::selectedFrame() -> ares::Node::Debugger::GraphicsFrame {
  if(auto item = graphicsList.selected()) {
    return item.attribute<ares::Node::Debugger::GraphicsFrame>("frame");
  }
  return {};
}

auto GraphicsViewer::resetStateTable() -> void {
  stateTable.reset();
  stateTable.setHeadered();
  stateTable.append(TableViewColumn().setText("Field"));
  stateTable.append(TableViewColumn().setText("Value").setExpandable());
}

auto GraphicsViewer::clearFrame() -> void {
  commandList.reset();
  commandDetail.setText("");
  resetStateTable();
  for(u32 index : range(FrameViewCount)) {
    viewCanvases[index]->setIcon();
    viewPanes[index]->setVisible(false);
  }
  waitingForFrame = false;
  shownFrame = false;
  tickCommands.clear();
  tickLayout.setVisible(false);
  exportStatus.setText("");
}

auto GraphicsViewer::reload() -> void {
  graphicsList.reset();
  for(auto frame : ares::Node::enumerate<ares::Node::Debugger::GraphicsFrame>(emulator->root)) {
    ComboButtonItem item{&graphicsList};
    item.setAttribute<ares::Node::Debugger::GraphicsFrame>("frame", frame);
    item.setText(frame->name());
  }
  for(auto graphics : ares::Node::enumerate<ares::Node::Debugger::Graphics>(emulator->root)) {
    ComboButtonItem item{&graphicsList};
    item.setAttribute<ares::Node::Debugger::Graphics>("node", graphics);
    item.setText(graphics->name());
  }
  eventChange();
}

auto GraphicsViewer::unload() -> void {
  graphicsList.reset();
  clearFrame();
  eventChange();
}

auto GraphicsViewer::refresh() -> void {
  if(auto item = graphicsList.selected()) {
    if(auto frame = selectedFrame()) {
      if(frame->captureValid()) refreshFrame(frame);
      return;
    }
    if(auto graphics = item.attribute<ares::Node::Debugger::Graphics>("node")) {
      auto width  = graphics->width();
      auto height = graphics->height();
      auto input  = graphics->capture();
      u32 offset = 0;
      image view;
      view.allocate(width, height);
      for(u32 y : range(height)) {
        auto output = view.data() + y * view.pitch();
        for(u32 x : range(width)) {
          view.write(output, 255 << 24 | input[offset++]);
          output += view.stride();
        }
      }
      graphicsView.setIcon(view);
    }
  } else {
    graphicsView.setIcon();
  }
}

auto GraphicsViewer::liveRefresh() -> void {
  if(visible() && waitingForFrame) {
    if(auto frame = selectedFrame()) {
      if(frame->captureValid()) refreshFrame(frame);
    }
    return;
  }
  if(visible() && liveOption.checked()) refresh();
}

auto GraphicsViewer::eventChange() -> void {
  bool frameMode = (bool)selectedFrame();
  frameStatus.setVisible(frameMode);
  commandLayout.setVisible(frameMode);
  tickLayout.setVisible(frameMode && !tickCommands.empty());
  viewLayout.setVisible(frameMode);
  stateTable.setVisible(frameMode);
  captureButton.setVisible(frameMode);
  exportFrameButton.setVisible(frameMode && shownFrame);
  exportVideoButton.setVisible(frameMode && shownFrame && !tickCommands.empty());
  graphicsView.setVisible(!frameMode);
  exportButton.setVisible(!frameMode);
  exportStatus.setVisible(frameMode);
  liveOption.setVisible(!frameMode);
  refreshButton.setVisible(!frameMode);
  if(frameMode && !shownFrame) frameStatus.setText("Ready to capture an RDP frame.");
  resize();
  refresh();
}

auto GraphicsViewer::eventCapture() -> void {
  auto frame = selectedFrame();
  if(!frame) return;
  {
    Program::Guard guard;
    frame->requestCapture();
  }
  clearFrame();
  waitingForFrame = true;
  exportFrameButton.setVisible(false);
  exportVideoButton.setVisible(false);
  frameStatus.setText("Capture armed; waiting for the game to present its next frame.");
  resize();
}

auto GraphicsViewer::eventCommand() -> void {
  commandDetail.setText("");
  auto command = commandList.selected();
  if(!command) return;
  auto frame = selectedFrame();
  if(!frame) return;
  u32 index = command.attribute<u32>("index");
  commandDetail.setText(frame->commandDetail(index));
  syncTickToCommand(index);
  refreshFrameViews();
  refreshFrameState();
}

//The core decides which syncs are worth an entry; the slider is just an index
//into that list, so one notch is one visibly different image.
auto GraphicsViewer::refreshTicks(ares::Node::Debugger::GraphicsFrame frame) -> void {
  tickCommands = frame->ticks();
  tickSlider.setLength(max(1u, (u32)tickCommands.size()));
  tickSlider.setPosition(0);
  tickLayout.setVisible(!tickCommands.empty());
  tickLabel.setText(tickCommands.empty()
    ? string{"Ticks: none"}
    : string{"Ticks: ", tickCommands.size()});
}

auto GraphicsViewer::eventTick() -> void {
  if(tickLocked || tickCommands.empty()) return;
  u32 position = min(tickSlider.position(), (u32)tickCommands.size() - 1);
  auto item = commandList.item(tickCommands[position]);
  if(!item) return;
  tickLocked = true;
  item.setSelected().setFocused();
  tickLocked = false;
  eventCommand();
}

//Keeps the slider on the last tick at or before the selected command, so
//selecting a command in the list leaves the strip pointing at the image that
//command belongs to.
auto GraphicsViewer::syncTickToCommand(u32 index) -> void {
  if(tickLocked || tickCommands.empty()) return;
  u32 position = 0;
  for(u32 tick : range((u32)tickCommands.size())) {
    if(tickCommands[tick] > index) break;
    position = tick;
  }
  tickLocked = true;
  tickSlider.setPosition(position);
  tickLocked = false;
  tickLabel.setText({"Tick ", position + 1, "/", tickCommands.size()});
}

//Renders every view the capture offers into its own pane, so the buffers can be
//compared at a glance while scrubbing rather than switched between.
auto GraphicsViewer::refreshFrameViews() -> void {
  auto command = commandList.selected();
  if(!command) {
    for(u32 index : range(FrameViewCount)) viewPanes[index]->setVisible(false);
    return;
  }
  auto frame = selectedFrame();
  if(!frame) return;

  Program::Guard guard;
  u32 commandIndex = command.attribute<u32>("index");
  auto views = frame->views();
  for(u32 index : range(FrameViewCount)) {
    if(index >= views.size()) {
      viewPanes[index]->setVisible(false);
      continue;
    }
    viewPanes[index]->setVisible(true);
    viewLabels[index]->setText(views[index]);

    auto result = frame->render(commandIndex, index);
    if(!result.width || !result.height
    || result.pixels.size() != result.width * result.height) {
      viewCanvases[index]->setIcon();
      continue;
    }
    image canvas;
    canvas.allocate(result.width, result.height);
    for(u32 y : range(result.height)) {
      auto output = canvas.data() + y * canvas.pitch();
      for(u32 x : range(result.width)) {
        canvas.write(output, result.pixels[y * result.width + x]);
        output += canvas.stride();
      }
    }
    viewCanvases[index]->setIcon(canvas);
  }
}

//The core emits one tab-separated field/value pair per line.
auto GraphicsViewer::refreshFrameState() -> void {
  resetStateTable();

  auto command = commandList.selected();
  if(!command) return;
  auto frame = selectedFrame();
  if(!frame) return;

  Program::Guard guard;
  auto state = frame->state(command.attribute<u32>("index"));
  for(auto& line : nall::split(state, "\n")) {
    if(!line) continue;
    auto fields = nall::split(line, "\t", 1L);
    TableViewItem item{&stateTable};
    item.append(TableViewCell().setText(fields.size() > 0 ? fields[0] : string{}));
    item.append(TableViewCell().setText(fields.size() > 1 ? fields[1] : string{}));
  }
  stateTable.resizeColumns();
}

auto GraphicsViewer::eventExportFrame() -> void {
  auto frame = selectedFrame();
  if(!frame || !frame->captureValid()) return;

  Program::Guard guard;
  u32 commandCount = frame->commandCount();
  string output{"RDP Frame Capture\n", frame->summary(), "\n\n"};
  for(u32 index : range(commandCount)) {
    output.append(
      "[", pad(string{index}, 4, '0'), " | 0x", hex(frame->commandOpcode(index), 2L),
      "] ", frame->commandText(index), "\n"
    );

    //The detail pane starts with its own command heading. The export has the
    //combined index/opcode heading above, so retain only the specially formatted
    //decoded fields and raw-word section.
    auto commandDetail = frame->commandDetail(index);
    auto detail = nall::split(commandDetail, "\n\n", 1L);
    if(detail.size() > 1) output.append(detail[1]);
    output.append("\n");
  }
  if(commandCount) {
    output.append("\nFinal RDP State\n", frame->state(commandCount - 1), "\n");
  }
  auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
  auto location = emulator->locate({
    Location::notsuffix(emulator->game->location), "-rdp-frame-", datetime, ".txt"
  }, ".txt", settings.paths.debugging);
  file::write(location, output);
  print("Wrote ", location, "\n");
  exportStatus.setText({"Exported ", location});
}

//One video per channel, a frame per tick, so the capture plays back as the frame
//being drawn. The tile view is skipped: it is a composite of eight unrelated tile
//descriptors on a fixed grid, and animating that shows nothing.
//
//Frames are concatenated into one PPM stream per channel rather than encoded
//individually: PNM images may be concatenated, so each frame carries its own
//dimensions and can be resynchronised on, and both ffmpeg (-f image2pipe) and
//ImageMagick read the result directly. Views are cropped to the scissor, so a
//capture's ticks come out in several sizes; only the size most of them agree on is
//written, which also drops the offscreen and HUD passes that would not line up.
auto GraphicsViewer::eventExportVideo() -> void {
  auto frame = selectedFrame();
  if(!frame || !frame->captureValid() || tickCommands.empty()) return;

  Program::Guard guard;
  auto views = frame->views();
  std::vector<u32> channels;
  for(u32 view : range((u32)views.size())) {
    if(views[view] != "Tiles") channels.push_back(view);
  }
  if(channels.empty()) return;

  //Rendered up front, in tick order: the replay only rewinds when asked for a
  //command earlier than the last one, and all views of a tick share one replay,
  //so this walks the capture forwards exactly once. The size to keep is not known
  //until every tick has been rendered, hence holding them all.
  std::vector<std::vector<ares::Core::Debugger::GraphicsFrame::Image>> renders;
  renders.resize(channels.size());
  for(u32 command : tickCommands) {
    for(u32 channel : range((u32)channels.size())) {
      renders[channel].push_back(frame->render(command, channels[channel]));
    }
  }

  //The export gets a directory of its own, named like the text export so the two
  //are recognisable as coming from the same capture.
  auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
  auto stem = emulator->locate({
    Location::notsuffix(emulator->game->location), "-rdp-frame-", datetime, ".mp4"
  }, ".mp4", settings.paths.debugging);
  auto folder = string{Location::notsuffix(stem), "/"};
  directory::create(folder);

  string failure;
  u32 encoded = 0;
  for(u32 channel : range((u32)channels.size())) {
    u32 keepWidth = 0, keepHeight = 0, keepCount = 0;
    std::vector<u64> dimensions;
    for(auto& candidate : renders[channel]) {
      if(!candidate.width || !candidate.height) continue;
      dimensions.push_back((u64)candidate.width << 32 | candidate.height);
    }
    std::sort(dimensions.begin(), dimensions.end());
    for(u32 begin = 0; begin < dimensions.size();) {
      u32 end = begin + 1;
      while(end < dimensions.size() && dimensions[end] == dimensions[begin]) end++;
      if(end - begin > keepCount) {
        keepCount = end - begin;
        keepWidth = dimensions[begin] >> 32;
        keepHeight = dimensions[begin];
      }
      begin = end;
    }
    if(!keepCount) continue;

    auto identifier = views[channels[channel]].downcase().replace(" ", "-");
    auto framesLocation = string{folder, identifier, ".ppm"};
    auto videoLocation = string{folder, identifier, ".mp4"};

    u32 frames = 0;
    if(auto fp = file::open(framesLocation, file::mode::write)) {
      string header{"P6\n", keepWidth, " ", keepHeight, "\n255\n"};
      std::vector<u8> row(keepWidth * 3);
      for(auto& render : renders[channel]) {
        if(render.width != keepWidth || render.height != keepHeight) continue;
        if(render.pixels.size() != (u64)keepWidth * keepHeight) continue;
        fp.writes(header);
        for(u32 y : range(keepHeight)) {
          auto pixels = render.pixels.data() + (u64)y * keepWidth;
          for(u32 x : range(keepWidth)) {
            row[x * 3 + 0] = pixels[x] >> 16;
            row[x * 3 + 1] = pixels[x] >>  8;
            row[x * 3 + 2] = pixels[x] >>  0;
          }
          fp.write(row);
        }
        frames++;
      }
      fp.close();
    }
    if(!frames) { file::remove(framesLocation); continue; }

    //Looped by the filter rather than by -stream_loop: the demuxer cannot seek back
    //to the start of a PPM stream, so -stream_loop fails outright. The frame count
    //is known here, which is what the filter needs. The commas inside mod() are
    //escaped for ffmpeg's filtergraph parser, which would otherwise read them as
    //filter separators; the one before pad is a real separator. No shell involved.
    auto result = nall::execute("ffmpeg",
      "-y",
      "-f", "image2pipe",
      "-c:v", "ppm",
      "-framerate", "15",
      "-i", framesLocation,
      "-vf", string{"loop=loop=2:size=", frames,
                    ",pad=iw+mod(iw\\,2):ih+mod(ih\\,2)"},
      "-pix_fmt", "yuv420p",
      "-preset", "slow",
      videoLocation
    );
    if(!result) {
      //The frames are left behind on failure: ffmpeg or ImageMagick can be run
      //over the PPM stream by hand without recapturing.
      print("ffmpeg failed for ", videoLocation, "\n", result.error, "\n");
      if(!failure) failure = string{"ffmpeg failed; PPM frames kept in ", folder};
      continue;
    }
    file::remove(framesLocation);
    encoded++;
    print("Wrote ", videoLocation, " (", frames, " frames ", keepWidth, "x", keepHeight, ")\n");
  }

  if(failure) { exportStatus.setText(failure); return; }
  exportStatus.setText({
    "Exported ", encoded, " video", encoded == 1 ? "" : "s",
    " from ", tickCommands.size(), " ticks to ", folder
  });
}

//Commands are colored by what they do, so a capture can be skimmed for the ones
//that put pixels on screen. Syncs are dimmed rather than highlighted: they are
//numerous, carry no operands, and are only worth finding when looking for the end
//of a command list.
static auto commandColor(u32 type) -> Color {
  switch(type) {
  case 1: return {96, 160, 255};   //draws
  case 2: return {224, 160, 64};   //texture loads
  case 3: return {128, 128, 128};  //syncs
  }
  return {};  //state changes keep the theme's foreground color
}

auto GraphicsViewer::refreshFrame(ares::Node::Debugger::GraphicsFrame frame) -> void {
  Program::Guard guard;
  commandList.reset();
  commandList.setHeadered();
  commandList.append(TableViewColumn().setText("#").setAlignment(1.0));
  commandList.append(TableViewColumn().setText("Command"));
  commandList.append(TableViewColumn().setText("Arguments").setExpandable());
  for(u32 index : range(frame->commandCount())) {
    TableViewItem item{&commandList};
    item.setAttribute<u32>("index", index);
    auto color = commandColor(frame->commandType(index));
    item.append(TableViewCell().setText(index));
    item.append(TableViewCell().setText(frame->commandText(index)).setForegroundColor(color));
    item.append(TableViewCell().setText(frame->commandArguments(index)).setForegroundColor(color));
  }
  commandList.resizeColumns();
  commandList.column(0).setWidth(60_sx);
  frameStatus.setText(frame->summary());
  waitingForFrame = false;
  shownFrame = true;
  exportFrameButton.setVisible(true);
  refreshTicks(frame);
  exportVideoButton.setVisible(!tickCommands.empty());
  //Every widget whose visibility depends on the capture has been set by here;
  //one relayout picks them all up. Showing a widget without it leaves a hole in
  //the layout until something else resizes the window.
  resize();
  //Open on the first tick when there is one: it is the first point in the frame
  //with something drawn, where command 0 is always an empty buffer.
  u32 first = tickCommands.empty() ? 0 : tickCommands[0];
  if(auto item = commandList.item(first)) {
    item.setSelected().setFocused();
    eventCommand();
  }
}

auto GraphicsViewer::eventExport() -> void {
  if(auto item = graphicsList.selected()) {
    if(auto graphics = item.attribute<ares::Node::Debugger::Graphics>("node")) {
      auto width  = graphics->width();
      auto height = graphics->height();
      auto input  = graphics->capture();
      auto identifier = graphics->name().downcase().replace(" ", "-");
      auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
      auto location = emulator->locate({Location::notsuffix(emulator->game->location), "-", identifier, "-", datetime, ".png"}, ".png", settings.paths.debugging);
      Encode::PNG::RGB8(location, input.data(), width * sizeof(u32), width, height);
    }
  }
}

auto GraphicsViewer::setVisible(bool visible) -> GraphicsViewer& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
