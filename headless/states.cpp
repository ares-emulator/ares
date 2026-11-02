#include "states.hpp"
#include <nall/directory.hpp>
#include <nall/file.hpp>
#include <nall/location.hpp>

using namespace nall;

namespace headless {

auto locateStatePath(const string& location, const string& suffix, const string& path, const string& system) -> string {
  if(!path) return {Location::notsuffix(location), suffix};

  string pathname = {path, system, "/"};
  directory::create(pathname);
  return {pathname, Location::prefix(location), suffix};
}

auto loadState(ares::Node::System& root, const string& gameLocation, u32 slot, const string& savesPath) -> bool {
  auto location = locateStatePath(gameLocation, {".bs", slot}, savesPath, root->name());
  auto memory = file::read(location);
  if(memory.empty()) return false;

  serializer state{memory.data(), (u32)memory.size()};
  return root->unserialize(state);
}

auto saveState(ares::Node::System& root, const string& gameLocation, u32 slot, const string& savesPath) -> bool {
  auto location = locateStatePath(gameLocation, {".bs", slot}, savesPath, root->name());
  if(auto state = root->serialize()) {
    return file::write(location, {state.data(), state.size()});
  }
  return false;
}

}
