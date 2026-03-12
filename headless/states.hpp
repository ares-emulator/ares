#pragma once

#include <ares/ares.hpp>
#include <nall/string.hpp>

namespace headless {

auto locateStatePath(const nall::string& location, const nall::string& suffix, const nall::string& path, const nall::string& system) -> nall::string;
auto loadState(ares::Node::System& root, const nall::string& gameLocation, u32 slot, const nall::string& savesPath) -> bool;
auto saveState(ares::Node::System& root, const nall::string& gameLocation, u32 slot, const nall::string& savesPath) -> bool;

}
