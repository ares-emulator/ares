target_sources(
  desktop-ui
  PRIVATE desktop-ui.hpp game-browser/game-browser.hpp input/hotkeys.cpp input/input.hpp presentation/presentation.hpp
)

target_sources(desktop-ui PRIVATE game-browser/game-browser.hpp)

target_sources(desktop-ui PRIVATE input/hotkeys.cpp input/input.hpp)

target_sources(desktop-ui PRIVATE presentation/presentation.hpp)

target_sources(
  desktop-ui
  PRIVATE
    program/drivers.cpp
    program/load.cpp
    program/platform.cpp
    program/program.hpp
    program/rewind.cpp
    program/states.cpp
    program/status.cpp
    program/utility.cpp
)

target_sources(desktop-ui PRIVATE resource/resource.cpp resource/resource.hpp)

target_sources(
  desktop-ui
  PRIVATE
    settings/audio.cpp
    settings/debug.cpp
    settings/drivers.cpp
    settings/emulators.cpp
    settings/firmware.cpp
    settings/home.cpp
    settings/hotkeys.cpp
    settings/input.cpp
    settings/options.cpp
    settings/paths.cpp
    settings/settings.hpp
    settings/video.cpp
)

target_sources(
  desktop-ui
  PRIVATE
    tools/cheats.cpp
    tools/graphics.cpp
    tools/manifest.cpp
    tools/memory.cpp
    tools/properties.cpp
    tools/streams.cpp
    tools/tools.hpp
    tools/tracer.cpp
)

target_sources(
  desktop-ui
  PRIVATE
    emulator/arcade.cpp
    emulator/atari-2600.cpp
    emulator/colecovision.cpp
    emulator/emulator.cpp
    emulator/emulator.hpp
    emulator/emulators.cpp
    emulator/famicom-disk-system.cpp
    emulator/famicom.cpp
    emulator/game-boy-advance.cpp
    emulator/game-boy-color.cpp
    emulator/game-boy.cpp
    emulator/game-gear.cpp
    emulator/master-system.cpp
    emulator/mega-32x.cpp
    emulator/mega-cd-32x.cpp
    emulator/mega-cd.cpp
    emulator/mega-drive.cpp
    emulator/msx.cpp
    emulator/msx2.cpp
    emulator/myvision.cpp
    emulator/neo-geo-aes.cpp
    emulator/neo-geo-mvs.cpp
    emulator/neo-geo-pocket-color.cpp
    emulator/neo-geo-pocket.cpp
    emulator/nintendo-64.cpp
    emulator/nintendo-64dd.cpp
    emulator/pc-engine-cd.cpp
    emulator/pc-engine.cpp
    emulator/playstation.cpp
    emulator/pocket-challenge-v2.cpp
    emulator/saturn.cpp
    emulator/sg-1000.cpp
    emulator/super-famicom.cpp
    emulator/supergrafx-cd.cpp
    emulator/supergrafx.cpp
    emulator/wonderswan-color.cpp
    emulator/wonderswan.cpp
    emulator/zx-spectrum-128.cpp
    emulator/zx-spectrum.cpp
)
