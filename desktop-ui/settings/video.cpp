auto VideoSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  colorAdjustmentLabel.setText("Color Adjustment").setFont(Font().setBold());
  colorAdjustmentLayout.setSize({3, 3}).setPadding(12_sx, 0);
  colorAdjustmentLayout.column(0).setAlignment(1.0);

  luminanceLabel.setText("Luminance:");
  luminanceValue.setAlignment(0.5);
  luminanceSlider.setLength(101).setPosition(settings.video.luminance * 100.0).onChange([&] {
    settings.video.luminance = luminanceSlider.position() / 100.0;
    luminanceValue.setText({luminanceSlider.position(), "%"});
    program.paletteUpdate();
  }).doChange();

  saturationLabel.setText("Saturation:");
  saturationValue.setAlignment(0.5);
  saturationSlider.setLength(201).setPosition(settings.video.saturation * 100.0).onChange([&] {
    settings.video.saturation = saturationSlider.position() / 100.0;
    saturationValue.setText({saturationSlider.position(), "%"});
    program.paletteUpdate();
  }).doChange();

  gammaLabel.setText("Gamma:");
  gammaValue.setAlignment(0.5);
  gammaSlider.setLength(101).setPosition((settings.video.gamma - 1.0) * 100.0).onChange([&] {
    settings.video.gamma = 1.0 + gammaSlider.position() / 100.0;
    gammaValue.setText({100 + gammaSlider.position(), "%"});
    program.paletteUpdate();
  }).doChange();

  emulatorSettingsLabel.setText("Emulator Settings").setFont(Font().setBold());
  colorBleedOption.setText("Color Bleed").setChecked(settings.video.colorBleed).onToggle([&] {
    settings.video.colorBleed = colorBleedOption.checked();
    if(emulator) emulator->setBoolean("Color Bleed", settings.video.colorBleed);
  });
  colorBleedLayout.setAlignment(1).setPadding(12_sx, 0);
  colorBleedHint.setText("Blurs adjacent pixels for translucency effects").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  colorEmulationOption.setText("Color Emulation").setChecked(settings.video.colorEmulation).onToggle([&] {
    settings.video.colorEmulation = colorEmulationOption.checked();
    if(emulator) emulator->setBoolean("Color Emulation", settings.video.colorEmulation);
  });
  colorEmulationLayout.setAlignment(1).setPadding(12_sx, 0);
  colorEmulationHint.setText("Matches colors to how they look on real hardware").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  interframeBlendingOption.setText("Interframe Blending").setChecked(settings.video.interframeBlending).onToggle([&] {
    settings.video.interframeBlending = interframeBlendingOption.checked();
    if(emulator) emulator->setBoolean("Interframe Blending", settings.video.interframeBlending);
  });
  interframeBlendingLayout.setAlignment(1).setPadding(12_sx, 0);
  interframeBlendingHint.setText("Emulates LCD translucency effects, but increases motion blur").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  overscanOption.setText("Overscan").setChecked(settings.video.overscan).onToggle([&] {
    settings.video.overscan = overscanOption.checked();
    if(emulator) emulator->setOverscan(settings.video.overscan);
  });
  overscanLayout.setAlignment(1).setPadding(12_sx, 0);
  overscanHint.setText("Shows extended PAL CRT lines, but these are usually blank in most games").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  renderSettingsLabel.setText("N64 Render Settings").setFont(Font().setBold());
  renderQualityLayout.setPadding(12_sx, 0);
  renderQualitySD.setText("SD Quality").onActivate([&] {
    settings.video.quality = "SD";
    renderSupersamplingOption.setChecked(false).setEnabled(false);
  });
  renderQualityHD.setText("HD Quality").onActivate([&] {
    settings.video.quality = "HD";
    renderSupersamplingOption.setChecked(settings.video.supersampling).setEnabled(true);
  });
  renderQualityUHD.setText("UHD Quality").onActivate([&] {
    settings.video.quality = "UHD";
    renderSupersamplingOption.setChecked(settings.video.supersampling).setEnabled(true);
  });
  if(settings.video.quality == "SD") renderQualitySD.setChecked();
  if(settings.video.quality == "HD") renderQualityHD.setChecked();
  if(settings.video.quality == "UHD") renderQualityUHD.setChecked();
  renderSupersamplingOption.setText("Supersampling").setChecked(settings.video.supersampling && settings.video.quality != "SD").setEnabled(settings.video.quality != "SD").onToggle([&] {
    settings.video.supersampling = renderSupersamplingOption.checked();
  });
  renderSupersamplingLayout.setAlignment(1).setPadding(12_sx, 0);
  renderSupersamplingHint.setText("Scales HD and UHD resolutions back down to SD").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  renderSettingsLayout.setPadding(12_sx, 0);
  renderSettingsHint.setText("Note: render settings changes require a game reload to take effect").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  #if !defined(VULKAN)
  //hide Vulkan-specific options if Vulkan is not available
  renderSettingsLabel.setCollapsible(true).setVisible(false);
  renderQualityLayout.setCollapsible(true).setVisible(false);
  renderSupersamplingLayout.setCollapsible(true).setVisible(false);
  renderSettingsHint.setCollapsible(true).setVisible(false);
  #endif
}
