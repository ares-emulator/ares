#pragma once

namespace ares {
  static const string Name       = "ares";
  static const string Version    = "116.8";
  static const string License    = "CC BY-NC-ND 4.0";
  static const string LicenseURI = "https://creativecommons.org/licenses/by-nc-nd/4.0/";
  static const string Website    = "ares.dev";
  static const string WebsiteURI = "https://ares.dev";

  //incremented only when serialization format changes
  static const u32    SerializerSignature = 0x31545341;  //"AST1" (little-endian)
  static const string SerializerVersion   = "116";
}
