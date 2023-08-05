/**
 * HTTP based debugging server.
 * 
 * This allows for remote debugging over TCP/HTTP.
 * Only reading/writing memory is supported at the moment.
 */
class DebugServer {
  public:
    auto start(u32 port) -> bool;

    auto update() -> void;
    auto stop() -> bool;

  private:
    bool isOpen = false;
};
