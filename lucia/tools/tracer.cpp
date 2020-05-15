auto TraceLogger::construct() -> void {
  setCollapsible();
  setVisible(false);

  tracerLabel.setText("Trace Logger").setFont(Font().setBold());
  tracerList.onToggle([&](auto item) { eventToggle(item); });
  selectAllButton.setText("Select All").onActivate([&] {
    for(auto item : tracerList.items()) {
      if(item.checked()) continue;
      item.setChecked(true);
      eventToggle(item);
    }
  });
  unselectAllButton.setText("Unselect All").onActivate([&] {
    for(auto item : tracerList.items()) {
      if(!item.checked()) continue;
      item.setChecked(false);
      eventToggle(item);
    }
  });
  traceToTerminal.setText("Trace To Terminal");
}

auto TraceLogger::reload() -> void {
  tracerList.reset();
  for(auto tracer : ares::Node::enumerate<ares::Node::Tracer>(emulator->root)) {
    ListViewItem item{&tracerList};
    item.setAttribute<ares::Node::Tracer>("tracer", tracer);
    item.setCheckable();
    item.setText({tracer->component(), " ", tracer->name()});
  }
}

auto TraceLogger::unload() -> void {
  tracerList.reset();
  if(fp) fp.close();
}

auto TraceLogger::eventToggle(ListViewItem item) -> void {
  if(auto tracer = item.attribute<ares::Node::Tracer>("tracer")) {
    tracer->setEnabled(item.checked());
  }
}
