auto TapeViewer::construct() -> void {
  setCollapsible();
  setVisible(false);

  stopped = true;

  tapeLabel.setText("Tape Viewer").setFont(Font().setBold());

  statusLabel.setText("Status: No tape loaded");
  lengthLabel.setText("Length: 0/0");
  newButton.setText("New").setEnabled(true).onActivate([&] {
    auto tape = loadButton.attribute<ares::Node::Tape>("node");
    if (tape->loaded())
      return;
    BrowserDialog dialog;
    dialog.setTitle("New Tape");
    dialog.setPath(settings.paths.home);
    dialog.setAlignment(presentation);
    dialog.setFilters({"*.wav"});
    if (auto location = dialog.saveFile()) {
      if (Encode::WAV::mono_16bit(location, {{1}}, 44100)) {
        if (emulator->loadTape(tape, location)) {
          tape->load();
        }
      }
    }
    refresh();
  });
  loadButton.setText("Load").setEnabled(true).onActivate([&] {
    auto tape = loadButton.attribute<ares::Node::Tape>("node");
    if (tape->loaded())
      return;
    if (emulator->loadTape(tape, "")) {
      tape->load();
    }
    refresh();
  });
  unloadButton.setText("Unload").setEnabled(false).onActivate([&] {
    auto tape = unloadButton.attribute<ares::Node::Tape>("node");
    if (!tape->loaded())
      return;
    if (tape->playing() || tape->recording())
      tape->stop();
    tape->unload();
    emulator->unloadTape(tape);
    refresh();
  });
  playButton.setText("Play").setEnabled(false).onActivate([&] {
    auto tape = playButton.attribute<ares::Node::Tape>("node");
    if (!tape->loaded() || tape->playing() || tape->recording())
      return;
    tape->play();
    refresh();
  });
  recordButton.setText("Record").setEnabled(false).onActivate([&] {
    auto tape = playButton.attribute<ares::Node::Tape>("node");
    if (!tape->loaded() || tape->playing() || tape->recording())
      return;
    tape->record();
    refresh();
  });
  fastForwardButton.setText("Fast Forward").setEnabled(false).onActivate([&] {
    auto tape = fastForwardButton.attribute<ares::Node::Tape>("node");
    if (!tape->loaded() || tape->playing() || tape->recording())
      return;
    tape->setPosition(tape->length());
    refresh();
  });
  rewindButton.setText("Rewind").setEnabled(false).onActivate([&] {
    auto tape = rewindButton.attribute<ares::Node::Tape>("node");
    if (!tape->loaded() || tape->playing() || tape->recording())
      return;
    tape->setPosition(0);
    refresh();
  });
  stopButton.setText("Stop").setEnabled(false).onActivate([&] {
    auto tape = stopButton.attribute<ares::Node::Tape>("node");
    if (!tape->loaded() || (!tape->playing() && !tape->recording()))
      return;
    tape->stop();
    refresh();
  });

  tapeLayout.setVisible(false);
  tapeList.setVisible(false);
}

auto TapeViewer::reload() -> void {
  tapeList.reset();
  for (auto tape: ares::Node::enumerate<ares::Node::Tape>(emulator->root)) {
    ComboButtonItem item{&tapeList};
    item.setAttribute<ares::Node::Tape>("node", tape);
    item.setText(tape->name());
  }
  eventChange();
}

auto TapeViewer::unload() -> void {
  tapeList.reset();
  eventChange();
}

auto TapeViewer::refresh() -> void {
  if (auto item = tapeList.selected()) {
    if (auto tape = item.attribute<ares::Node::Tape>("node")) {
      stopped = !(tape->playing() || tape->recording());

      string status;
      if (tape->loaded()) {
        if (tape->playing())
          status = "Playing";
        else if (tape->recording())
          status = "Recording";
        else
          status = "Stopped";
      } else {
        status = "No tape loaded";
      }
      statusLabel.setText({"Status: ", status});

      u64 position = tape->position() / tape->frequency();
      u64 length = tape->length() / tape->frequency();
      lengthLabel.setText({"Length: ", position, "/", length, " seconds"});
      newButton.setEnabled(!tape->loaded());
      loadButton.setEnabled(!tape->loaded());
      unloadButton.setEnabled(tape->loaded());
      playButton.setEnabled(tape->loaded() && tape->supportPlay() && !tape->playing() && !tape->recording());
      recordButton.setEnabled(tape->loaded() && tape->supportRecord() && !tape->playing() && !tape->recording());
      fastForwardButton.setEnabled(tape->loaded() && !tape->playing() && !tape->recording());
      rewindButton.setEnabled(tape->loaded() && !tape->playing() && !tape->recording());
      stopButton.setEnabled(tape->loaded() && (tape->playing() || tape->recording()));
    }
  }
}

auto TapeViewer::liveRefresh() -> void {
  if (!visible() || !emulator)
    return;

  if (tapeList.itemCount() != ares::Node::enumerate<ares::Node::Tape>(emulator->root).size()) {
    reload();
    return;
  }

  if (auto item = tapeList.selected()) {
    if (auto tape = item.attribute<ares::Node::Tape>("node")) {
      if (tape->playing() || tape->recording()) {
        u64 position = tape->position() / tape->frequency();
        u64 length = tape->length() / tape->frequency();
        lengthLabel.setText({"Length: ", position, "/", length, " seconds"});
      } else if (!stopped) {
        refresh();
      }
    }
  }
}

auto TapeViewer::eventChange() -> void {
  if (auto item = tapeList.selected()) {
    if (auto tape = item.attribute<ares::Node::Tape>("node")) {
      tapeLayout.setVisible(true);
      tapeList.setVisible(true);

      newButton.setAttribute<ares::Node::Tape>("node", tape);
      loadButton.setAttribute<ares::Node::Tape>("node", tape);
      unloadButton.setAttribute<ares::Node::Tape>("node", tape);
      playButton.setAttribute<ares::Node::Tape>("node", tape);
      recordButton.setAttribute<ares::Node::Tape>("node", tape);
      fastForwardButton.setAttribute<ares::Node::Tape>("node", tape);
      rewindButton.setAttribute<ares::Node::Tape>("node", tape);
      stopButton.setAttribute<ares::Node::Tape>("node", tape);
      refresh();
    }
  } else {
    tapeList.setVisible(false);
    tapeLayout.setVisible(false);
  }
}
