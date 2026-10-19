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

#if defined(ARES_ENABLE_RCHEEVOS)
struct rc_client_t;
#endif

struct RetroAchievements {
  ~RetroAchievements();

  auto initialize() -> void;
  auto login(const string& username, const string& password) -> bool;
  auto logout() -> void;
  auto pumpUi() -> void;
  auto requestAvatar(const string& url) -> void;
  auto hasUser() const -> bool;
  auto loginPending() const -> bool;
  auto username() const -> string;
  auto displayName() const -> string;
  auto userScore() const -> u32;
  auto userScoreSoftcore() const -> u32;
  auto avatarUrl() const -> string;
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
  auto setEnabled(bool enabled) -> void;
  auto reset() -> void;
  auto resetGame() -> void;

private:
  auto ensureClient() -> bool;
  auto syncClientLogin() -> void;
  auto loadGame() -> void;
  auto unloadClientGame() -> void;
  auto setUser(const RetroAchievementsLoginResult& result) -> void;
  auto clearUser() -> void;
  auto completeLogin(u64 generation, const RetroAchievementsLoginResult& result) -> void;

#if defined(ARES_ENABLE_RCHEEVOS)
  rc_client_t* _client = nullptr;
#endif
  string _gameHash;
  std::vector<u8> _pendingRomData;
  bool _gameReady = false;
  bool _gameLoadPending = false;
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
};

extern RetroAchievements retroAchievements;
