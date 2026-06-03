struct RetroAchievementsLoginResult {
  bool success = false;
  string username;
  string token;
  string displayName;
  string avatarUrl;
  u32 score = 0;
  u32 scoreSoftcore = 0;
  string message;
};

struct RetroAchievements {
  auto initialize() -> void;
  auto login(const string& username, const string& password) -> RetroAchievementsLoginResult;
  auto logout() -> void;
  auto hasUser() const -> bool;
  auto username() const -> string;
  auto displayName() const -> string;
  auto userScore() const -> u32;
  auto userScoreSoftcore() const -> u32;
  auto avatarUrl() const -> string;
  auto readMemory(u32 address, u8* buffer, u32 size) const -> u32;
  auto gameLoaded() -> void;
  auto gameUnloaded() -> void;
  auto gameHash() const -> string;

private:
  auto setUser(const RetroAchievementsLoginResult& result) -> void;
  auto clearUser() -> void;

  string _gameHash;
  string _username;
  string _displayName;
  string _avatarUrl;
  u32 _score = 0;
  u32 _scoreSoftcore = 0;
  bool _authenticated = false;
};

extern RetroAchievements retroAchievements;
