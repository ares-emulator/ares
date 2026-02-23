auto TraceLogger::construct() -> void {
  setCollapsible();
  setVisible(false);

  tracerLabel.setText("Trace Logger").setFont(Font().setBold());
  tracerList.onToggle([&](TableViewCell cell) {
    if(auto item = cell.parent()) {
      if(auto tracer = item.attribute<ares::Node::Debugger::Tracer::Tracer>("tracer")) {
        if(cell.offset() == 1) tracer->setPrefix(cell.checked());
        if(cell.offset() == 2) tracer->setTerminal(cell.checked());
        if(cell.offset() == 3) tracer->setFile(cell.checked());
        if(cell.offset() == 4) {
          if(auto instruction = tracer->cast<ares::Node::Debugger::Tracer::Instruction>()) {
            instruction->setMask(cell.checked());
          }
        }
      }
    }
  });
}

auto TraceLogger::reload() -> void {
  tracerList.reset();
  tracerList.setHeadered();

  tracerList.append(TableViewColumn().setText("Name").setExpandable());
  tracerList.append(TableViewColumn().setText("Show Prefix").setAlignment(1.0));
  tracerList.append(TableViewColumn().setText("Log to Terminal").setAlignment(1.0));
  tracerList.append(TableViewColumn().setText("Log to File").setAlignment(1.0));
  tracerList.append(TableViewColumn().setText("Mask").setAlignment(1.0));


  for(auto tracer : ares::Node::enumerate<ares::Node::Debugger::Tracer::Tracer>(emulator->root)) {
    TableViewItem item{&tracerList};
    item.setAttribute<ares::Node::Debugger::Tracer::Tracer>("tracer", tracer);
    item.append(TableViewCell().setText({tracer->component(), " ", tracer->name()}));
    item.append(TableViewCell().setCheckable().setChecked(tracer->prefix()));
    item.append(TableViewCell().setCheckable().setChecked(tracer->terminal()));
    item.append(TableViewCell().setCheckable().setChecked(tracer->file()));
    if(auto instruction = tracer->cast<ares::Node::Debugger::Tracer::Instruction>()) {
      item.append(TableViewCell().setCheckable().setChecked(instruction->mask()));
    } else {
      item.append(TableViewCell());
    }
  }
}

auto TraceLogger::unload() -> void {
  tracerList.reset();
  if(fp) fp.close();
}

