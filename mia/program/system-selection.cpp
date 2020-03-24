SystemSelection::SystemSelection(View* view) : Panel(view, Size{155_sx, ~0}) {
  setCollapsible().setVisible(false);

  for(auto& medium : media) {
    ListViewItem item{&systemList};
    item.setText(medium->name());
  }

  systemList.onChange([&] { eventChange(); });
}

auto SystemSelection::eventChange() -> void {
  if(auto item = systemList.selected()) {
    gameManager.select(item.text());
  } else {
    programWindow.show(home);
  }
}
