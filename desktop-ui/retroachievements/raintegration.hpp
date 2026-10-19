#pragma once

namespace RA::Integration {
#if defined(_WIN32) && defined(RC_CLIENT_SUPPORTS_RAINTEGRATION)
inline auto available() -> bool { return true; }
auto start(RetroAchievements& manager, rc_client_t* client) -> rc_client_async_handle_t*;
auto stop(rc_client_t* client, rc_client_async_handle_t* pending, bool active) -> void;
auto finish(rc_client_t* client) -> void;
auto setConsoleId(rc_client_t* client, u32 consoleId) -> void;
auto menuItems(rc_client_t* client) -> std::vector<RetroAchievements::DevelopmentMenuItem>;
auto activateMenuItem(rc_client_t* client, u32 id) -> bool;
auto hardcoreEnabled(rc_client_t* client) -> bool;
auto hasModifications(rc_client_t* client) -> bool;
auto achievementUnlockLabel(rc_client_t* client, u32 achievementId) -> const char*;
#else
inline auto available() -> bool { return false; }
inline auto start(RetroAchievements&, rc_client_t*) -> rc_client_async_handle_t* { return nullptr; }
inline auto stop(rc_client_t*, rc_client_async_handle_t*, bool) -> void {}
inline auto finish(rc_client_t*) -> void {}
inline auto setConsoleId(rc_client_t*, u32) -> void {}
inline auto menuItems(rc_client_t*) -> std::vector<RetroAchievements::DevelopmentMenuItem> { return {}; }
inline auto activateMenuItem(rc_client_t*, u32) -> bool { return false; }
inline auto hardcoreEnabled(rc_client_t*) -> bool { return false; }
inline auto hasModifications(rc_client_t*) -> bool { return false; }
inline auto achievementUnlockLabel(rc_client_t*, u32) -> const char* { return "Achievement unlocked: "; }
#endif
}
