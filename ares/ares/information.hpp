#pragma once

namespace ares {
  static const string Name       = "ares";
  static const string Version    = "118";
  static const string Copyright  = "Near";
  static const string License    = "CC BY-NC-ND 4.0";
  static const string LicenseURI = "https://creativecommons.org/licenses/by-nc-nd/4.0/";
  static const string Website    = "ares.dev";
  static const string WebsiteURI = "https://ares.dev";

  //incremented only when serialization format changes
  static const u32    SerializerSignature = 0x31545342;  //"BST1" (little-endian)
  static const string SerializerVersion   = "118";
}
