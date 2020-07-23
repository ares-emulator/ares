namespace ares {

Debug _debug;

auto Debug::reset() -> void {
  _unimplementedNotices.reset();
}

auto Debug::_unimplemented(const string& text) -> void {
  //only warn about missing unimplemented functionality once per type
  if(_unimplementedNotices.find(text)) return;
  _unimplementedNotices.append(text);

  print(terminal::color::magenta("[unimplemented] "), text, "\n");
}

}
