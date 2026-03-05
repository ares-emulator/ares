#pragma once

#include <ares/ares.hpp>

namespace nogui {

struct CoreOptions {
  bool videoPixelAccuracy = false;
  nall::string videoQuality = "SD";
  bool videoSupersampling = false;
  bool videoDisableVideoInterfaceProcessing = false;
  bool videoWeaveDeinterlacing = true;

  bool generalHomebrewMode = false;
  bool generalForceInterpreter = false;

  bool nintendo64ExpansionPak = true;
  nall::string nintendo64ControllerPakBankString = "32KiB (Default)";

  bool gameBoyAdvancePlayer = false;
  bool megadriveTMSS = false;
};

auto normalizeRegion(const nall::string& region) -> nall::string;
auto defaultSystemNameForMedium(const nall::string& medium) -> nall::string;
auto defaultProfileForMedium(
  const nall::string& medium,
  const nall::string& region,
  bool gameBoyAdvancePlayer = false
) -> nall::string;
auto configureCoreOptionsForMedium(const nall::string& medium, const CoreOptions& options) -> void;
auto loadCoreForMedium(const nall::string& medium, ares::Node::System& root, const nall::string& profile) -> bool;
auto connectDefaultPorts(ares::Node::System& root) -> void;

}
