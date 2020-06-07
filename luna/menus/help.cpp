HelpMenu::HelpMenu(MenuBar* parent) : Menu(parent) {
  setText("Help");
  about.setIcon(Icon::Prompt::Question).setText("About luna ...").onActivate([&] {
    image logo{Resource::Ares::Logo};
    logo.shrink();
    AboutDialog()
    .setLogo(logo)
    .setDescription("luna — an advanced multi-system emulator")
    .setVersion(ares::Version)
    .setCopyright(ares::Copyright)
    .setLicense(ares::License, ares::LicenseURI)
    .setWebsite(ares::Website, ares::WebsiteURI)
    .setAlignment(program, {0.5f, program.panelLayout.visible() ? 0.32f : 0.5f})
    .show();
  });
}
