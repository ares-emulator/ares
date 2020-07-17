auto VideoSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  colorAdjustmentLabel.setText("Color Adjustment").setFont(Font().setBold());
  colorAdjustmentLayout.setSize({3, 3});
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
  colorBleedHint.setText("Blurs adjacent pixels for translucency effects").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});
  colorEmulationOption.setText("Color Emulation").setChecked(settings.video.colorEmulation).onToggle([&] {
    settings.video.colorEmulation = colorEmulationOption.checked();
    if(emulator) emulator->setBoolean("Color Emulation", settings.video.colorEmulation);
  });
  colorEmulationHint.setText("Matches colors to how they look on real hardware").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});
  interframeBlendingOption.setText("Interframe Blending").setChecked(settings.video.interframeBlending).onToggle([&] {
    settings.video.interframeBlending = interframeBlendingOption.checked();
    if(emulator) emulator->setBoolean("Interframe Blending", settings.video.interframeBlending);
  });
  interframeBlendingHint.setText("Emulates LCD translucency effects, but increases motion blur").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});
  overscanOption.setText("Overscan").setChecked(settings.video.overscan).onToggle([&] {
    settings.video.overscan = overscanOption.checked();
    if(emulator) emulator->setOverscan(settings.video.overscan);
  });
  overscanLabel.setText("Shows extended PAL CRT lines, but these are usually blank in most games").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});
}
