#include "../desktop-ui.hpp"

// CMake selects this implementation when RetroAchievements support is disabled.
RetroAchievements retroAchievements;

RetroAchievements::~RetroAchievements() = default;

auto RetroAchievements::initialize() -> void {}
auto RetroAchievements::login(const string&, const string&) -> bool { return false; }
auto RetroAchievements::logout() -> void {}
auto RetroAchievements::pumpUi() -> void {}
auto RetroAchievements::requestAvatar(const string&) -> void {}
auto RetroAchievements::hasUser() const -> bool { return false; }
auto RetroAchievements::loginPending() const -> bool { return false; }
auto RetroAchievements::username() const -> string { return {}; }
auto RetroAchievements::displayName() const -> string { return {}; }
auto RetroAchievements::userScore() const -> u32 { return 0; }
auto RetroAchievements::userScoreSoftcore() const -> u32 { return 0; }
auto RetroAchievements::avatarUrl() const -> string { return {}; }
auto RetroAchievements::readMemory(u32, u8*, u32) const -> u32 { return 0; }
auto RetroAchievements::handleClientLoginResult(int, const char*, u64) -> void {}
auto RetroAchievements::handleClientGameLoadResult(int, const char*, u64) -> void {}
auto RetroAchievements::frame() -> void {}
auto RetroAchievements::idle() -> void {}
auto RetroAchievements::gameLoaded() -> void {}
auto RetroAchievements::gameUnloaded() -> void {}
auto RetroAchievements::gameHash() const -> string { return {}; }
auto RetroAchievements::hardcore() const -> bool { return false; }
auto RetroAchievements::supportsGame() const -> bool { return false; }
auto RetroAchievements::enableHardcore(bool) -> bool { return false; }
auto RetroAchievements::setEnabled(bool) -> void {}
auto RetroAchievements::reset() -> void {}
auto RetroAchievements::resetGame() -> void {}
