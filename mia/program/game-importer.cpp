GameImporter::GameImporter(View* parent) : Panel(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  importList.onChange([&] { eventChange(); });
  abortButton.setCollapsible().setText("Abort").onActivate([&] { processing = false; });
  closeButton.setCollapsible().setText("Close").onActivate([&] { eventClose(); });
}

auto GameImporter::import(string system, const vector<string>& files) -> void {
  importList.reset();
  abortButton.setVisible(true);
  closeButton.setVisible(false);
  resize();
  systemSelection.setEnabled(false);
  programWindow.show(*this);

  processing = true;
  u32 index = 1;
  for(auto& file : files) {
    if(!processing) break;
    messageLabel.setText({"[", index++, "/", files.size(), "] Importing ", Location::file(file), " ..."});
    Application::processEvents();

    ListViewItem item{&importList};
    auto pak = mia::Medium::create(system);
    if(mia::import(pak, file)) {
      item.setIcon(Icon::Action::Add);
    } else {
      item.setIcon(Icon::Action::Close);
      item.setForegroundColor({192, 0, 0});
    }
    item.setText(Location::file(file));
    importList.resizeColumn();
  }
  processing = false;
  messageLabel.setText("Completed.");
  abortButton.setVisible(false);
  closeButton.setVisible(true);
  resize();
  systemSelection.setEnabled(true);
}

auto GameImporter::eventChange() -> void {
  if(processing) return;
  if(auto item = importList.selected()) {
    if(auto error = item.attribute("error")) {
      messageLabel.setText({"Error: ", error, "."});
    } else {
      messageLabel.setText("OK.");
    }
  } else {
    messageLabel.setText("Completed.");
  }
}

auto GameImporter::eventClose() -> void {
  if(processing) return;
  gameManager.refresh();
}
