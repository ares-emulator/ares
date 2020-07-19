namespace ares {

struct Debug {
  auto reset() -> void;

  template<typename... P> auto unimplemented(P&&... p) -> void {
    return _unimplemented({forward<P>(p)...});
  }

private:
  auto _unimplemented(const string&) -> void;

  vector<string> _unimplementedNotices;
};

extern Debug debug;

}
