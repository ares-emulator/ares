#if defined(Hiro_VerticalLayout)

struct VerticalLayout;
struct VerticalLayoutCell;

struct mVerticalLayout;
struct mVerticalLayoutCell;

using sVerticalLayout = shared_pointer<mVerticalLayout>;
using sVerticalLayoutCell = shared_pointer<mVerticalLayoutCell>;

struct mVerticalLayout : mSizable {
  using type = mVerticalLayout;
  using mSizable::remove;

  auto alignment() const -> maybe<f32>;
  auto append(sSizable sizable, Size size, f32 spacing = 5_sy) -> type&;
  auto cell(u32 position) const -> VerticalLayoutCell;
  auto cell(sSizable sizable) const -> VerticalLayoutCell;
  auto cells() const -> vector<VerticalLayoutCell>;
  auto cellCount() const -> u32;
  auto minimumSize() const -> Size override;
  auto padding() const -> Geometry;
  auto remove(sSizable sizable) -> type&;
  auto remove(sVerticalLayoutCell cell) -> type&;
  auto reset() -> type& override;
  auto resize() -> type&;
  auto setAlignment(maybe<f32> alignment) -> type&;
  auto setEnabled(bool enabled) -> type& override;
  auto setFont(const Font& font) -> type& override;
  auto setGeometry(Geometry geometry) -> type& override;
  auto setPadding(Geometry padding) -> type&;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setSpacing(f32 spacing) -> type&;
  auto setVisible(bool visible) -> type& override;
  auto spacing() const -> f32;
  auto synchronize() -> type&;

private:
  auto destruct() -> void override;

  struct State {
    maybe<f32> alignment;
    vector<VerticalLayoutCell> cells;
    Geometry padding;
    f32 spacing = 5_sy;
  } state;
};

struct mVerticalLayoutCell : mObject {
  using type = mVerticalLayoutCell;

  auto alignment() const -> maybe<f32>;
  auto collapsible() const -> bool;
  auto setAlignment(maybe<f32> alignment) -> type&;
  auto setEnabled(bool enabled) -> type& override;
  auto setFont(const Font& font) -> type& override;
  auto setParent(mObject* parent = nullptr, s32 offset = -1) -> type& override;
  auto setSizable(sSizable sizable) -> type&;
  auto setSize(Size size) -> type&;
  auto setSpacing(f32 spacing) -> type&;
  auto setVisible(bool visible) -> type& override;
  auto sizable() const -> Sizable;
  auto size() const -> Size;
  auto spacing() const -> f32;
  auto synchronize() -> type&;

private:
  auto destruct() -> void override;

  struct State {
    maybe<f32> alignment;
    sSizable sizable;
    Size size;
    f32 spacing = 5_sy;
  } state;

  friend class mVerticalLayout;
};

#endif
