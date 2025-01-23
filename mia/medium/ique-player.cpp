struct iQuePlayer : Medium {
  auto name() -> string override { return "iQue Player"; }
  auto extensions() -> vector<string> override { return {}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto iQuePlayer::load(string location) -> bool {
  this->location = location;
  this->manifest = "game\n  name:  [system]\n  title:  [system]\n  region:  CHN";
  
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->append("manifest.bml", manifest);

  return true;
}

auto iQuePlayer::save(string location) -> bool {
  return true;
}