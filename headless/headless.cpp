#include <ares/ares.hpp>
#include "cli.hpp"
#include "emulators.hpp"
#include "runtime.hpp"
#include "states.hpp"
#include "video-output.hpp"
#include <nall/gdb/server.hpp>
#include <nall/main.hpp>

using namespace nall;

auto nall::main(Arguments arguments) -> void {
  ares::Memory::FixedAllocator::get();

  headless::CliOptions cli;
  string parseError;
  if(!headless::parseCliOptions(arguments, cli, parseError)) {
    fprintf(stderr, "error: %s\n", parseError.data());
    return;
  }

  if(cli.showHelp) {
    headless::printHeadlessUsage();
    return;
  }

  if(cli.showVersion) {
    print(ares::Version, "\n");
    return;
  }

  mia::setHomeLocation([]() -> string { return {Path::userData(), "ares/Systems/"}; });
  mia::setSaveLocation([&cli]() -> string { return cli.launchSettings.savesPath; });

  headless::Runtime runtime;
  runtime.runFramesTarget = cli.runFramesTarget;
  runtime.stateSlot = cli.stateSlot;
  runtime.gdbEnabled = cli.launchSettings.gdbEnabled;
  runtime.gdbPort = cli.launchSettings.gdbPort;
  runtime.gdbUseIPv4 = cli.launchSettings.gdbUseIPv4;
  runtime.awaitGdbClient = cli.launchSettings.awaitGdbClient;
  runtime.verbosity = cli.verbosity;
  runtime.saveLastFramePath = cli.saveLastFramePath;
  runtime.videoChecksum = cli.videoChecksum;
  runtime.benchmarkDuration = cli.benchmarkDuration;
  runtime.benchmarkFrameTarget = cli.benchmarkFrameTarget;

  runtime.medium = cli.systemOverride ? cli.systemOverride : mia::identify(cli.gamePath);
  if(!runtime.medium) {
    fprintf(stderr, "error: unable to determine game type for: %s\n", Location::file(cli.gamePath).data());
    return;
  }

  runtime.gamePak = mia::Medium::create(runtime.medium);
  if(!runtime.gamePak) {
    fprintf(stderr, "error: unsupported system: %s\n", runtime.medium.data());
    return;
  }
  auto gameLoad = runtime.gamePak->load(cli.gamePath);
  if(gameLoad != successful) {
    fprintf(stderr, "error: failed to load game media.\n");
    return;
  }

  auto systemName = headless::defaultSystemNameForMedium(runtime.medium);

  runtime.systemPak = mia::System::create(systemName);
  if(!runtime.systemPak || runtime.systemPak->load() != successful) {
    fprintf(stderr, "error: failed to load system data for: %s\n", systemName.data());
    return;
  }

  auto region = runtime.gamePak->pak ? headless::normalizeRegion(runtime.gamePak->pak->attribute("region")) : string{};
  runtime.profile = headless::defaultProfileForMedium(runtime.medium, region, cli.launchSettings.coreOptions.gameBoyAdvancePlayer);
  if(!runtime.profile) {
    fprintf(stderr, "error: no default core profile for system: %s\n", runtime.medium.data());
    return;
  }

  ares::platform = &runtime;
  headless::configureCoreOptionsForMedium(runtime.medium, cli.launchSettings.coreOptions);
  if(!headless::loadCoreForMedium(runtime.medium, runtime.root, runtime.profile)) {
    fprintf(stderr, "error: failed to load ares core profile: %s\n", runtime.profile.data());
    return;
  }

  headless::connectDefaultPorts(runtime.root);

  if(runtime.gdbEnabled) {
    nall::GDB::server.reset();
    nall::GDB::server.open(runtime.gdbPort, runtime.gdbUseIPv4);
    nall::GDB::server.onClientConnectCallback = []() {
      fprintf(stderr, "GDB client connected\n");
    };

    if(runtime.awaitGdbClient) {
      fprintf(stderr, "Waiting for GDB client on port %u...\n", runtime.gdbPort);
      while(!runtime.shouldExit && !nall::GDB::server.hasClient()) {
        nall::GDB::server.updateLoop();
        usleep(20 * 1000);
      }
    }
  }

  if(cli.verbosity == 0) {
    freopen("/dev/null", "w", stderr);
  }

  runtime.root->power();
  if(runtime.stateSlot) {
    if(!headless::loadState(runtime.root, runtime.gamePak->location, runtime.stateSlot, cli.launchSettings.savesPath)) {
      fprintf(stderr, "warning: failed to load state from slot %u\n", runtime.stateSlot);
    } else {
      if(cli.verbosity >= 1) print("Loaded state from slot ", runtime.stateSlot, "\n");
    }
  }
  while(!runtime.shouldExit) {
    if(runtime.gdbEnabled && nall::GDB::server.isHalted()) {
      nall::GDB::server.updateLoop();
      usleep(1000);
      continue;
    }
    if(runtime.gdbEnabled) {
      nall::GDB::server.updateLoop();
    }
    runtime.root->run();
    if(runtime.gdbEnabled) {
      nall::GDB::server.updateLoop();
    }
  }

  if(cli.saveOnExit) {
    if(headless::saveState(runtime.root, runtime.gamePak->location, cli.saveOnExitSlot, cli.launchSettings.savesPath)) {
      if(cli.verbosity >= 1) print("Saved state to slot ", cli.saveOnExitSlot, "\n");
    } else {
      fprintf(stderr, "warning: failed to save state to slot %u\n", cli.saveOnExitSlot);
    }
  }

  runtime.root->save();
  if(runtime.saveLastFramePath) {
    auto result = headless::saveCapturedFramePng(
      runtime.saveLastFramePath,
      runtime.lastFrame,
      runtime.lastFramePitch,
      runtime.lastFrameWidth,
      runtime.lastFrameHeight,
      runtime.screens
    );
    if(result == headless::SaveFrameResult::NoFrameCaptured) {
      fprintf(stderr, "warning: no video frame captured, PNG not written.\n");
    } else if(result == headless::SaveFrameResult::EncodeFailed) {
      fprintf(stderr, "warning: failed to write PNG: %s\n", runtime.saveLastFramePath.data());
    }
  }
  runtime.gamePak->save(runtime.gamePak->location);
  runtime.systemPak->save(runtime.systemPak->location);
  runtime.root->unload();
  if(runtime.gdbEnabled) {
    nall::GDB::server.close();
    nall::GDB::server.reset();
  }
}
