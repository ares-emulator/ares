#include "../desktop-ui.hpp"

// CMake selects this implementation when RetroAchievements support is disabled.
RetroAchievements retroAchievements;

RetroAchievements::~RetroAchievements() = default;

auto RetroAchievements::initialize() -> void {}
auto RetroAchievements::login(const string&, const string&) -> bool { return false; }
auto RetroAchievements::logout() -> void {}
auto RetroAchievements::pumpUi() -> void {}
auto RetroAchievements::requestImage(const string&) -> void {}
auto RetroAchievements::hasUser() const -> bool { return false; }
auto RetroAchievements::loginPending() const -> bool { return false; }
auto RetroAchievements::username() const -> string { return {}; }
auto RetroAchievements::displayName() const -> string { return {}; }
auto RetroAchievements::userScore() const -> u32 { return 0; }
auto RetroAchievements::userScoreSoftcore() const -> u32 { return 0; }
auto RetroAchievements::avatarUrl() const -> string { return {}; }
auto RetroAchievements::gameView() const -> RetroAchievementsGameView {
  RetroAchievementsGameView view;
  view.message = "RetroAchievements support is unavailable in this build.";
  return view;
}
auto RetroAchievements::leaderboardView() const -> RetroAchievementsLeaderboardView { return {}; }
auto RetroAchievements::fetchLeaderboard(u32, bool) -> void {}
auto RetroAchievements::handleLeaderboardResult(int, const char*, rc_client_leaderboard_entry_list_t*, u64) -> void {}
auto RetroAchievements::imageForUrl(const string&, u32) -> image { return {}; }
auto RetroAchievements::imageVersion() const -> u64 { return 0; }
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
auto RetroAchievements::enableEncore(bool) -> void {}
auto RetroAchievements::setEnabled(bool) -> void {}
auto RetroAchievements::encoreUnlock(u32) const -> bool { return false; }
auto RetroAchievements::reset() -> void {}
auto RetroAchievements::resetGame() -> void {}
