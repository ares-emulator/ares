NodeManager::NodeManager(View* view) : PanelList(view, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  listView.onChange([&] { onChange(); });
}

auto NodeManager::show() -> void {
  root = emulator.root;
  listView.selectNone().doChange();
  refresh();
  setVisible(true);
}

auto NodeManager::hide() -> void {
  setVisible(false);
  root = {};
}

//recreate the tree after nodes have been added or removed
auto NodeManager::refresh() -> void {
  //save the currently selected node to try and reselect it after rebuilding the tree
  ares::Node::Object selected;
  if(auto item = listView.selected()) {
    selected = item.attribute<ares::Node::Object>("node");
  }

  listView.reset();
  if(root) for(auto& node : *root) refresh(node, 0);

  //try and restore the previously selected node
  for(auto& item : listView.items()) {
    if(item.attribute<ares::Node::Object>("node") == selected) {
      item.setSelected().setFocused();
    }
  }
}

auto NodeManager::refresh(ares::Node::Object node, uint depth) -> void {
  if(node->is<ares::Node::Memory>()) return;
  if(node->is<ares::Node::Graphics>()) return;
  if(node->is<ares::Node::Tracer>()) return;
  if(node->is<ares::Node::Input>()) return;
  if(node->is<ares::Node::Sprite>()) return;

  ListViewItem item{&listView};
  item.setAttribute<ares::Node::Object>("node", node);
  item.setAttribute<uint>("depth", depth);
  item.setText(name(node, depth));

  for(auto& node : *node) refresh(node, depth + 1);
}

//refresh the tree after settings have changed
auto NodeManager::refreshSettings() -> void {
  for(auto& item : listView.items()) {
    auto node = item.attribute<ares::Node::Object>("node");
    if(auto setting = node->cast<ares::Node::Setting>()) {
      uint depth = item.attribute<uint>("depth");
      item.setText(name(node, depth));
    }
  }
}

//generate a name for the panel list for a given node
//insert spacing based on depth to simulate a TreeView in a ListView widget
auto NodeManager::name(ares::Node::Object node, uint depth) -> string {
  string name;
  for(uint n : range(depth)) name.append("   ");
  name.append(node->attribute("name") ? node->attribute("name") : node->name());
  if(auto setting = node->cast<ares::Node::Setting>()) {
    name.append(": ", setting->readValue());
    if(!setting->dynamic() && setting->readLatch() != setting->readValue()) {
      name.append(" (", setting->readLatch(), ")");
    }
  }
  return name;
}

auto NodeManager::onChange() -> void {
  if(auto node = listView.selected().attribute<ares::Node::Object>("node")) {
    if(auto port = node->cast<ares::Node::Port>()) {
      portConnector.refresh(port);
      return program.setPanelItem(portConnector);
    }

    if(auto setting = node->cast<ares::Node::Setting>()) {
      settingEditor.refresh(setting);
      return program.setPanelItem(settingEditor);
    }

    if(auto inputs = node->find<ares::Node::Input>()) {
      if(inputs.first()->parent() == node) {
        inputMapper.refresh(node);
        return program.setPanelItem(inputMapper);
      }
    }

    if(auto peripheral = node->cast<ares::Node::Peripheral>()) {
      peripheralOverview.refresh(peripheral);
      return program.setPanelItem(peripheralOverview);
    }
  }

  program.setPanelItem(home);
}
