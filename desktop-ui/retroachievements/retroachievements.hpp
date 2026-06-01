struct RetroAchievementsLoginResult {
  bool success = false;
  string username;
  string token;
  string message;
};

struct RetroAchievements {
  auto login(const string& username, const string& password) -> RetroAchievementsLoginResult;
  auto gameLoaded() -> void;
  auto gameUnloaded() -> void;
  auto gameHash() const -> string;

private:
  string _gameHash;
};

extern RetroAchievements retroAchievements;
