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
    if(emulator) emulator->setColorBleed(settings.video.colorBleed);
  });
  colorBleedLayout.setAlignment(1).setPadding(12_sx, 0);
  colorBleedHint.setText("Blurs adjacent pixels for translucency effects").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  colorEmulationOption.setText("Color Emulation").setChecked(settings.video.colorEmulation).onToggle([&] {
    settings.video.colorEmulation = colorEmulationOption.checked();
    if(emulator) emulator->setBoolean("Color Emulation", settings.video.colorEmulation);
  });
  colorEmulationLayout.setAlignment(1).setPadding(12_sx, 0);
  colorEmulationHint.setText("Matches colors to how they look on real hardware").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  deepBlackBoostOption.setText("Deep Black Boost").setChecked(settings.video.deepBlackBoost).onToggle([&] {
    settings.video.deepBlackBoost = deepBlackBoostOption.checked();
    if(emulator) emulator->setBoolean("Deep Black Boost", settings.video.deepBlackBoost);
  });
  deepBlackBoostLayout.setAlignment(1).setPadding(12_sx, 0);
  deepBlackBoostHint.setText("Applies a gamma ramp to crush black levels (SNES/SFC)").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
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
  overscanHint.setText("Displays the full frame without cropping 'undesirable' borders").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  pixelAccuracyOption.setText("Pixel Accuracy Mode").setChecked(settings.video.pixelAccuracy).onToggle([&] {
    settings.video.pixelAccuracy = pixelAccuracyOption.checked();
    if(emulator) emulator->setBoolean("Pixel Accuracy", settings.video.pixelAccuracy);
  });
  pixelAccuracyLayout.setAlignment(1).setPadding(12_sx, 0);
  pixelAccuracyHint.setText("Use pixel-accurate emulation where available").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  renderSettingsLabel.setText("N64 Render Settings").setFont(Font().setBold());

  renderQualityLayout.setPadding(12_sx, 0);

  disableVideoInterfaceProcessingOption.setText("Disable Video Interface Processing").setChecked(settings.video.disableVideoInterfaceProcessing).onToggle([&] {
    settings.video.disableVideoInterfaceProcessing = disableVideoInterfaceProcessingOption.checked();
    if(emulator) emulator->setBoolean("Disable Video Interface Processing", settings.video.disableVideoInterfaceProcessing);
  });
  disableVideoInterfaceProcessingLayout.setAlignment(1).setPadding(12_sx, 0);
  disableVideoInterfaceProcessingHint.setText("Disables Video Interface post processing to render image from VRAM directly").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  weaveDeinterlacingOption.setText("Weave Deinterlacing").setChecked(settings.video.weaveDeinterlacing).onToggle([&] {
    settings.video.weaveDeinterlacing = weaveDeinterlacingOption.checked();
    if(emulator) emulator->setBoolean("(Experimental) Double the perceived vertical resolution; disabled when supersampling is used", settings.video.weaveDeinterlacing);
    if(weaveDeinterlacingOption.checked() == true) {
      renderSupersamplingOption.setChecked(false).setEnabled(false);
      settings.video.supersampling = false;
    } else {
      if(settings.video.quality != "SD") renderSupersamplingOption.setEnabled(true);
    }
  });
  weaveDeinterlacingLayout.setAlignment(1).setPadding(12_sx, 0);
  weaveDeinterlacingHint.setText("Doubles the perceived vertical resolution; incompatible with supersampling").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  renderQuality1x.setText("1x Native").onActivate([&] {
    settings.video.quality = "SD";
    renderSupersamplingOption.setChecked(false).setEnabled(false);
    settings.video.supersampling = false;
    weaveDeinterlacingOption.setEnabled(true);
  });
  renderQuality2x.setText("2x Native").onActivate([&] {
    settings.video.quality = "HD";
    if(weaveDeinterlacingOption.checked() == false) renderSupersamplingOption.setChecked(settings.video.supersampling).setEnabled(true);
  });
  renderQuality4x.setText("4x Native").onActivate([&] {
    settings.video.quality = "UHD";
    if(weaveDeinterlacingOption.checked() == false) renderSupersamplingOption.setChecked(settings.video.supersampling).setEnabled(true);
  });
  if(settings.video.quality == "SD") renderQuality1x.setChecked();
  if(settings.video.quality == "HD") renderQuality2x.setChecked();
  if(settings.video.quality == "UHD") renderQuality4x.setChecked();
  renderSupersamplingOption.setText("Supersampling").setChecked(settings.video.supersampling && settings.video.quality != "SD").setEnabled(settings.video.quality != "SD").onToggle([&] {
    settings.video.supersampling = renderSupersamplingOption.checked();
    if(renderSupersamplingOption.checked() == true) {
      weaveDeinterlacingOption.setEnabled(false).setChecked(false);
      settings.video.weaveDeinterlacing = false;
    } else {
      weaveDeinterlacingOption.setEnabled(true);
    }
  });
  renderSupersamplingLayout.setAlignment(1).setPadding(12_sx, 0);
  renderSupersamplingHint.setText("Scales 2x and 4x resolutions back down to native.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  renderSettingsLayout.setPadding(12_sx, 0);
  renderSettingsHint.setText("Note: render settings changes require a game reload to take effect").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  #if !defined(VULKAN)
  //hide Vulkan-specific options if Vulkan is not available
  renderSettingsLabel.setCollapsible(true).setVisible(false);
  renderQualityLayout.setCollapsible(true).setVisible(false);
  renderSupersamplingLayout.setCollapsible(true).setVisible(false);
  renderSettingsHint.setCollapsible(true).setVisible(false);
  disableVideoInterfaceProcessingLayout.setCollapsible(true).setVisible(false);
  weaveDeinterlacingLayout.setCollapsible(true).setVisible(false);
  #endif
}
