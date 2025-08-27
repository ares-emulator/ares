namespace ares {

Debug _debug;

auto Debug::reset() -> void {
  _totalNotices = 0;
  _unhandledNotices.clear();
  _unimplementedNotices.clear();
  _unusualNotices.clear();
  _unverifiedNotices.clear();
}

auto Debug::_unhandled(const string& text) -> void {
  if(std::find(_unhandledNotices.begin(), _unhandledNotices.end(), text) != _unhandledNotices.end()) return;
  if(_totalNotices++ > 256) return;
  _unhandledNotices.push_back(text);

  print(terminal::color::yellow("[unhandled] "), text, "\n");
}

auto Debug::_unimplemented(const string& text) -> void {
  if(std::find(_unimplementedNotices.begin(), _unimplementedNotices.end(), text) != _unimplementedNotices.end()) return;
  if(_totalNotices++ > 256) return;
  _unimplementedNotices.push_back(text);

  print(terminal::color::magenta("[unimplemented] "), text, "\n");
}

auto Debug::_unusual(const string& text) -> void {
  if(std::find(_unusualNotices.begin(), _unusualNotices.end(), text) != _unusualNotices.end()) return;
  if(_totalNotices++ > 256) return;
  _unusualNotices.push_back(text);

  print(terminal::color::cyan("[unusual] "), text, "\n");
}

auto Debug::_unverified(const string& text) -> void {
  if(std::find(_unverifiedNotices.begin(), _unverifiedNotices.end(), text) != _unverifiedNotices.end()) return;
  if(_totalNotices++ > 256) return;
  _unverifiedNotices.push_back(text);

  print(terminal::color::gray("[unverified] "), text, "\n");
}

}
