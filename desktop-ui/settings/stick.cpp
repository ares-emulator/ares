auto StickSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  nintendo64ControlStickSettingsLabel.setText("Nintendo 64 Control Stick Settings").setFont(Font().setBold());
  nintendo64ControlStickSettingsLayout.setSize({3, 3}).setPadding(12_sx, 6_sy);
  nintendo64ControlStickSettingsLayout.column(0).setAlignment(1.0);

  for(auto& opt : array<string[10]>{"Octagon (Virtual) (Default)", "Circle (Diagonal)", "Circle (Cardinal)", "Circle (Maximum)", "Octagon (Morphed)", "Square (Maximum Virtual)", "Square (Maximum Morphed)", "Custom Octagon (Virtual)", "Custom Circle", "Custom Octagon (Morphed)"}) {
    ComboButtonItem item{&outputStyleOption};
    item.setText(opt);
    if(opt == settings.stick.outputStyleString) {
      item.setSelected();
      customMaxOutputSlider.setEnabled(false);

      if(opt == "Octagon (Virtual) (Default)") {
        settings.stick.outputStyleChoice = "Octagon (Virtual) (Default)";
      } else if(opt == "Circle (Diagonal)") {
        settings.stick.outputStyleChoice = "Circle (Diagonal)";
      } else if(opt == "Circle (Cardinal)") {
        settings.stick.outputStyleChoice = "Circle (Cardinal)";
      } else if(opt == "Circle (Maximum)") {
        settings.stick.outputStyleChoice = "Circle (Maximum)";
      } else if(opt == "Octagon (Morphed)") {
        settings.stick.outputStyleChoice = "Octagon (Morphed)";
      } else if(opt == "Square (Maximum Virtual)") {
        settings.stick.outputStyleChoice = "Square (Maximum Virtual)";
      } else if(opt == "Square (Maximum Morphed)") {
        settings.stick.outputStyleChoice = "Square (Maximum Morphed)";
      } else if(opt == "Custom Octagon (Virtual)") {
        settings.stick.outputStyleChoice = "Custom Octagon (Virtual)";
        customMaxOutputSlider.setEnabled(true);
      } else if(opt == "Custom Circle") {
        settings.stick.outputStyleChoice = "Custom Circle";
        customMaxOutputSlider.setEnabled(true);
      } else if(opt == "Custom Octagon (Morphed)") {
        settings.stick.outputStyleChoice = "Custom Octagon (Morphed)";
        customMaxOutputSlider.setEnabled(true);
      }
    }
  }
  outputStyleOption.onChange([&] {
    auto idx = outputStyleOption.selected();
    auto value = idx.text();

    if(value != settings.stick.outputStyleString) {
      Program::Guard guard;
      settings.stick.outputStyleString = value;

      if(value == "Octagon (Virtual) (Default)") {
        settings.stick.outputStyleChoice = "Octagon (Virtual) (Default)";
      } else if(value == "Circle (Diagonal)") {
        settings.stick.outputStyleChoice = "Circle (Diagonal)";
      } else if(value == "Circle (Cardinal)") {
        settings.stick.outputStyleChoice = "Circle (Cardinal)";
      } else if(value == "Circle (Maximum)") {
        settings.stick.outputStyleChoice = "Circle (Maximum)";
      } else if(value == "Octagon (Morphed)") {
        settings.stick.outputStyleChoice = "Octagon (Morphed)";
      } else if(value == "Square (Maximum Virtual)") {
        settings.stick.outputStyleChoice = "Square (Maximum Virtual)";
      } else if(value == "Square (Maximum Morphed)") {
        settings.stick.outputStyleChoice = "Square (Maximum Morphed)";
      } else if(value == "Custom Octagon (Virtual)") {
        settings.stick.outputStyleChoice = "Custom Octagon (Virtual)";
      } else if(value == "Custom Circle") {
        settings.stick.outputStyleChoice = "Custom Circle";
      } else if(value == "Custom Octagon (Morphed)") {
        settings.stick.outputStyleChoice = "Custom Octagon (Morphed)";
      }
    }
    if(emulator) emulator->setOutputStyle(settings.stick.outputStyleString);
    updateSlider();
  });
  outputStyleLayout.setAlignment(1).setPadding(12_sx, 0);
  outputStyleLabel.setText("Output Style:").setToolTip(
    "The output style is either a circle, a shape cut out of a circle,\n"
    "or a circle morphed into another shape.\n\n"
    "Virtual, Diagonal, and Morphed options apply a sizing formula\n"
    "to maintain the physical diagonal limit of the system's stick."
  );
  outputStyleHint.setText("Applies a gate shape to the control stick according to the style selected").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  maxOutputReducerOneFactorLabel.setText("Max Output Reducer 1 Factor:").setToolTip(
    "Reduces maximum output by the set percentage. \n\n"
    "If the sum of both maximum output reducers is greater than 100%,\n"
    "the maximum output will be reduced by 100% when both are used."
  );
  maxOutputReducerOneFactorValue.setAlignment(0.5).setToolTip(maxOutputReducerOneFactorLabel.toolTip());
  maxOutputReducerOneFactorSlider.setLength(101).setPosition(settings.stick.maxOutputReducerOneFactor * 100.0).onChange([&] {
    Program::Guard guard;
    settings.stick.maxOutputReducerOneFactor = maxOutputReducerOneFactorSlider.position() / 100.0;
    if(emulator) emulator->setMaxOutputReducerOneFactor(settings.stick.maxOutputReducerOneFactor);
    maxOutputReducerOneFactorValue.setText({maxOutputReducerOneFactorSlider.position(), "%"});
  }).doChange();

  maxOutputReducerTwoFactorLabel.setText("Max Output Reducer 2 Factor:").setToolTip(
    "Reduces maximum output by the set percentage. \n\n"
    "If the sum of both maximum output reducers is greater than 100%,\n"
    "the maximum output will be reduced by 100% when both are used."
  );
  maxOutputReducerTwoFactorValue.setAlignment(0.5).setToolTip(maxOutputReducerTwoFactorLabel.toolTip());
  maxOutputReducerTwoFactorSlider.setLength(101).setPosition(settings.stick.maxOutputReducerTwoFactor * 100.0).onChange([&] {
    Program::Guard guard;
    settings.stick.maxOutputReducerTwoFactor = maxOutputReducerTwoFactorSlider.position() / 100.0;
    if(emulator) emulator->setMaxOutputReducerTwoFactor(settings.stick.maxOutputReducerTwoFactor);
    maxOutputReducerTwoFactorValue.setText({maxOutputReducerTwoFactorSlider.position(), "%"});
  }).doChange();

  customMaxOutputLabel.setText("Custom Max Output:").setToolTip(
    "Set the value that equates to maximum output.\n"
    "Percentage refers to the physical limit of the system's native stick assembly.\n"
    "A value of 127.5 represents the true limit of the system.\n\n"
    "Only applies to custom output styles."
  );
  customMaxOutputValue.setAlignment(0.5).setToolTip(customMaxOutputLabel.toolTip());
  customMaxOutputSlider.setLength(256).setPosition(settings.stick.customMaxOutput * 2.0).onChange([&] {
    Program::Guard guard;
    settings.stick.customMaxOutput = customMaxOutputSlider.position() * 0.5;
    if(emulator) emulator->setCustomMaxOutput(settings.stick.customMaxOutput);
    customMaxOutputValue.setText({customMaxOutputSlider.position() * 0.5, " (", (customMaxOutputSlider.position() * 0.5) / 85.0 * 100.0, "%)"}); //Important to include percentage like this? Reflects physical limitation by gate of the controller but not electrical limitation
    updateSlider();
  }).doChange();
  customMaxOutputSpacer.setColor({192, 192, 192});

  deadzoneShapeLayout.setPadding(12_sx, 6_sy);
  deadzoneShapeLabel.setText("Deadzone Shape:").setToolTip(
    "Set the shape of the deadzone to either Axial or Radial.\n\n"
    "Axial is representative of the physical deadzones present in early analog stick\n"
    "assemblies and inherently leads to cardinal snapping.\n\n"
    "Radial creates a uniform deadzone in all directions and lacks cardinal snapping.\n\n"
    "Mixing deadzone shapes may lead to unexpected results and is not recommended. Please\n"
    "either match or turn off deadzones in other software to maintain the chosen shape."
  );
  deadzoneShapeAxial.setText("Axial").onActivate([&] {
    Program::Guard guard;
    settings.stick.deadzoneShape = "Axial";
    if(emulator) emulator->setDeadzoneShape(settings.stick.deadzoneShape);
  });
  deadzoneShapeRadial.setText("Radial").onActivate([&] {
    Program::Guard guard;
    settings.stick.deadzoneShape = "Radial";
    if(emulator) emulator->setDeadzoneShape(settings.stick.deadzoneShape);
  });
  if(settings.stick.deadzoneShape == "Axial") deadzoneShapeAxial.setChecked();
  if(settings.stick.deadzoneShape == "Radial") deadzoneShapeRadial.setChecked();

  nintendo64ControlStickSettingsTwoLayout.setSize({3, 1}).setPadding(12_sx, 0);
  nintendo64ControlStickSettingsTwoLayout.column(0).setAlignment(1.0);

  deadzoneSizeLabel.setText("Deadzone Size:");
  deadzoneSizeValue.setAlignment(0.5);
  deadzoneSizeSlider.setLength(128).setPosition(settings.stick.deadzoneSize).onChange([&] {
    Program::Guard guard;
    settings.stick.deadzoneSize = deadzoneSizeSlider.position();
    if(emulator) emulator->setDeadzoneSize(settings.stick.deadzoneSize);
    deadzoneSizeValue.setText(deadzoneSizeSlider.position());
    updateSlider();
  }).doChange();
  deadzoneSpacer.setColor({192, 192, 192});

  nintendo64ControlStickSettingsThreeLayout.setSize({3, 1}).setPadding(12_sx, 3_sy);
  nintendo64ControlStickSettingsThreeLayout.column(0).setAlignment(1.0);

  proportionalSensitivityLabel.setText("Sensitivity:");
  proportionalSensitivityValue.setAlignment(0.5);
  proportionalSensitivitySlider.setLength(201).setPosition(settings.stick.proportionalSensitivity * 100.0).onChange([&] {
    Program::Guard guard;
    settings.stick.proportionalSensitivity = proportionalSensitivitySlider.position() / 100.0;
    if(emulator) emulator->setProportionalSensitivity(settings.stick.proportionalSensitivity);
    proportionalSensitivityValue.setText({proportionalSensitivitySlider.position(), "%"});
    updateSlider();
  }).doChange();

  for(auto& optR : array<string[7]>{"Linear (Default)", "Relaxed to Aggressive", "Relaxed to Linear", "Linear to Relaxed", "Aggressive to Relaxed", "Aggressive to Linear", "Linear to Aggressive"}) {
    ComboButtonItem item{&responseCurveOption};
    item.setText(optR);
    if(optR == settings.stick.responseCurveString) {
      item.setSelected();

      if(optR == "Linear (Default)") {
        settings.stick.responseCurveChoice = "Linear (Default)";
      } else if(optR == "Relaxed to Aggressive") {
        settings.stick.responseCurveChoice = "Relaxed to Aggressive";
      } else if(optR == "Relaxed to Linear") {
        settings.stick.responseCurveChoice = "Relaxed to Linear";
      } else if(optR == "Linear to Relaxed") {
        settings.stick.responseCurveChoice = "Linear to Relaxed";
      } else if(optR == "Aggressive to Relaxed") {
        settings.stick.responseCurveChoice = "Aggressive to Relaxed";
      } else if(optR == "Aggressive to Linear") {
        settings.stick.responseCurveChoice = "Aggressive to Linear";
      } else if(optR == "Linear to Aggressive") {
        settings.stick.responseCurveChoice = "Linear to Aggressive";
      }
    }
  }
  responseCurveOption.onChange([&] {
    auto idxR = responseCurveOption.selected();
    auto valueR = idxR.text();
    if(valueR != settings.stick.responseCurveString) {
      Program::Guard guard;
      settings.stick.responseCurveString = valueR;

      if(valueR == "Linear (Default)") {
        settings.stick.responseCurveChoice = "Linear (Default)";
      } else if(valueR == "Relaxed to Aggressive") {
        settings.stick.responseCurveChoice = "Relaxed to Aggressive";
      } else if(valueR == "Relaxed to Linear") {
        settings.stick.responseCurveChoice = "Relaxed to Linear";
      } else if(valueR == "Linear to Relaxed") {
        settings.stick.responseCurveChoice = "Linear to Relaxed";
      } else if(valueR == "Aggressive to Relaxed") {
        settings.stick.responseCurveChoice = "Aggressive to Relaxed";
      } else if(valueR == "Aggressive to Linear") {
        settings.stick.responseCurveChoice = "Aggressive to Linear";
      } else if(valueR == "Linear to Aggressive") {
        settings.stick.responseCurveChoice = "Linear to Aggressive";
      }
    }
    if(emulator) emulator->setResponseCurve(settings.stick.responseCurveString);
    updateSlider();
  });
  responseCurveLayout.setAlignment(1).setPadding(12_sx, 0_sy);
  responseCurveLabel.setText("Response Curve:");
  responseCurveHint.setText("Changes control stick behavior according to the response curve selected").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  nintendo64ControlStickSettingsFourLayout.setSize({3, 2}).setPadding(12_sx, 6_sy);
  nintendo64ControlStickSettingsFourLayout.column(0).setAlignment(1.0);

  rangeNormalizedInflectionPointLabel.setText("Inflection Point:").setToolTip(
    "Set the relative distance with respect to the range of\n"
    "the real deadzone to the stick edge where the curve changes behavior."
  );
  rangeNormalizedInflectionPointValue.setAlignment(0.5).setToolTip(rangeNormalizedInflectionPointLabel.toolTip());
  rangeNormalizedInflectionPointSlider.setLength(199).setPosition((settings.stick.rangeNormalizedInflectionPoint - 0.005) / 0.5 * 100.0).onChange([&] {
    Program::Guard guard;
    settings.stick.rangeNormalizedInflectionPoint = (rangeNormalizedInflectionPointSlider.position() * 0.5 + 0.5) / 100.0;
    if(emulator) emulator->setRangeNormalizedInflectionPoint(settings.stick.rangeNormalizedInflectionPoint);
    rangeNormalizedInflectionPointValue.setText({0.5 + 0.5 * rangeNormalizedInflectionPointSlider.position(), "%"});
    updateSlider();
  }).doChange();

  responseStrengthLabel.setText("Response Strength:").setToolTip(
    "Set the response strength.\n"
    "Changes are subtle when low and pronounced when high."
  );
  responseStrengthValue.setAlignment(0.5).setToolTip(responseStrengthLabel.toolTip());
  responseStrengthSlider.setLength(101).setPosition(settings.stick.responseStrength * 100.0).onChange([&] {
    Program::Guard guard;
    settings.stick.responseStrength = responseStrengthSlider.position() / 100.0;
    if(emulator) emulator->setResponseStrength(settings.stick.responseStrength);
    responseStrengthValue.setText({responseStrengthSlider.position(), "%"});
    updateSlider();
  }).doChange();
  responseSpacer.setColor({192, 192, 192});

  virtualNotchOption.setText("Virtual Notches").setChecked(settings.stick.virtualNotch).onToggle([&] {
    Program::Guard guard;
    settings.stick.virtualNotch = virtualNotchOption.checked();
    if(emulator) emulator->setVirtualNotch(settings.stick.virtualNotch);
  });
  virtualNotchLayout.setAlignment(1).setPadding(12_sx, 6_sy);
  virtualNotchHint.setText("Enable/Disable virtual notches").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  nintendo64ControlStickSettingsFiveLayout.setSize({3, 2}).setPadding(12_sx, 3_sy);
  nintendo64ControlStickSettingsFiveLayout.column(0).setAlignment(1.0);

  notchLengthFromEdgeLabel.setText("Notch Length from Edge:").setToolTip(
    "Set the notch length from the circular edge of the analog stick.\n"
    "A higher value results in a larger notch that can be reached sooner."
  );
  notchLengthFromEdgeValue.setAlignment(0.5).setToolTip(notchLengthFromEdgeLabel.toolTip());
  notchLengthFromEdgeSlider.setLength(21).setPosition(settings.stick.notchLengthFromEdge * 100.0).onChange([&] {
    Program::Guard guard;
    settings.stick.notchLengthFromEdge = notchLengthFromEdgeSlider.position() / 100.0;
    if(emulator) emulator->setNotchLengthFromEdge(settings.stick.notchLengthFromEdge);
    notchLengthFromEdgeValue.setText({notchLengthFromEdgeSlider.position(), "%"});
  }).doChange();

  notchAngularSnappingDistanceLabel.setText("Angular Snapping Distance:").setToolTip(
    "Set the angular distance at which input will snap to the nearest notch."
  );
  notchAngularSnappingDistanceValue.setAlignment(0.5).setToolTip(notchAngularSnappingDistanceLabel.toolTip());
  notchAngularSnappingDistanceSlider.setLength(91).setPosition(settings.stick.notchAngularSnappingDistance * 2.0).onChange([&] {
    Program::Guard guard;
    settings.stick.notchAngularSnappingDistance = notchAngularSnappingDistanceSlider.position() * 0.5;
    if(emulator) emulator->setNotchAngularSnappingDistance(settings.stick.notchAngularSnappingDistance);
    notchAngularSnappingDistanceValue.setText({notchAngularSnappingDistanceSlider.position() * 0.5, "\u00b0"}); //unicode escape code for degree symbol
  }).doChange();

  advisoryHint.setText("Note: Settings currently only apply to Controller Port 1 with the system's default used for the remaining ports.\nCustomization must be reapplied after connecting to a peripheral with an analog stick in the system's menu.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
}

auto StickSettings::updateSlider() -> void {
  if(settings.stick.outputStyleChoice == "Custom Octagon (Virtual)" || settings.stick.outputStyleChoice == "Custom Circle" || settings.stick.outputStyleChoice == "Custom Octagon (Morphed)") {
    customMaxOutputSlider.setEnabled(true);
    settings.stick.customMaxOutput = customMaxOutputSlider.position() * 0.5;
  } else {
    customMaxOutputSlider.setEnabled(false);
    settings.stick.customMaxOutput = 85.0;
  }
  Program::Guard guard;
  if(emulator) emulator->setCustomMaxOutput(settings.stick.customMaxOutput);

  auto maxOutputMultiplier = settings.stick.customMaxOutput / 85.0;
  auto diagonalMax = 0.0;
  auto cardinalMax = 0.0;
  auto configuredInnerDeadzone = settings.stick.deadzoneSize;
  auto saturationRadius = 0.0;

  if(settings.stick.outputStyleChoice == "Custom Octagon (Morphed)") {
    diagonalMax = maxOutputMultiplier * 69.0;
    cardinalMax = maxOutputMultiplier * 85.0;
    saturationRadius = maxOutputMultiplier * 85.0;
  } else if(settings.stick.outputStyleChoice == "Custom Octagon (Virtual)") {
    diagonalMax = maxOutputMultiplier * 69.0;
    cardinalMax = maxOutputMultiplier * 85.0;
    saturationRadius = sqrt(2.0) * (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
  } else if(settings.stick.outputStyleChoice == "Custom Circle") {
    diagonalMax = maxOutputMultiplier * 85.0 * sqrt(2.0) / 2.0;
    cardinalMax = maxOutputMultiplier * 85.0;
    saturationRadius = maxOutputMultiplier * 85.0;
  } else if(settings.stick.outputStyleChoice == "Octagon (Virtual) (Default)") {
    diagonalMax = 69.0;
    cardinalMax = 85.0;
    saturationRadius = sqrt(2.0) * (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
  } else if(settings.stick.outputStyleChoice == "Circle (Diagonal)") {
    diagonalMax = 69.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    cardinalMax = saturationRadius;
  } else if(settings.stick.outputStyleChoice == "Circle (Maximum)") {
    diagonalMax = 127.0 * sqrt(2.0) / 2.0;
    cardinalMax = 127.0;
    saturationRadius = 127.0;
  } else if(settings.stick.outputStyleChoice == "Circle (Cardinal)") {
    diagonalMax = 85.0 * sqrt(2.0) / 2.0;
    cardinalMax = 85.0;
    saturationRadius = 85.0;
  } else if(settings.stick.outputStyleChoice == "Octagon (Morphed)") {
    diagonalMax = 69.0;
    cardinalMax = 85.0;
    saturationRadius = 85.0;
  } else if(settings.stick.outputStyleChoice == "Square (Maximum Virtual)") {
    diagonalMax = 127.0;
    cardinalMax = 127.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
  } else if(settings.stick.outputStyleChoice == "Square (Maximum Morphed)") {
    diagonalMax = 127.0;
    cardinalMax = 127.0;
    saturationRadius = 127.0;
  }

  if(settings.stick.outputStyleChoice == "Octagon (Virtual) (Default)" || settings.stick.outputStyleChoice == "Custom Octagon (Virtual)" || settings.stick.outputStyleChoice == "Square (Maximum Virtual)") {
    deadzoneSizeHint.setText({"Virtual: ", settings.stick.deadzoneSize / cardinalMax * 100.0, "%     ", "Real: ",  settings.stick.deadzoneSize / saturationRadius * 100.0, "%"}).setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  } else {
    deadzoneSizeHint.setText({"Virtual: N/A     ", "Real: ", settings.stick.deadzoneSize / saturationRadius * 100.0, "%"}).setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  }
  auto inflectionPoint = settings.stick.rangeNormalizedInflectionPoint * (saturationRadius - configuredInnerDeadzone) + configuredInnerDeadzone;
  auto b = 1.0;
  auto c = b * (log(1.0 - cos(Math::Pi * (inflectionPoint - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone))) - log(2.0)) / log((inflectionPoint - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone));


  auto configuredProportionalSensitivity = settings.stick.proportionalSensitivity;

  auto maxA = 0.0;
  auto configuredResponseStrength = settings.stick.responseStrength;
  auto a = 0.0;

  if(settings.stick.responseCurveChoice == "Aggressive to Relaxed" || settings.stick.responseCurveChoice == "Aggressive to Linear" || settings.stick.responseCurveChoice == "Linear to Aggressive") {
    maxA = Math::Pi * 1e-09 / (Math::Pi * 1e-09 - c * (saturationRadius - configuredInnerDeadzone) * tan(Math::Pi * 1e-09 / (2 * (saturationRadius - configuredInnerDeadzone))));
    a = configuredResponseStrength * (maxA - 1.0) + 1.0;
  } else {
    maxA = 1.0;
    a = (1.0 - configuredResponseStrength) * maxA;
  }

  auto diagonalInput = saturationRadius * sqrt(2.0) / 2.0;
  auto diagonalOutput = 0.0;

  if(settings.stick.responseCurveChoice == "Linear (Default)") {
    diagonalOutput = configuredProportionalSensitivity * (diagonalInput - configuredInnerDeadzone) * saturationRadius / (saturationRadius - configuredInnerDeadzone);
  } else if(settings.stick.responseCurveChoice == "Relaxed to Aggressive" || settings.stick.responseCurveChoice == "Aggressive to Relaxed") {
    diagonalOutput = configuredProportionalSensitivity * pow(((diagonalInput - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)), (a / b)) * saturationRadius * pow((sin(((diagonalInput - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)) * Math::Pi / 2.0)), (2.0 * (b - a) / c));
  } else if(settings.stick.responseCurveChoice == "Relaxed to Linear" || settings.stick.responseCurveChoice == "Aggressive to Linear") {
    if(diagonalInput <= inflectionPoint) {
      diagonalOutput = configuredProportionalSensitivity * pow(((diagonalInput - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)), (a / b)) * saturationRadius * pow((sin(((diagonalInput - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)) * Math::Pi / 2.0)), (2.0 * (b - a) / c));
    } else {
      diagonalOutput = configuredProportionalSensitivity * (diagonalInput - configuredInnerDeadzone) * saturationRadius / (saturationRadius - configuredInnerDeadzone);
    }
  } else if(settings.stick.responseCurveChoice == "Linear to Relaxed" || settings.stick.responseCurveChoice == "Linear to Aggressive") {
    if(diagonalInput <= inflectionPoint) {
      diagonalOutput = configuredProportionalSensitivity * (diagonalInput - configuredInnerDeadzone) * saturationRadius / (saturationRadius - configuredInnerDeadzone);
    } else {
      diagonalOutput = configuredProportionalSensitivity * pow(((diagonalInput - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)), (a / b)) * saturationRadius * pow((sin(((diagonalInput - configuredInnerDeadzone) / (saturationRadius - configuredInnerDeadzone)) * Math::Pi / 2.0)), (2.0 * (b - a) / c));
    }
  }

  diagonalOutput += 1e-09; //add epsilon to counteract floating point precision error
  if((diagonalOutput < diagonalMax) && !(settings.stick.outputStyleChoice == "Octagon (Morphed)" || settings.stick.outputStyleChoice == "Square (Maximum Morphed)" || settings.stick.outputStyleChoice == "Custom Octagon (Morphed)")) {
    if(settings.stick.responseCurveChoice == "Relaxed to Aggressive" || settings.stick.responseCurveChoice == "Relaxed to Linear") {
      responseStrengthHint.setText("Note: Reduced diagonal output.\nLower the inflection point, lessen the response strength, raise sensitivity, decrease deadzone, and/or increase max output for full diagonal output.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
    } else if(settings.stick.responseCurveChoice == "Aggressive to Relaxed" || settings.stick.responseCurveChoice == "Linear to Aggressive") {
      responseStrengthHint.setText("Note: Reduced diagonal output.\nRaise the inflection point, lessen the response strength, raise sensitivity, decrease deadzone, and/or increase max output for full diagonal output.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
    } else if(settings.stick.responseCurveChoice == "Linear (Default)" || settings.stick.responseCurveChoice == "Linear to Relaxed" || settings.stick.responseCurveChoice == "Aggressive to Linear") {
      responseStrengthHint.setText("Note: Reduced diagonal output.\nRaise sensitivity, decrease deadzone, and/or increase max output for full diagonal output.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
    }
  } else {
    responseStrengthHint.setText("Note: Full diagonal output.\n").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);;
  }
}
