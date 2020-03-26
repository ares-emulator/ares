auto Emulator::inputUpdate() -> void {
  if(inputInstance && inputInstance.driver() != settings.input.driver) {
    inputInstance.reset();
  }

  if(!inputInstance) {
    inputInstance.create(settings.input.driver);
    inputInstance.setContext(program.handle());
    inputInstance.onChange({&InputManager::eventInput, &inputManager});
    if(!inputInstance.ready()) {
      inputInstance.create(settings.input.driver = "None");
    }
  }
}
