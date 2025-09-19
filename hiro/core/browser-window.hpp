#if defined(Hiro_BrowserWindow)
struct BrowserWindow {
  using type = BrowserWindow;

  auto directory() -> string;
  auto open() -> string;
  auto save() -> string;
  auto setFilters(const std::vector<string>& filters = {"*"}) -> type&;
  auto setParent(sWindow parent) -> type&;
  auto setPath(const string& path = "") -> type&;
  auto setTitle(const string& title = "") -> type&;
  auto setAllowsFolders(bool allows = false) -> type&;
  auto allowsFolders() const -> bool;

//private:
  struct State {
    std::vector<string> filters;
    sWindow parent;
    string path;
    string title;
    bool allowsFolders;
  } state;
};
#endif
