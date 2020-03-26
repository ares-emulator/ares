SettingsMenu::SettingsMenu(MenuBar* parent) : Menu(parent) {
  setText("Settings");
  outputMenu.setText("Output").setIcon(Icon::Emblem::Image);
  outputCenter.setText("Center").onActivate([&] {
    settings.video.output = "Center";
  });
  outputScale.setText("Scale").onActivate([&] {
    settings.video.output = "Scale";
  });
  outputStretch.setText("Stretch").onActivate([&] {
    settings.video.output = "Stretch";
  });
  if(settings.video.output == "Center") outputCenter.setChecked();
  if(settings.video.output == "Scale") outputScale.setChecked();
  if(settings.video.output == "Stretch") outputStretch.setChecked();
  aspectCorrection.setText("Aspect Correction").setChecked(settings.video.aspectCorrection).onToggle([&] {
    settings.video.aspectCorrection = aspectCorrection.checked();
  });
  scaleMenu.setText("Scale").setIcon(Icon::Emblem::Image);
  scale2x.setText("640x480").onActivate([&] {
    settings.video.scale = 2;
    program.resize();
  });
  scale3x.setText("960x720").onActivate([&] {
    settings.video.scale = 3;
    program.resize();
  });
  scale4x.setText("1280x960").onActivate([&] {
    settings.video.scale = 4;
    program.resize();
  });
  shaderMenu.setText("Shader").setIcon(Icon::Emblem::Image).setEnabled(false);
  muteAudio.setText("Mute Audio").setChecked(settings.audio.mute).onToggle([&] {
    settings.audio.mute = muteAudio.checked();
    emulator.audioUpdateEffects();
  });
  showStatusBar.setText("Show Status Bar").setChecked(settings.interface.showStatusBar).onToggle([&] {
    settings.interface.showStatusBar = showStatusBar.checked();
    if(showStatusBar.checked()) {
      program.showStatus();
    } else {
      program.hideStatus();
    }
  });
  showSystemPanels.setText("Show System Panels").setChecked(settings.interface.showSystemPanels).onToggle([&] {
    settings.interface.showSystemPanels = showSystemPanels.checked();
    if(showSystemPanels.checked()) {
      program.showPanels();
    } else {
      program.hidePanels();
    }
  });
}

auto SettingsMenu::updateShaders() -> void {
  shaderMenu.reset();
  if(!videoInstance.ready() || !videoInstance.hasShader()) {
    shaderMenu.setEnabled(false);
    return;
  }
  shaderMenu.setEnabled();

  Group shaders;

  MenuRadioItem none{&shaderMenu};
  none.setText("None").onActivate([&] {
    settings.video.shader = "None";
    emulator.videoUpdateShader();
  });
  shaders.append(none);

  MenuRadioItem blur{&shaderMenu};
  blur.setText("Blur").onActivate([&] {
    settings.video.shader = "Blur";
    emulator.videoUpdateShader();
  });
  shaders.append(blur);

  string location{Path::settings, "Shaders/"};

  if(settings.video.driver == "OpenGL 3.2") {
    for(auto shaderName : directory::folders(location, "*.shader")) {
      if(shaders.objectCount() == 2) shaderMenu.append(MenuSeparator());
      MenuRadioItem item{&shaderMenu};
      item.setText(Location::prefix(shaderName)).onActivate([=] {
        settings.video.shader = {location, shaderName};
        emulator.videoUpdateShader();
      });
      shaders.append(item);
    }
  }

  if(settings.video.shader == "None") none.setChecked();
  if(settings.video.shader == "Blur") blur.setChecked();
  for(auto item : shaders.objects<MenuRadioItem>()) {
    if(settings.video.shader == string{location, item.text(), ".shader/"}) {
      item.setChecked();
    }
  }
}
