#pragma once

#include <ares/ares.hpp>

namespace nogui {

auto normalizeRegion(const nall::string& region) -> nall::string;
auto defaultSystemNameForMedium(const nall::string& medium) -> nall::string;
auto defaultProfileForMedium(const nall::string& medium, const nall::string& region) -> nall::string;
auto loadCoreForMedium(const nall::string& medium, ares::Node::System& root, const nall::string& profile) -> bool;
auto connectDefaultPorts(ares::Node::System& root) -> void;

}
