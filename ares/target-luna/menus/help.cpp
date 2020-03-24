HelpMenu::HelpMenu(MenuBar* parent) : Menu(parent) {
  setText("Help");
  about.setIcon(Icon::Prompt::Question).setText("About luna ...").onActivate([&] {
    AboutDialog()
    .setName("luna")
    .setVersion(ares::Version)
  //.setCopyright(ares::Copyright)
    .setLicense(ares::License)
    .setWebsite(ares::Website)
    .setAlignment(program, {0.5f, program.panelLayout.visible() ? 0.32f : 0.5f})
    .show();
  });
}
