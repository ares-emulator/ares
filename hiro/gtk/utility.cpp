namespace hiro {

#if defined(Hiro_Color)
static auto CreateColor(const Color& color) -> GdkColor {
  GdkColor gdkColor;
  gdkColor.pixel = (color.red() << 16) | (color.green() << 8) | (color.blue() << 0);
  gdkColor.red = (color.red() << 8) | (color.red() << 0);
  gdkColor.green = (color.green() << 8) | (color.green() << 0);
  gdkColor.blue = (color.blue() << 8) | (color.blue() << 0);
  return gdkColor;
}
#endif

static auto CreatePixbuf(image icon, bool scale = false) -> GdkPixbuf* {
  if(!icon) return nullptr;

  if(scale) icon.scale(15, 15);
  icon.transform(0, 32, 255u << 24, 255u << 0, 255u << 8, 255u << 16);  //GTK stores images in ABGR format

  auto pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, true, 8, icon.width(), icon.height());
  memory::copy(gdk_pixbuf_get_pixels(pixbuf), icon.data(), icon.size());

  if(scale) {
    auto scaled = gdk_pixbuf_scale_simple(pixbuf, 15, 15, GDK_INTERP_BILINEAR);
    g_object_unref(pixbuf);
    pixbuf = scaled;
  }

  return pixbuf;
}

static auto CreateImage(const image& icon, bool scale = false) -> GtkImage* {
  auto pixbuf = CreatePixbuf(icon, scale);
  auto gtkIcon = (GtkImage*)gtk_image_new_from_pixbuf(pixbuf);
  g_object_unref(pixbuf);
  return gtkIcon;
}

static auto DropPaths(GtkSelectionData* data) -> std::vector<string> {
  gchar** uris = gtk_selection_data_get_uris(data);
  if(uris == nullptr) return {};

  std::vector<string> paths;
  for(u32 n = 0; uris[n] != nullptr; n++) {
    gchar* pathname = g_filename_from_uri(uris[n], nullptr, nullptr);
    if(pathname == nullptr) continue;

    string path = pathname;
    g_free(pathname);
    if(directory::exists(path) && !path.endsWith("/")) path.append("/");
    paths.push_back(path);
  }

  g_strfreev(uris);
  return paths;
}

}
