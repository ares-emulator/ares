#include "../desktop-ui.hpp"
#include "raintegration.hpp"

#if defined(RC_CLIENT_SUPPORTS_RAINTEGRATION)
#include <cstdio>

#define boolean RA_win_boolean
extern "C" {
#include <rc_client_raintegration.h>
}
#undef boolean

namespace {

auto RC_CCONV onIntegrationLoad(int result, const char* errorMessage, rc_client_t*, void* userdata) -> void {
  if(auto* manager = static_cast<RetroAchievements*>(userdata)) manager->handleIntegrationLoad(result, errorMessage);
}

auto RC_CCONV onIntegrationEvent(const rc_client_raintegration_event_t* event, rc_client_t* client) -> void {
  if(!event || !client) return;
  auto* manager = static_cast<RetroAchievements*>(rc_client_get_userdata(client));
  if(!manager) return;
  switch(event->type) {
  case RC_CLIENT_RAINTEGRATION_EVENT_MENUITEM_CHECKED_CHANGED:
  case RC_CLIENT_RAINTEGRATION_EVENT_MENU_CHANGED:
    manager->handleIntegrationEvent(RetroAchievements::DevelopmentEvent::MenuChanged);
    break;
  case RC_CLIENT_RAINTEGRATION_EVENT_HARDCORE_CHANGED:
    manager->handleIntegrationEvent(RetroAchievements::DevelopmentEvent::HardcoreChanged);
    break;
  case RC_CLIENT_RAINTEGRATION_EVENT_PAUSE:
    manager->handleIntegrationEvent(RetroAchievements::DevelopmentEvent::Pause);
    break;
  }
}

auto RC_CCONV onWriteMemory(u32 address, u8* buffer, u32 size, rc_client_t* client) -> void {
  if(!client || !buffer || !size) return;
  if(auto* manager = static_cast<RetroAchievements*>(rc_client_get_userdata(client))) {
    manager->writeMemory(address, buffer, size);
  }
}

auto RC_CCONV onGetGameName(char* buffer, u32 size, rc_client_t* client) -> void {
  if(!buffer || !size) return;
  Program::Guard guard;
  string title;
  if(client) {
    if(auto* game = rc_client_get_game_info(client); game && game->title) title = game->title;
  }
  if(!title && emulator && emulator->root) title = emulator->root->game();
  if(!title && emulator && emulator->game) title = emulator->game->name();
  if(!title && emulator && emulator->game) title = Location::prefix(emulator->game->location);
  if(!title) title = "ares";
  std::snprintf(buffer, size, "%s", (const char*)title);
}

}

namespace RA::Integration {

auto start(RetroAchievements& manager, rc_client_t* client) -> rc_client_async_handle_t* {
  if(!client || !presentation.handle()) {
    manager.handleIntegrationLoad(RC_INVALID_STATE, "main window is unavailable");
    return nullptr;
  }
  auto directory = utf16_t(Path::program());
  return rc_client_begin_load_raintegration(
    client, (const wchar_t*)directory, (HWND)presentation.handle(),
    ares::Name, ares::Version, onIntegrationLoad, &manager
  );
}

auto stop(rc_client_t* client, rc_client_async_handle_t* pending, bool active) -> void {
  if(!client) return;
  if(pending) rc_client_abort_async(client, pending);
  if(pending || active) rc_client_unload_raintegration(client);
}

auto finish(rc_client_t* client) -> void {
  if(!client) return;
  rc_client_raintegration_set_get_game_name_function(client, onGetGameName);
  rc_client_raintegration_set_event_handler(client, onIntegrationEvent);
  rc_client_raintegration_set_write_memory_function(client, onWriteMemory);
}

auto setConsoleId(rc_client_t* client, u32 consoleId) -> void {
  if(client) rc_client_raintegration_set_console_id(client, consoleId);
}

auto menuItems(rc_client_t* client) -> std::vector<RetroAchievements::DevelopmentMenuItem> {
  std::vector<RetroAchievements::DevelopmentMenuItem> items;
  auto* menu = client ? rc_client_raintegration_get_menu(client) : nullptr;
  if(!menu) return items;
  items.reserve(menu->num_items);
  for(u32 index : range(menu->num_items)) {
    auto& source = menu->items[index];
    RetroAchievements::DevelopmentMenuItem item;
    item.id = source.id;
    item.separator = !source.id || !source.label;
    item.checked = source.checked != 0;
    item.enabled = source.enabled != 0;
    item.toggle = source.id == 1710 || source.id == 1718 || source.id == 1723;
    if(source.label) item.label = source.label;
    items.push_back(std::move(item));
  }
  return items;
}

auto activateMenuItem(rc_client_t* client, u32 id) -> bool {
  return client && id && rc_client_raintegration_activate_menu_item(client, id) != 0;
}

auto hardcoreEnabled(rc_client_t* client) -> bool {
  return client && rc_client_get_hardcore_enabled(client);
}

auto hasModifications(rc_client_t* client) -> bool {
  return client && rc_client_raintegration_has_modifications(client);
}

auto achievementUnlockLabel(rc_client_t* client, u32 achievementId) -> const char* {
  if(!client) return "Achievement unlocked: ";
  switch(rc_client_raintegration_get_achievement_state(client, achievementId)) {
  case RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_LOCAL:
    return "Local achievement unlocked: ";
  case RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_MODIFIED:
    return "Modified achievement unlocked locally: ";
  case RC_CLIENT_RAINTEGRATION_ACHIEVEMENT_STATE_INSECURE:
    return "Achievement unlocked locally: ";
  default:
    return "Achievement unlocked: ";
  }
}

}
#endif
