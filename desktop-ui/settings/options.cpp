struct NANDType {
  string name;
  u8 id[4];
};

const NANDType NANDs64[] =  {
  { "Samsung K9F1208U0M", { 0xec, 0x76, 0xa5, 0xc0 } },
  { "Toshiba TC58512FT",  { 0x98, 0x76, 0x00, 0x00 } },
  { "ST NAND512-A",       { 0x20, 0x76, 0x00, 0x00 } },
};

const NANDType NANDs128[] =  {
  { "Samsung K9K1G08U0M", { 0xec, 0x79, 0xa5, 0xc0 } },
};

auto OptionSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  commonSettingsLabel.setText("Emulator Options").setFont(Font().setBold());

  rewind.setText("Rewind").setChecked(settings.general.rewind).onToggle([&] {
    settings.general.rewind = rewind.checked();
    program.rewindReset();
  }).doToggle();
  rewindLayout.setAlignment(1).setPadding(12_sx, 0);
      rewindHint.setText("Allows you to reverse time via the rewind hotkey").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  runAhead.setText("Run-Ahead").setEnabled(co_serializable()).setChecked(settings.general.runAhead && co_serializable()).onToggle([&] {
    settings.general.runAhead = runAhead.checked() && co_serializable();
    program.runAheadUpdate();
  });
  runAheadLayout.setAlignment(1).setPadding(12_sx, 0);
      runAheadHint.setText("Removes one frame of input lag, but doubles system requirements").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  autoSaveMemory.setText("Auto-Save Memory Periodically").setChecked(settings.general.autoSaveMemory).onToggle([&] {
    settings.general.autoSaveMemory = autoSaveMemory.checked();
  });
  autoSaveMemoryLayout.setAlignment(1).setPadding(12_sx, 0);
      autoSaveMemoryHint.setText("Helps safeguard game saves from being lost").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  homebrewMode.setText("Homebrew Development Mode").setChecked(settings.general.homebrewMode).onToggle([&] {
    settings.general.homebrewMode = homebrewMode.checked();
  });
  homebrewModeLayout.setAlignment(1).setPadding(12_sx, 0);
      homebrewModeHint.setText("Activate core-specific features to help homebrew developers").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  forceInterpreter.setText("Force Interpreter").setChecked(settings.general.forceInterpreter).onToggle([&] {
    settings.general.forceInterpreter = forceInterpreter.checked();
  });
  forceInterpreterLayout.setAlignment(1).setPadding(12_sx, 0);
      forceInterpreterHint.setText("(Slow) Enable interpreter for cores that default to a recompiler").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  nintendo64SettingsLabel.setText("Nintendo 64 Settings").setFont(Font().setBold());

  nintendo64ExpansionPakOption.setText("4MB Expansion Pak").setChecked(settings.nintendo64.expansionPak).onToggle([&] {
    settings.nintendo64.expansionPak = nintendo64ExpansionPakOption.checked();
  });
  nintendo64ExpansionPakLayout.setAlignment(1).setPadding(12_sx, 0);
      nintendo64ExpansionPakHint.setText("Enable/Disable the 4MB Expansion Pak").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  
  iQuePlayer64MiBNANDPresets.reset();
  for(auto nand : NANDs64) {
    iQuePlayer64MiBNANDPresets.append(ComboButtonItem().setText(nand.name));
  }
  iQuePlayer64MiBNANDPresets.items().first().setSelected();
  iQuePlayer64MiBNANDPresets.onChange([&](){
    for(auto nand : NANDs64) {
      if(nand.name == iQuePlayer64MiBNANDPresets.selected().text()) {
        memcpy(settings.nintendo64.nand64, nand.id, 4);
        for(auto n : range(4))
          iQuePlayer64MiBNANDID[n].setText(hex(settings.nintendo64.nand64[n], 2L));
      }
    }
  });

  for(auto n : range(4)) {
    iQuePlayer64MiBNANDID[n].setText(hex(settings.nintendo64.nand64[n], 2L));
    iQuePlayer64MiBNANDID[n].setEditable(true);
    iQuePlayer64MiBNANDID[n].onChange([&, n](){
      settings.nintendo64.nand64[n] = iQuePlayer64MiBNANDID[n].text().hex();
      string portStr = hex(settings.nintendo64.nand64[n], 2L);
      string portStrSmall = hex(settings.nintendo64.nand64[n]);

      if((portStr != iQuePlayer64MiBNANDID[n].text()) && (portStrSmall != iQuePlayer64MiBNANDID[n].text())) {
        iQuePlayer64MiBNANDID[n].setText(portStr);
      }

      for(auto n : range(iQuePlayer64MiBNANDPresets.itemCount())) {
        if(memcmp(NANDs64[n].id, settings.nintendo64.nand64, 4) == 0)
          iQuePlayer64MiBNANDPresets.item(n).setSelected();
      }
    });
  }

  iQuePlayer64MiBNANDID[0].doChange();

  iQuePlayer64MiBNANDLayout.setAlignment(1).setPadding(12_sx, 0);
      iQuePlayer64MiBNANDHint.setText("Set the ID for 64 MiB NANDs").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  iQuePlayer128MiBNANDPresets.reset();
  for(auto nand : NANDs128) {
    iQuePlayer128MiBNANDPresets.append(ComboButtonItem().setText(nand.name));
  }
  iQuePlayer128MiBNANDPresets.items().first().setSelected();
  iQuePlayer128MiBNANDPresets.onChange([&](){
    for(auto nand : NANDs128) {
      if(nand.name == iQuePlayer128MiBNANDPresets.selected().text()) {
        memcpy(settings.nintendo64.nand128, nand.id, 4);
        for(auto n : range(4))
          iQuePlayer128MiBNANDID[n].setText(hex(settings.nintendo64.nand128[n], 2L));
      }
    }
  });

  for(auto n : range(4)) {
    iQuePlayer128MiBNANDID[n].setText(hex(settings.nintendo64.nand128[n], 2L));
    iQuePlayer128MiBNANDID[n].setEditable(true);
    iQuePlayer128MiBNANDID[n].onChange([&, n](){
      settings.nintendo64.nand128[n] = iQuePlayer128MiBNANDID[n].text().hex();
      string portStr = hex(settings.nintendo64.nand128[n], 2L);
      string portStrSmall = hex(settings.nintendo64.nand128[n]);

      if((portStr != iQuePlayer128MiBNANDID[n].text()) && (portStrSmall != iQuePlayer128MiBNANDID[n].text())) {
        iQuePlayer128MiBNANDID[n].setText(portStr);
      }

      for(auto n : range(iQuePlayer128MiBNANDPresets.itemCount())) {
        if(memcmp(NANDs64[n].id, settings.nintendo64.nand128, 4) == 0)
          iQuePlayer128MiBNANDPresets.item(n).setSelected();
      }
    });
  }

  iQuePlayer128MiBNANDID[0].doChange();

  iQuePlayer128MiBNANDLayout.setAlignment(1).setPadding(12_sx, 0);
      iQuePlayer128MiBNANDHint.setText("Set the ID for 128 MiB NANDs").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  megaDriveSettingsLabel.setText("Mega Drive Settings").setFont(Font().setBold());

  megaDriveTmssOption.setText("TMSS Boot Rom").setChecked(settings.megadrive.tmss).onToggle([&] {
    settings.megadrive.tmss = megaDriveTmssOption.checked();
  });
  megaDriveTmssLayout.setAlignment(1).setPadding(12_sx, 0);
    megaDriveTmssHint.setText("Enable/Disable the TMSS Boot Rom at system initialization").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

}
