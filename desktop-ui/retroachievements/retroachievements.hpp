struct RetroAchievementsLoginResult {
  bool success = false;
  string username;
  string token;
  string message;
};

struct RetroAchievements {
  auto login(const string& username, const string& password) -> RetroAchievementsLoginResult;
};

extern RetroAchievements retroAchievements;
