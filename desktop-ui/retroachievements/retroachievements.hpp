struct RetroAchievementsLoginResult {
  bool success = false;
  string username;
  string token;
  string displayName;
  u32 score = 0;
  u32 scoreSoftcore = 0;
  string message;
};

struct RetroAchievements {
  auto initialize() -> void;
  auto login(const string& username, const string& password) -> RetroAchievementsLoginResult;
  auto gameLoaded() -> void;
  auto gameUnloaded() -> void;
  auto gameHash() const -> string;

private:
  string _gameHash;
};

extern RetroAchievements retroAchievements;
