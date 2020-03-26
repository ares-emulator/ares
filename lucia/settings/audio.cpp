auto AudioSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  effectsLabel.setText("Effects").setFont(Font().setBold());
  effectsLayout.setSize({3, 2});
  effectsLayout.column(0).setAlignment(1.0);

  volumeLabel.setText("Volume:");
  volumeValue.setAlignment(0.5);
  volumeSlider.setLength(201).setPosition(settings.audio.volume * 100.0).onChange([&] {
    settings.audio.volume = volumeSlider.position() / 100.0;
    volumeValue.setText({volumeSlider.position(), "%"});
  }).doChange();

  balanceLabel.setText("Balance:");
  balanceValue.setAlignment(0.5);
  balanceSlider.setLength(101).setPosition((settings.audio.balance * 50.0) + 50.0).onChange([&] {
    settings.audio.balance = ((int)balanceSlider.position() - 50.0) / 50.0;
    balanceValue.setText({balanceSlider.position(), "%"});
  }).doChange();
}
