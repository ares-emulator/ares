#pragma once

#include <ares/ares.hpp>
#include <vector>

namespace headless {

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

struct FirmwareQuery {
  nall::string systemName;
  nall::string type;
  nall::string region;
};

auto normalizeRegion(const nall::string& region) -> nall::string;
auto resolveSystemAlias(const nall::string& name) -> nall::string;
auto printSystemAliases() -> void;
auto defaultSystemNameForMedium(const nall::string& medium) -> nall::string;
auto firmwareSettingPath(
  const nall::string& systemName,
  const nall::string& type,
  const nall::string& region
) -> nall::string;
auto firmwareQueriesForMedium(
  const nall::string& medium,
  const nall::string& region
) -> std::vector<FirmwareQuery>;
auto firmwareIsOptionalForMedium(const nall::string& medium) -> bool;
auto defaultProfileForMedium(
  const nall::string& medium,
  const nall::string& region,
  bool gameBoyAdvancePlayer = false
) -> nall::string;
auto configureCoreOptionsForMedium(const nall::string& medium, const CoreOptions& options) -> void;
auto loadCoreForMedium(const nall::string& medium, ares::Node::System& root, const nall::string& profile) -> bool;
auto connectDefaultPorts(ares::Node::System& root) -> void;

}
