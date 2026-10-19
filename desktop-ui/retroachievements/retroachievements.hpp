#include <unordered_map>
#include <unordered_set>

struct RetroAchievementsLoginResult {
  bool success = false;
  int errorCode = 0;
  string username;
  string token;
  string displayName;
  string avatarUrl;
  u32 score = 0;
  u32 scoreSoftcore = 0;
  string message;
};

struct rc_client_t;
struct rc_client_async_handle_t;
struct rc_client_leaderboard_entry_list_t;

struct RetroAchievementsGameView {
  enum class Status { Disabled, LoginRequired, NoGame, Loading, Unrecognized, Error, Ready };

  struct Set {
    u32 id = 0;
    string title;
    string badgeUrl;
    u32 unlocked = 0;
    u32 total = 0;
    u32 pointsUnlocked = 0;
    u32 pointsTotal = 0;
    bool core = false;
  };

  struct Achievement {
    u32 id = 0;
    u32 setId = 0;
    string title;
    string description;
    string type;
    string progress;
    string badgeUrl;
    string lockedBadgeUrl;
    u32 points = 0;
    u64 unlockTime = 0;
    bool unlocked = false;
    bool hardcoreUnlocked = false;
  };

  struct Leaderboard {
    u32 id = 0;
    u32 setId = 0;
    string title;
    string description;
    string trackerValue;
    bool active = false;
    bool lowerIsBetter = false;
  };

  Status status = Status::Disabled;
  string message;
  string gameTitle;
  std::vector<Set> sets;
  std::vector<Achievement> achievements;
  std::vector<Leaderboard> leaderboards;
};

struct RetroAchievementsLeaderboardView {
  struct Entry {
    u32 rank = 0;
    string user;
    string score;
    string avatarUrl;
    bool isSelf = false;
  };

  u32 leaderboardId = 0;
  bool topEntries = false;
  bool pending = false;
  string status;
  std::vector<Entry> entries;
};

struct RetroAchievements {
  ~RetroAchievements();

  auto initialize() -> void;
  auto login(const string& username, const string& password) -> bool;
  auto logout() -> void;
  auto pumpUi() -> void;
  auto requestImage(const string& url) -> void;
  auto hasUser() const -> bool;
  auto loginPending() const -> bool;
  auto username() const -> string;
  auto displayName() const -> string;
  auto userScore() const -> u32;
  auto userScoreSoftcore() const -> u32;
  auto avatarUrl() const -> string;
  auto gameView() const -> RetroAchievementsGameView;
  auto leaderboardView() const -> RetroAchievementsLeaderboardView;
  auto fetchLeaderboard(u32 leaderboardId, bool topEntries) -> void;
  auto handleLeaderboardResult(int result, const char* errorMessage, rc_client_leaderboard_entry_list_t* list, u64 generation) -> void;
  auto imageForUrl(const string& url, u32 size) -> image;
  auto imageVersion() const -> u64;
  auto readMemory(u32 address, u8* buffer, u32 size) const -> u32;
  auto handleClientLoginResult(int result, const char* errorMessage, u64 generation) -> void;
  auto handleClientGameLoadResult(int result, const char* errorMessage, u64 generation) -> void;
  auto frame() -> void;
  auto idle() -> void;
  auto gameLoaded() -> void;
  auto gameUnloaded() -> void;
  auto gameHash() const -> string;
  auto hardcore() const -> bool;
  auto supportsGame() const -> bool;
  auto enableHardcore(bool enabled) -> bool;
  auto enableEncore(bool enabled) -> void;
  auto setEnabled(bool enabled) -> void;
  auto encoreUnlock(u32 achievementId) const -> bool;
  auto reset() -> void;
  auto resetGame() -> void;

private:
  auto ensureClient() -> bool;
  auto beginSavedTokenLogin(bool startup) -> bool;
  auto syncClientLogin() -> void;
  auto loadGame() -> void;
  auto unloadClientGame() -> void;
  auto reloadEncoreGameIfNeeded() -> void;
  auto setUser(const RetroAchievementsLoginResult& result) -> void;
  auto clearUser() -> void;
  auto completeLogin(u64 generation, const RetroAchievementsLoginResult& result) -> void;

  rc_client_t* _client = nullptr;
  string _gameHash;
  string _gameLoadError;
  std::vector<u8> _pendingRomData;
  bool _gameReady = false;
  bool _gameLoadPending = false;
  bool _gameUnrecognized = false;
  bool _encoreGameActive = false;
  bool _reloadEncoreGame = false;
  std::unordered_set<u32> _previousUnlocks;
  string _username;
  string _token;
  string _displayName;
  string _avatarUrl;
  u32 _score = 0;
  u32 _scoreSoftcore = 0;
  bool _authenticated = false;
  bool _loginPending = false;
  bool _startupTokenLoginPending = false;
  u64 _loginGeneration = 0;
  u64 _gameGeneration = 0;
  RetroAchievementsLeaderboardView _leaderboardView;
  rc_client_async_handle_t* _leaderboardFetch = nullptr;
  u64 _leaderboardGeneration = 0;
  std::unordered_map<std::string, image> _images;
  std::unordered_set<std::string> _pendingImages;
  std::unordered_set<std::string> _failedImages;
  u64 _imageVersion = 0;
  bool _shuttingDown = false;
};

extern RetroAchievements retroAchievements;
