#if defined(Hiro_RadioLabel)
struct mRadioLabel : mWidget {
  Declare(RadioLabel)

  auto checked() const -> bool;
  auto doActivate() const -> void;
  auto group() const -> Group override;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto setChecked() -> type&;
  auto setGroup(sGroup group = {}) -> type& override;
  auto setText(const string& text = "") -> type&;
  auto text() const -> string;

//private:
  struct State {
    bool checked = false;
    sGroup group;
    function<void ()> onActivate;
    string text;
  } state;
};
#endif
