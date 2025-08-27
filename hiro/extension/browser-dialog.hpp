#if defined(Hiro_BrowserDialog)

struct BrowserDialogWindow;

struct BrowserDialog {
  using type = BrowserDialog;

  BrowserDialog();
  auto alignment() const -> Alignment;
  auto alignmentWindow() const -> Window;
  auto filters() const -> std::vector<string>;
  auto openFile() -> string;           //one existing file
  auto openFiles() -> std::vector<string>;  //any existing files
  auto openFolder() -> string;         //one existing folder
  auto openObject() -> string;         //one existing file or folder
  auto option() -> string;
  auto path() const -> string;
  auto saveFile() -> string;           //one file
  auto selected() -> std::vector<string>;
  auto selectFolder() -> string;       //one existing folder
  auto setAlignment(Alignment = Alignment::Center) -> type&;
  auto setAlignment(sWindow relativeTo, Alignment = Alignment::Center) -> type&;
  auto setFilters(const std::vector<string>& filters = {}) -> type&;
  auto setName(const string& name = "") -> type&;
  auto setOptions(const std::vector<string>& options = {}) -> type&;
  auto setPath(const string& path = "") -> type&;
  auto setTitle(const string& title = "") -> type&;
  auto title() const -> string;

private:
  struct State {
    string action;
    Alignment alignment = Alignment::Center;
    std::vector<string> filters = {"*"};
    string name;
    std::vector<string> options;
    string path;
    sWindow relativeTo;
    string title;
  } state;

  struct Response {
    string option;
    std::vector<string> selected;
  } response;

  auto _run() -> std::vector<string>;

  friend struct BrowserDialogWindow;
};

#endif
