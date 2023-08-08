#pragma once

/**
 * Provides an interface between cores and the debugger.
 * This indirection is done to avoid pulling in the whole server + TCP dependency for each core.
 */
namespace ares::DebugServer {
}