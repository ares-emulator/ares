#include <algorithm>

namespace {
auto clearRows(TableView& table) -> void {
  while(table.itemCount()) table.remove(table.item(table.itemCount() - 1));
}

auto sameAchievements(const RetroAchievementsGameView& left, const RetroAchievementsGameView& right) -> bool {
  if(left.status != right.status || left.message != right.message || left.gameTitle != right.gameTitle) return false;
  if(left.sets.size() != right.sets.size() || left.achievements.size() != right.achievements.size()) return false;
  for(u32 index : range(left.sets.size())) {
    auto& a = left.sets[index];
    auto& b = right.sets[index];
    if(a.id != b.id || a.title != b.title || a.badgeUrl != b.badgeUrl || a.unlocked != b.unlocked || a.total != b.total ||
       a.pointsUnlocked != b.pointsUnlocked || a.pointsTotal != b.pointsTotal || a.core != b.core) return false;
  }
  for(u32 index : range(left.achievements.size())) {
    auto& a = left.achievements[index];
    auto& b = right.achievements[index];
    if(a.id != b.id || a.setId != b.setId || a.title != b.title || a.description != b.description || a.type != b.type ||
       a.badgeUrl != b.badgeUrl || a.lockedBadgeUrl != b.lockedBadgeUrl ||
       a.points != b.points || a.unlockTime != b.unlockTime || a.unlocked != b.unlocked || a.hardcoreUnlocked != b.hardcoreUnlocked) return false;
  }
  return true;
}

auto sameLeaderboards(const RetroAchievementsGameView& left, const RetroAchievementsGameView& right) -> bool {
  if(left.status != right.status || left.message != right.message || left.gameTitle != right.gameTitle ||
     left.leaderboards.size() != right.leaderboards.size()) return false;
  for(u32 index : range(left.leaderboards.size())) {
    auto& a = left.leaderboards[index];
    auto& b = right.leaderboards[index];
    if(a.id != b.id || a.setId != b.setId || a.title != b.title || a.description != b.description ||
       a.active != b.active || a.lowerIsBetter != b.lowerIsBetter) return false;
  }
  return true;
}

auto sameEntries(const RetroAchievementsLeaderboardView& left, const RetroAchievementsLeaderboardView& right) -> bool {
  if(left.leaderboardId != right.leaderboardId || left.topEntries != right.topEntries || left.pending != right.pending ||
     left.status != right.status || left.entries.size() != right.entries.size()) return false;
  for(u32 index : range(left.entries.size())) {
    auto& a = left.entries[index];
    auto& b = right.entries[index];
    if(a.rank != b.rank || a.user != b.user || a.score != b.score || a.avatarUrl != b.avatarUrl || a.isSelf != b.isSelf) return false;
  }
  return true;
}

auto badgeStyle(image picture, bool locked, bool hardcore) -> image {
  if(!picture) return {};
  auto* pixels = (u32*)picture.data();
  auto width = picture.width();
  auto height = picture.height();
  if(locked) {
    for(u32 index : range(width * height)) {
      auto pixel = pixels[index];
      auto luma = (((pixel >> 16) & 255) * 54 + ((pixel >> 8) & 255) * 183 + (pixel & 255) * 19) / 256;
      pixels[index] = (pixel & 0xff000000) | (luma << 16) | (luma << 8) | luma;
    }
  }
  if(hardcore && width >= 2 && height >= 2) {
    for(u32 x : range(width)) pixels[x] = pixels[(height - 1) * width + x] = 0xffffd54a;
    for(u32 y : range(height)) pixels[y * width] = pixels[y * width + width - 1] = 0xffffd54a;
  }
  return picture;
}

auto achievementTitleColor(bool hardcore, bool unlocked) -> Color {
  return hardcore ? Color{160, 120, 20} : unlocked ? Color{40, 120, 60} : Color{130, 130, 130};
}

auto achievementProgressText(const RetroAchievementsGameView::Achievement& row) -> string {
  if(row.progress) return row.progress;
  if(row.hardcoreUnlocked) return "Unlocked (Hardcore)";
  if(row.unlocked) return "Unlocked";
  return "Locked";
}

auto updateAchievementIcons(AchievementsViewer& viewer) -> void {
  for(u32 index : range(viewer.lastView.sets.size())) {
    if(auto item = viewer.subsetList.item(index + 1)) {
      auto badge = retroAchievements.imageForUrl(viewer.lastView.sets[index].badgeUrl, 24);
      item.setIcon(badge ? (multiFactorImage)badge : (multiFactorImage)Icon::Action::Bookmark);
    }
  }
  for(u32 index : range(viewer.rows.size())) {
    auto item = viewer.list.item(index);
    if(!item) continue;
    auto& row = viewer.rows[index];
    auto url = row.unlocked ? row.badgeUrl : row.lockedBadgeUrl;
    if(!url) url = row.badgeUrl;
    auto badge = badgeStyle(retroAchievements.imageForUrl(url, 32), !row.unlocked, row.hardcoreUnlocked);
    if(auto cell = item.cell(0)) cell.setIcon(badge ? (multiFactorImage)badge : (multiFactorImage)Icon::Action::Bookmark);
  }
}

auto updateLeaderboardIcons(LeaderboardsViewer& viewer) -> void {
  for(u32 index : range(viewer.lastEntries.entries.size())) {
    auto item = viewer.entries.item(index);
    if(!item) continue;
    auto avatar = retroAchievements.imageForUrl(viewer.lastEntries.entries[index].avatarUrl, 24);
    if(auto cell = item.cell(1)) cell.setIcon(avatar ? (multiFactorImage)avatar : (multiFactorImage)Icon::Action::Bookmark);
  }
}
}

auto AchievementsViewer::construct() -> void {
  setCollapsible();
  setVisible(false);
  title.setText("RetroAchievements").setFont(Font().setBold());
  subsetLabel.setText("Set:");
  subsetList.onChange([&] { eventSubsetChange(); });
  summary.setForegroundColor(SystemColor::Sublabel);
  list.setHeadered();
  list.append(TableViewColumn().setText("Badge"));
  list.append(TableViewColumn().setText("Achievement").setExpandable());
  list.append(TableViewColumn().setText("Type"));
  list.append(TableViewColumn().setText("Description").setExpandable());
  list.append(TableViewColumn().setText("Progress"));
  list.append(TableViewColumn().setText("Pts").setAlignment(1.0));
}

auto AchievementsViewer::reload() -> void { refresh(); }
auto AchievementsViewer::unload() -> void {
  lastView = {};
  rows.clear();
  selectedSetId = 0;
  refresh();
}

auto AchievementsViewer::refresh() -> void {
  lastPoll = std::chrono::steady_clock::now();
  lastView = retroAchievements.gameView();
  lastImageVersion = retroAchievements.imageVersion();
  updating = true;
  clearRows(list);
  subsetList.reset();
  rows.clear();
  subsetLayout.setVisible(false);
  summary.setVisible(false);
  progress.setVisible(false);

  if(lastView.status != RetroAchievementsGameView::Status::Ready) {
    TableViewItem item{&list};
    item.append(TableViewCell());
    item.append(TableViewCell().setText(lastView.message));
    for(u32 index : range(4)) item.append(TableViewCell());
    updating = false;
    return;
  }

  bool found = selectedSetId == 0;
  for(auto& set : lastView.sets) if(set.id == selectedSetId) found = true;
  if(!found) selectedSetId = 0;
  ComboButtonItem all{&subsetList};
  all.setText("All Active Sets");
  if(!selectedSetId) all.setSelected();
  for(auto& set : lastView.sets) {
    ComboButtonItem item{&subsetList};
    item.setText({"[", set.unlocked, "/", set.total, "] ", set.title});
    if(set.id == selectedSetId) item.setSelected();
  }
  subsetLayout.setVisible(lastView.sets.size() > 1);

  u32 unlocked = 0, total = 0, pointsUnlocked = 0, pointsTotal = 0;
  string setName = lastView.sets.size() == 1 ? lastView.sets.front().title : string{"All Active Sets"};
  for(auto& set : lastView.sets) {
    if(selectedSetId && set.id != selectedSetId) continue;
    unlocked += set.unlocked;
    total += set.total;
    pointsUnlocked += set.pointsUnlocked;
    pointsTotal += set.pointsTotal;
    if(selectedSetId) setName = set.title;
  }
  summary.setVisible().setText({setName, ": ", unlocked, "/", total, " achievements, ", pointsUnlocked, "/", pointsTotal, " points"});
  progress.setVisible().setPosition(total ? (u32)(100.0 * unlocked / total) : 0);

  for(auto& row : lastView.achievements) if(!selectedSetId || row.setId == selectedSetId) rows.push_back(row);
  std::stable_sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) {
    if(a.unlocked != b.unlocked) return a.unlocked > b.unlocked;
    if(a.unlocked && a.unlockTime != b.unlockTime) return a.unlockTime > b.unlockTime;
    return a.id < b.id;
  });
  if(rows.empty()) {
    TableViewItem item{&list};
    item.append(TableViewCell());
    item.append(TableViewCell().setText("This set has no achievements."));
    for(u32 index : range(4)) item.append(TableViewCell());
  }
  for(auto& row : rows) {
    auto color = achievementTitleColor(row.hardcoreUnlocked, row.unlocked);
    auto displayTitle = row.hardcoreUnlocked ? string{row.title, " (HC)"} : row.title;
    TableViewItem item{&list};
    item.append(TableViewCell().setIcon(Icon::Action::Bookmark));
    item.append(TableViewCell().setText(displayTitle).setForegroundColor(color));
    item.append(TableViewCell().setText(row.type).setForegroundColor(color));
    item.append(TableViewCell().setText(row.description).setForegroundColor(row.unlocked ? Color{70, 70, 70} : Color{145, 145, 145}));
    item.append(TableViewCell().setText(achievementProgressText(row)).setForegroundColor(color));
    item.append(TableViewCell().setText(row.points).setForegroundColor(color));
  }
  updateAchievementIcons(*this);
  list.resizeColumns();
  list.column(0).setWidth(40);
  list.column(2).setWidth(110);
  updating = false;
}

auto AchievementsViewer::liveRefresh() -> void {
  if(!visible()) return;
  auto now = std::chrono::steady_clock::now();
  if(now - lastPoll < std::chrono::milliseconds(500)) return;
  lastPoll = now;
  auto latest = retroAchievements.gameView();
  if(!sameAchievements(lastView, latest)) return refresh();
  for(u32 index : range(rows.size())) {
    for(auto& row : latest.achievements) {
      if(row.id != rows[index].id) continue;
      if(row.progress != rows[index].progress) {
        rows[index].progress = row.progress;
        if(auto item = list.item(index)) if(auto cell = item.cell(4)) cell.setText(achievementProgressText(rows[index]));
      }
      break;
    }
  }
  lastView = std::move(latest);
  if(lastImageVersion != retroAchievements.imageVersion()) {
    lastImageVersion = retroAchievements.imageVersion();
    updateAchievementIcons(*this);
  }
}

auto AchievementsViewer::eventSubsetChange() -> void {
  if(updating) return;
  if(auto item = subsetList.selected()) {
    selectedSetId = item.offset() && item.offset() - 1 < lastView.sets.size() ? lastView.sets[item.offset() - 1].id : 0;
    refresh();
  }
}

auto AchievementsViewer::setVisible(bool visible) -> AchievementsViewer& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}

auto LeaderboardsViewer::construct() -> void {
  setCollapsible();
  setVisible(false);
  title.setText("Leaderboards").setFont(Font().setBold());
  status.setForegroundColor(SystemColor::Sublabel);
  selectedTitle.setFont(Font().setBold());
  description.setForegroundColor(SystemColor::Sublabel);
  mode.setForegroundColor(SystemColor::Sublabel);
  fetchStatus.setForegroundColor(SystemColor::Sublabel);
  list.onChange([&] { eventLeaderboardChange(); });
  aroundButton.setText("Around Me").onActivate([&] { topEntries = false; requestedId = 0; refresh(); });
  topButton.setText("Top Entries").onActivate([&] { topEntries = true; requestedId = 0; refresh(); });
  entries.setHeadered();
  entries.append(TableViewColumn().setText("Rank"));
  entries.append(TableViewColumn().setText("User"));
  entries.append(TableViewColumn().setText("Name").setExpandable());
  entries.append(TableViewColumn().setText("Score").setExpandable());
}

auto LeaderboardsViewer::reload() -> void { requestedId = 0; refresh(); }
auto LeaderboardsViewer::unload() -> void {
  selectedId = requestedId = 0;
  lastView = {};
  lastEntries = {};
  refresh();
}

auto LeaderboardsViewer::refresh() -> void {
  lastPoll = std::chrono::steady_clock::now();
  lastView = retroAchievements.gameView();
  lastEntries = retroAchievements.leaderboardView();
  lastImageVersion = retroAchievements.imageVersion();
  updating = true;
  list.reset();
  clearRows(entries);
  content.setVisible(false);
  selectedTitle.setText();
  description.setText();
  mode.setText();
  fetchStatus.setText();
  if(lastView.status != RetroAchievementsGameView::Status::Ready) {
    if(lastView.status == RetroAchievementsGameView::Status::LoginRequired) {
      status.setText("Log in to RetroAchievements to view leaderboards.");
    } else {
      status.setText(lastView.message);
    }
    updating = false;
    return;
  }
  if(lastView.leaderboards.empty()) {
    status.setText("This game has no leaderboards.");
    updating = false;
    return;
  }
  status.setText({lastView.leaderboards.size(), lastView.leaderboards.size() == 1 ? " leaderboard available." : " leaderboards available."});
  content.setVisible();
  bool found = false;
  for(auto& row : lastView.leaderboards) if(row.id == selectedId) found = true;
  if(!found) selectedId = lastView.leaderboards.front().id;
  for(auto& row : lastView.leaderboards) {
    auto item = ListViewItem().setText({row.active ? "[Active] " : "", row.title});
    list.append(item);
    if(row.id == selectedId) item.setSelected();
  }
  for(auto& row : lastView.leaderboards) {
    if(row.id != selectedId) continue;
    selectedTitle.setText(row.title);
    description.setText(row.description ? row.description : string{"No description."});
    string modeText = row.lowerIsBetter ? "Lower is better" : "Higher is better";
    if(row.trackerValue) modeText.append("  |  Current: ", row.trackerValue);
    mode.setText(modeText);
    break;
  }
  if(selectedId != requestedId || lastEntries.leaderboardId != selectedId) {
    requestedId = selectedId;
    retroAchievements.fetchLeaderboard(selectedId, topEntries);
    lastEntries = retroAchievements.leaderboardView();
  }
  if(lastEntries.leaderboardId == selectedId) {
    fetchStatus.setText(lastEntries.status);
    aroundButton.setEnabled(!lastEntries.pending);
    topButton.setEnabled(!lastEntries.pending);
    for(auto& row : lastEntries.entries) {
      auto color = row.isSelf ? Color{40, 120, 60} : Color{};
      TableViewItem item{&entries};
      item.append(TableViewCell().setText(row.rank ? string{row.rank} : string{"-"}).setForegroundColor(color));
      item.append(TableViewCell().setIcon(Icon::Action::Bookmark));
      item.append(TableViewCell().setText(row.user).setForegroundColor(color));
      item.append(TableViewCell().setText(row.score).setForegroundColor(color));
    }
    updateLeaderboardIcons(*this);
  }
  entries.resizeColumns();
  entries.column(0).setWidth(56);
  entries.column(1).setWidth(36);
  updating = false;
}

auto LeaderboardsViewer::liveRefresh() -> void {
  if(!visible()) return;
  auto now = std::chrono::steady_clock::now();
  if(now - lastPoll < std::chrono::milliseconds(500)) return;
  lastPoll = now;
  auto view = retroAchievements.gameView();
  auto scores = retroAchievements.leaderboardView();
  if(!sameLeaderboards(lastView, view) || !sameEntries(lastEntries, scores)) return refresh();
  for(auto& row : view.leaderboards) {
    if(row.id != selectedId) continue;
    string modeText = row.lowerIsBetter ? "Lower is better" : "Higher is better";
    if(row.trackerValue) modeText.append("  |  Current: ", row.trackerValue);
    mode.setText(modeText);
    break;
  }
  lastView = std::move(view);
  if(lastImageVersion != retroAchievements.imageVersion()) {
    lastImageVersion = retroAchievements.imageVersion();
    updateLeaderboardIcons(*this);
  }
}

auto LeaderboardsViewer::eventLeaderboardChange() -> void {
  if(updating) return;
  if(auto item = list.selected(); item && item.offset() < lastView.leaderboards.size()) {
    selectedId = lastView.leaderboards[item.offset()].id;
    topEntries = false;
    requestedId = 0;
    refresh();
  }
}

auto LeaderboardsViewer::setVisible(bool visible) -> LeaderboardsViewer& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}

auto ChallengesViewer::construct() -> void {
  setCollapsible();
  setVisible(false);
  title.setText("Challenges").setFont(Font().setBold());
  view.setEditable(false);
}
auto ChallengesViewer::reload() -> void { refresh(); }
auto ChallengesViewer::unload() -> void { view.setText(); }
auto ChallengesViewer::refresh() -> void { view.setText("Challenges coming soon."); }
auto ChallengesViewer::setVisible(bool visible) -> ChallengesViewer& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
