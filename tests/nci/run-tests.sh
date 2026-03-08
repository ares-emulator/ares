#!/usr/bin/env bash
set -euo pipefail

PORT=55355
HOST="localhost"
TIMEOUT=2
PASS=0
FAIL=0
SKIP=0
ARES_PID=""

cleanup() {
  if [ -n "$ARES_PID" ] && kill -0 "$ARES_PID" 2>/dev/null; then
    kill "$ARES_PID" 2>/dev/null || true
    wait "$ARES_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT

# find ares binary
ARES_BIN="${1:-}"
if [ -z "$ARES_BIN" ]; then
  # search common build paths
  SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
  ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
  for candidate in \
    "$ROOT_DIR/build_local/desktop-ui/ares.app/Contents/MacOS/ares" \
    "$ROOT_DIR/build/desktop-ui/ares.app/Contents/MacOS/ares" \
    "$ROOT_DIR/build_local/desktop-ui/ares" \
    "$ROOT_DIR/build/desktop-ui/ares"; do
    if [ -x "$candidate" ]; then
      ARES_BIN="$candidate"
      break
    fi
  done
fi

if [ -z "$ARES_BIN" ] || [ ! -x "$ARES_BIN" ]; then
  echo "SKIP: ares binary not found (pass path as first argument)"
  echo "pass 0 / fail 0 / skip 1"
  exit 0
fi

echo "NCI Protocol Integration Tests"
echo "==============================="
echo "Using: $ARES_BIN"
echo ""

# start ares with NCI enabled
"$ARES_BIN" --setting NCI/Enabled=true --no-file-prompt &
ARES_PID=$!

# wait for NCI server to be ready
sleep 3

if ! kill -0 "$ARES_PID" 2>/dev/null; then
  echo "FAIL: ares failed to start"
  echo "pass 0 / fail 1 / skip 0"
  exit 1
fi

send_command() {
  echo -n "$1" | nc -u -w"$TIMEOUT" "$HOST" "$PORT" 2>/dev/null || true
}

# test_command <command> <expected_pattern> <description>
# pattern is matched with grep -q (regex)
test_command() {
  local cmd="$1"
  local pattern="$2"
  local desc="$3"

  local response
  response="$(send_command "$cmd")"

  if echo "$response" | grep -q "$pattern"; then
    echo "  PASS: $desc"
    echo "    response: $response"
    PASS=$((PASS + 1))
  else
    echo "  FAIL: $desc"
    echo "    command:  $cmd"
    echo "    expected: $pattern"
    echo "    got:      $response"
    FAIL=$((FAIL + 1))
  fi
}

echo "System commands:"
test_command "VERSION" "." "VERSION returns non-empty response"
test_command "GET_STATUS" "^GET_STATUS CONTENTLESS" "GET_STATUS returns CONTENTLESS without ROM"
test_command "QUIT_BOGUS" "^UNKNOWN_COMMAND" "unknown command returns UNKNOWN_COMMAND"

echo ""
echo "Config params (GET):"
test_command "GET_CONFIG_PARAM Audio/Volume" "^GET_CONFIG_PARAM Audio/Volume" "GET_CONFIG_PARAM Audio/Volume"
test_command "GET_CONFIG_PARAM NCI/Port" "^GET_CONFIG_PARAM NCI/Port" "GET_CONFIG_PARAM NCI/Port"
test_command "GET_CONFIG_PARAM Paths/Screenshots" "^GET_CONFIG_PARAM Paths/Screenshots" "GET_CONFIG_PARAM Paths/Screenshots"
test_command "GET_CONFIG_PARAM Paths/Saves" "^GET_CONFIG_PARAM Paths/Saves" "GET_CONFIG_PARAM Paths/Saves"
test_command "GET_CONFIG_PARAM Nonexistent/Path" "^UNSUPPORTED" "GET_CONFIG_PARAM unknown path returns UNSUPPORTED"

echo ""
echo "Config params (SET):"
test_command "SET_CONFIG_PARAM Audio/Balance 0.5" "^SET_CONFIG_PARAM Audio/Balance 0.5" "SET_CONFIG_PARAM Audio/Balance 0.5"
test_command "SET_CONFIG_PARAM Nonexistent/Path foo" "^UNSUPPORTED" "SET_CONFIG_PARAM unknown path returns UNSUPPORTED"
# restore balance to default
send_command "SET_CONFIG_PARAM Audio/Balance 0.0" >/dev/null 2>&1

echo ""
echo "Display commands:"
test_command "SHOW_MSG test" "^SHOW_MSG OK" "SHOW_MSG returns OK"
test_command "SET_SHADER None" "^SET_SHADER None" "SET_SHADER returns shader path"
test_command "FULLSCREEN_TOGGLE" "^FULLSCREEN_TOGGLE OK" "FULLSCREEN_TOGGLE returns OK"
# toggle back to restore state
send_command "FULLSCREEN_TOGGLE" >/dev/null 2>&1

echo ""
echo "Audio commands:"
test_command "MUTE" "^MUTE " "MUTE returns state"
# toggle back
send_command "MUTE" >/dev/null 2>&1
test_command "VOLUME_UP" "^VOLUME_UP " "VOLUME_UP returns level"
test_command "VOLUME_DOWN" "^VOLUME_DOWN " "VOLUME_DOWN returns level"

echo ""
echo "Playback commands (contentless - expect toggle or error):"
test_command "PAUSE_TOGGLE" "^PAUSE_TOGGLE " "PAUSE_TOGGLE returns state"
# toggle back
send_command "PAUSE_TOGGLE" >/dev/null 2>&1

echo ""
echo "State commands:"
test_command "STATE_SLOT_PLUS" "^STATE_SLOT_PLUS " "STATE_SLOT_PLUS returns slot"
test_command "STATE_SLOT_MINUS" "^STATE_SLOT_MINUS " "STATE_SLOT_MINUS returns slot"

echo ""
echo "Commands requiring content (should return -1):"
test_command "SAVE_STATE" "^SAVE_STATE -1" "SAVE_STATE returns -1 without content"
test_command "LOAD_STATE" "^LOAD_STATE -1" "LOAD_STATE returns -1 without content"
test_command "LOAD_STATE_SLOT 1" "^LOAD_STATE_SLOT -1" "LOAD_STATE_SLOT returns -1 without content"
test_command "SCREENSHOT" "^SCREENSHOT -1" "SCREENSHOT returns -1 without content"
test_command "RESET" "^RESET -1" "RESET returns -1 without content"
test_command "FAST_FORWARD" "^FAST_FORWARD -1" "FAST_FORWARD returns -1 without content"
test_command "FRAMEADVANCE" "^FRAMEADVANCE -1" "FRAMEADVANCE returns -1 without content"
test_command "REWIND" "^REWIND -1" "REWIND returns -1 without content"
test_command "READ_CORE_MEMORY 0000 1" "^READ_CORE_MEMORY -1" "READ_CORE_MEMORY returns -1 without content"
test_command "WRITE_CORE_MEMORY 0000 FF" "^WRITE_CORE_MEMORY -1" "WRITE_CORE_MEMORY returns -1 without content"
test_command "CLOSE_CONTENT" "^CLOSE_CONTENT -1" "CLOSE_CONTENT returns -1 without content"
test_command "LOAD_CONTENT /nonexistent/path.rom" "^LOAD_CONTENT -1" "LOAD_CONTENT returns -1 for nonexistent file"
test_command "LOAD_CONTENT" "^LOAD_CONTENT -1" "LOAD_CONTENT returns -1 with no args"
test_command "DISK_EJECT_TOGGLE" "^DISK_EJECT_TOGGLE -1" "DISK_EJECT_TOGGLE returns -1 without content"

echo ""
echo "Shutdown:"
QUIT_RESPONSE="$(send_command "QUIT")"
if echo "$QUIT_RESPONSE" | grep -q "^QUIT OK"; then
  echo "  PASS: QUIT returns OK"
  PASS=$((PASS + 1))
else
  echo "  FAIL: QUIT returns OK"
  echo "    got: $QUIT_RESPONSE"
  FAIL=$((FAIL + 1))
fi

# wait for ares to exit
wait "$ARES_PID" 2>/dev/null || true
ARES_PID=""

echo ""
echo "pass $PASS / fail $FAIL / skip $SKIP"

if [ "$FAIL" -gt 0 ]; then
  exit 1
fi
