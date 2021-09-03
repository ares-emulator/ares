auto StreamManager::construct() -> void {
  setCollapsible();
  setVisible(false);

  streamLabel.setText("Audio Streams").setFont(Font().setBold());
  streamList.onToggle([&](auto cell) {
    if(auto item = streamList.selected()) {
      if(auto stream = item.attribute<ares::Node::Audio::Stream>("node")) {
        stream->setMuted(!cell.checked());
      }
    }
  });
}

auto StreamManager::reload() -> void {
  streamList.reset();
  streamList.setHeadered();
  streamList.append(TableViewColumn().setIcon(Icon::Device::Speaker));
  streamList.append(TableViewColumn().setText("Name").setExpandable());
  streamList.append(TableViewColumn().setText("Channels").setAlignment(1.0));
  streamList.append(TableViewColumn().setText("Frequency").setAlignment(1.0));
  for(auto stream : ares::Node::enumerate<ares::Node::Audio::Stream>(emulator->root)) {
    TableViewItem item{&streamList};
    item.setAttribute<ares::Node::Audio::Stream>("node", stream);
    item.append(TableViewCell().setCheckable().setChecked());
    item.append(TableViewCell().setText(stream->name()));
    item.append(TableViewCell().setText(stream->channels()));
    item.append(TableViewCell().setText({u32(stream->frequency() + 0.5), "hz"}));
  }
  streamList.resizeColumns();
  streamList.column(0).setWidth(16);
}

auto StreamManager::unload() -> void {
  streamList.reset();
}
