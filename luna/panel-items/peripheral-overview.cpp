PeripheralOverview::PeripheralOverview(View* parent) : PanelItem(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  header.setText("Manifest").setFont(Font().setBold());
  manifest.setFont(Font::Mono).setEditable(false).setWordWrap(false);
}

auto PeripheralOverview::show() -> void {
  setVisible(true);
}

auto PeripheralOverview::hide() -> void {
  setVisible(false);
  peripheral = {};
}

auto PeripheralOverview::refresh(ares::Node::Peripheral peripheral) -> void {
  this->peripheral = peripheral;
  manifest.setText(peripheral->manifest());
}
