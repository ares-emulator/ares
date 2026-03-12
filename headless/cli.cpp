#include "cli.hpp"
#include "parse.hpp"
#include <ares/ares.hpp>
#include <nall/inode.hpp>

using namespace nall;

namespace {

auto parseBenchmarkSpec(const string& value, double& durationSeconds, u64& frameTarget) -> bool {
  durationSeconds = 0.0;
  frameTarget = 0;
  if(value.length() == 0) return false;

  auto suffix = value[value.size() - 1];
  if(suffix == 's' || suffix == 'S' || suffix == 'f' || suffix == 'F') {
    string prefix;
    for(u32 i : range((u32)value.size() - 1)) prefix.append(value[i]);
    u64 parsed = 0;
    if(!headless::parsePositiveInteger(prefix, parsed)) return false;
    if(suffix == 'f' || suffix == 'F') frameTarget = parsed;
    else durationSeconds = (double)parsed;
    return true;
  }

  durationSeconds = value.real();
  return durationSeconds > 0.0;
}

auto parseVerbosity(const string& value, u32& out) -> bool {
  if(value == "0" || value == "quiet") {
    out = 0;
    return true;
  }
  if(value == "1" || value == "normal") {
    out = 1;
    return true;
  }
  if(value == "2" || value == "verbose") {
    out = 2;
    return true;
  }
  return false;
}

auto parseStateSlot(const string& value, u32& out) -> bool {
  if(!value) {
    out = 1;
    return true;
  }

  u64 parsed = 0;
  if(!headless::parsePositiveInteger(value, parsed) || parsed < 1 || parsed > 9) return false;
  out = (u32)parsed;
  return true;
}

}

namespace headless {

auto parseCliOptions(Arguments& arguments, CliOptions& options, string& error) -> bool {
  while(arguments.find("--system")) arguments.take("--system", options.systemOverride);
  if(!parseLaunchSettings(arguments, options.launchSettings, error)) return false;

  if(string benchmarkSpec; arguments.take("--benchmark", benchmarkSpec)) {
    if(!parseBenchmarkSpec(benchmarkSpec, options.benchmarkDuration, options.benchmarkFrameTarget)) {
      error = "--benchmark expects a positive value (eg: 5, 5s, 300f).";
      return false;
    }
  }

  if(arguments.find("--run-frames")) {
    string frames;
    if(!arguments.take("--run-frames", frames) || !headless::parsePositiveInteger(frames, options.runFramesTarget)) {
      error = "--run-frames requires a positive integer frame count.";
      return false;
    }
  }

  {
    string loadState;
    while(arguments.takeOptional("--load-state", loadState)) {
      if(!parseStateSlot(loadState, options.stateSlot)) {
        error = "--load-state accepts optional slot 1-9.";
        return false;
      }
    }
  }

  if(arguments.take("--quiet")) options.verbosity = 0;
  if(arguments.take("--verbose")) options.verbosity = 2;

  if(arguments.find("--verbosity")) {
    string verbosityMode;
    if(!arguments.take("--verbosity", verbosityMode) || !parseVerbosity(verbosityMode, options.verbosity)) {
      error = "--verbosity expects one of: quiet|normal|verbose (or 0|1|2).";
      return false;
    }
  }

  {
    string saveOnExitValue;
    while(arguments.takeOptional("--save-on-exit", saveOnExitValue)) {
      if(!parseStateSlot(saveOnExitValue, options.saveOnExitSlot)) {
        error = "--save-on-exit accepts optional slot 1-9.";
        return false;
      }
      options.saveOnExit = true;
    }
  }

  if(arguments.find("--save-last-frame") && !arguments.take("--save-last-frame", options.saveLastFramePath)) {
    error = "--save-last-frame requires an output .png path.";
    return false;
  }
  options.videoChecksum = arguments.take("--video-checksum");

  options.showHelp = arguments.take("--help");
  options.showVersion = arguments.take("--version");
  if(options.showHelp || options.showVersion) return true;

  for(auto arg : arguments) {
    if(!arg.beginsWith("--")) {
      options.gamePath = arg;
      break;
    }
  }

  if(!options.gamePath || !inode::exists(options.gamePath)) {
    error = "provide a valid game file or directory.";
    return false;
  }

  return true;
}

auto printHeadlessUsage() -> void {
  print("\n Usage: ares-headless [OPTIONS]... game\n\n");
  print("Options:\n");
  print("  --help                Displays available options and exit\n");
  print("  --version             Displays the version string of the application\n");
  print("  --system name         Override auto-detected system\n");
  print("  --setting name=value  Override supported core settings\n");
  print("  --benchmark value     Run benchmark for N seconds (eg 5s / 5) or N frames (eg 300f)\n");
  print("  --run-frames frames   Run for exactly N frames and then exit\n");
  print("  --quiet               Suppress non-essential runtime logging\n");
  print("  --verbose             Alias for --verbosity verbose\n");
  print("  --verbosity level     quiet|normal|verbose (or 0|1|2)\n");
  print("  --video-checksum      Print the final frame buffer checksum on exit (XXH64 hash)\n");
  print("  --load-state slot     Load save state slot (default: 1)\n");
  print("  --save-on-exit slot   Save state on exit (default: 1)\n");
  print("  --save-last-frame p   Save final rendered frame to PNG path\n");
  print("  --settings-file path  (only reads savestates location for now)\n");
}

}
