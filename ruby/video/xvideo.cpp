#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

extern "C" auto XvShmCreateImage(Display*, XvPortID, int, char*, int, int, XShmSegmentInfo*) -> XvImage*;

struct VideoXVideo : VideoDriver {
  VideoXVideo& self = *this;
  VideoXVideo(Video& super) : VideoDriver(super) {}
  ~VideoXVideo() { terminate(); }

  auto create() -> bool override {
    VideoDriver::exclusive = true;
    VideoDriver::shader = "Blur";
    return initialize();
  }

  auto driver() -> string override { return "XVideo"; }
  auto ready() -> bool override { return _ready; }

  auto hasFullScreen() -> bool override { return true; }
  auto hasMonitor() -> bool override { return true; }
  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }

  auto hasFormats() -> vector<string> override {
    return _formatNames;
  }

  auto setFullScreen(bool fullScreen) -> bool override {
    return initialize();
  }

  auto setMonitor(string monitor) -> bool override {
    return initialize();
  }

  auto setContext(uintptr context) -> bool override {
    return initialize();
  }

  auto setBlocking(bool blocking) -> bool override {
    bool result = false;
    Display* display = XOpenDisplay(nullptr);
    Atom atom = XInternAtom(display, "XV_SYNC_TO_VBLANK", true);
    if(atom != None && _port >= 0) {
      XvSetPortAttribute(display, _port, atom, self.blocking);
      result = true;
    }
    XCloseDisplay(display);
    return result;
  }

  auto setFormat(string format) -> bool override {
    return initialize();
  }

  auto focused() -> bool override {
    return true;
  }

  auto clear() -> void override {
    memory::fill<u32>(_buffer, _bufferWidth * _bufferHeight);
    //clear twice in case video is double buffered ...
    output();
    output();
  }

  auto size(u32& width, u32& height) -> void override {
    if(self.fullScreen) {
      width = _monitorWidth;
      height = _monitorHeight;
    } else {
      XWindowAttributes parent;
      XGetWindowAttributes(_display, _parent, &parent);
      width = parent.width;
      height = parent.height;
    }
  }

  auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool override {
    if(width != _width || height != _height) resize(_width = width, _height = height);
    pitch = _bufferWidth * 4;
    return data = _buffer;
  }

  auto release() -> void override {
  }

  auto output(u32 width = 0, u32 height = 0) -> void override {
    XWindowAttributes window;
    XGetWindowAttributes(_display, _window, &window);

    XWindowAttributes parent;
    XGetWindowAttributes(_display, _parent, &parent);

    if(window.width != parent.width || window.height != parent.height) {
      XResizeWindow(_display, _window, parent.width, parent.height);
    }

    u32 viewportX = 0;
    u32 viewportY = 0;
    u32 viewportWidth = parent.width;
    u32 viewportHeight = parent.height;

    if(self.fullScreen) {
      viewportX = _monitorX;
      viewportY = _monitorY;
      viewportWidth = _monitorWidth;
      viewportHeight = _monitorHeight;
    }

    auto& name = _formatName;
    if(name == "RGB24" ) renderRGB24 (_width, _height);
    if(name == "RGB24P") renderRGB24P(_width, _height);
    if(name == "RGB16" ) renderRGB16 (_width, _height);
    if(name == "RGB15" ) renderRGB15 (_width, _height);
    if(name == "UYVY"  ) renderUYVY  (_width, _height);
    if(name == "YUY2"  ) renderYUY2  (_width, _height);
    if(name == "YV12"  ) renderYV12  (_width, _height);
    if(name == "I420"  ) renderI420  (_width, _height);

    if(!width) width = viewportWidth;
    if(!height) height = viewportHeight;
    s32 x = viewportX + ((s32)viewportWidth - (s32)width) / 2;
    s32 y = viewportY + ((s32)viewportHeight - (s32)height) / 2;

    XvShmPutImage(_display, _port, _window, _gc, _image,
      0, 0, _width, _height,
      x, y, width, height,
      true);
  }

  auto poll() -> void override {
    while(XPending(_display)) {
      XEvent event;
      XNextEvent(_display, &event);
      if(event.type == Expose) {
        XWindowAttributes attributes;
        XGetWindowAttributes(_display, _window, &attributes);
        super.doUpdate(attributes.width, attributes.height);
      }
    }
  }

private:
  auto initialize() -> bool {
    terminate();
    if(!self.fullScreen && !self.context) return false;

    _display = XOpenDisplay(nullptr);
    _screen = DefaultScreen(_display);

    if(!XShmQueryExtension(_display)) {
      print("XVideo: XShm extension not found.\n");
      return false;
    }

    //find an appropriate Xv port
    _port = -1;
    s32 depth = 0;
    s32 visualID = 0;
    XvAdaptorInfo* adaptorInfo = nullptr;
    u32 adaptorCount = 0;
    XvQueryAdaptors(_display, DefaultRootWindow(_display), &adaptorCount, &adaptorInfo);
    for(u32 n : range(adaptorCount)) {
      //find adaptor that supports both input (memory->drawable) and image (drawable->screen) masks
      if(adaptorInfo[n].num_formats < 1) continue;
      if(!(adaptorInfo[n].type & XvInputMask)) continue;
      if(!(adaptorInfo[n].type & XvImageMask)) continue;

      _port = adaptorInfo[n].base_id;
      depth = adaptorInfo[n].formats->depth;
      visualID = adaptorInfo[n].formats->visual_id;
      break;
    }
    XvFreeAdaptorInfo(adaptorInfo);
    if(_port < 0) {
      print("XVideo: failed to find valid XvPort.\n");
      return false;
    }

    XVisualInfo visualTemplate;
    visualTemplate.visualid = visualID;
    visualTemplate.screen = _screen;
    visualTemplate.depth = depth;
    visualTemplate.visual = 0;
    s32 visualMatches = 0;
    auto visualInfo = XGetVisualInfo(_display, VisualIDMask | VisualScreenMask | VisualDepthMask, &visualTemplate, &visualMatches);
    if(visualMatches < 1 || !visualInfo->visual) {
      if(visualInfo) XFree(visualInfo);
      print("XVideo: unable to find Xv-compatible visual.\n");
      return false;
    }

    _parent = self.fullScreen ? RootWindow(_display, _screen) : (Window)self.context;
    //create child window to attach to parent window.
    //this is so that even if parent window visual depth doesn't match Xv visual
    //(common with composited windows), Xv can still render to child window.
    XWindowAttributes windowAttributes{};
    XGetWindowAttributes(_display, _parent, &windowAttributes);

    auto monitor = Video::monitor(self.monitor);
    _monitorX = monitor.x;
    _monitorY = monitor.y;
    _monitorWidth = monitor.width;
    _monitorHeight = monitor.height;

    _colormap = XCreateColormap(_display, _parent, visualInfo->visual, AllocNone);
    XSetWindowAttributes attributes{};
    attributes.border_pixel = 0;
    attributes.colormap = _colormap;
    attributes.override_redirect = self.fullScreen;
    _window = XCreateWindow(_display, _parent,
      0, 0, windowAttributes.width, windowAttributes.height,
      0, depth, InputOutput, visualInfo->visual,
      CWBorderPixel | CWColormap | CWOverrideRedirect, &attributes);
    XSelectInput(_display, _window, ExposureMask);
    XFree(visualInfo);
    XSetWindowBackground(_display, _window, 0);
    XMapWindow(_display, _window);

    _gc = XCreateGC(_display, _window, 0, 0);

    s32 attributeCount = 0;
    auto attributeList = XvQueryPortAttributes(_display, _port, &attributeCount);
    for(auto n : range(attributeCount)) {
      if(string{attributeList[n].name} == "XV_AUTOPAINT_COLORKEY") {
        //set colorkey to auto paint, so that Xv video output is always visible
        Atom atom = XInternAtom(_display, "XV_AUTOPAINT_COLORKEY", true);
        if(atom != None) XvSetPortAttribute(_display, _port, atom, 1);
      }
    }
    XFree(attributeList);

    queryAvailableFormats();
    if(!_formatNames) {
      print("XVideo: unable to find a supported image format.\n");
      return false;
    }
    if(auto match = _formatNames.find(self.format)) {
      _formatID = _formatIDs[match()];
      _formatName = _formatNames[match()];
    } else {
      _formatID = _formatIDs[0];
      _formatName = _formatNames[0];
      self.format = _formatName;
    }

    _ready = true;
    initializeTables();
    resize(_width = 256, _height = 256);
    clear();
    return true;
  }

  auto terminate() -> void {
    _ready = false;

    if(_image) {
      XShmDetach(_display, &_shmInfo);
      shmdt(_shmInfo.shmaddr);
      shmctl(_shmInfo.shmid, IPC_RMID, nullptr);
      XFree(_image);
      _image = nullptr;
    }

    if(_gc) {
      XFreeGC(_display, _gc);
      _gc = 0;
    }

    if(_window) {
      XUnmapWindow(_display, _window);
      _window = 0;
    }

    if(_colormap) {
      XFreeColormap(_display, _colormap);
      _colormap = 0;
    }

    if(_display) {
      XCloseDisplay(_display);
      _display = nullptr;
    }

    delete[] _buffer, _buffer = nullptr, _bufferWidth = 0, _bufferHeight = 0;
    delete[] _ytable, _ytable = nullptr;
    delete[] _utable, _utable = nullptr;
    delete[] _vtable, _vtable = nullptr;
  }

  auto queryAvailableFormats() -> void {
    auto& ids = _formatIDs;
    auto& names = _formatNames;

    ids.reset();
    names.reset();

    s32 count = 0;
    auto array = XvListImageFormats(_display, _port, &count);

    for(u32 sort : range(8)) {
      for(u32 n : range(count)) {
        auto id = array[n].id;
        auto type = array[n].type;
        auto format = array[n].format;
        auto depth = array[n].bits_per_pixel;
        auto redMask = array[n].red_mask;
        auto order = array[n].component_order;
        string components;
        for(u32 n : range(4)) if(char c = order[n]) components.append(c);

        if(type == XvRGB) {
          if(sort == 0 && depth == 32) ids.append(id), names.append("RGB24");
          if(sort == 1 && depth == 24) ids.append(id), names.append("RGB24P");
          if(sort == 2 && depth <= 16 && redMask == 0xf800) ids.append(id), names.append("RGB16");
          if(sort == 3 && depth <= 16 && redMask == 0x7c00) ids.append(id), names.append("RGB15");
        }

        if(type == XvYUV && format == XvPacked) {
          if(sort == 4 && depth == 16 && components == "UYVY") ids.append(id), names.append("UYVY");
          if(sort == 5 && depth == 16 && components == "YUYV") ids.append(id), names.append("YUY2");
        }

        if(type == XvYUV && format == XvPlanar) {
          if(sort == 6 && depth == 12 && components == "YVU" ) ids.append(id), names.append("YV12");
          if(sort == 7 && depth == 12 && components == "YUV" ) ids.append(id), names.append("I420");
        }
      }
    }

    free(array);
  }

  auto resize(u32 width, u32 height) -> void {
    if(_bufferWidth >= width && _bufferHeight >= height) return;
    _bufferWidth = max(width, _bufferWidth);
    _bufferHeight = max(height, _bufferHeight);

    //must round to be evenly divisible by 4
    if(u32 round = _bufferWidth & 3) _bufferWidth += 4 - round;
    if(u32 round = _bufferHeight & 3) _bufferHeight += 4 - round;

    _bufferWidth = bit::round(_bufferWidth);
    _bufferHeight = bit::round(_bufferHeight);

    if(_image) {
      XShmDetach(_display, &_shmInfo);
      shmdt(_shmInfo.shmaddr);
      shmctl(_shmInfo.shmid, IPC_RMID, nullptr);
      XFree(_image);
    }

    _image = XvShmCreateImage(_display, _port, _formatID, 0, _bufferWidth, _bufferHeight, &_shmInfo);

    _shmInfo.shmid = shmget(IPC_PRIVATE, _image->data_size, IPC_CREAT | 0777);
    _shmInfo.shmaddr = _image->data = (char*)shmat(_shmInfo.shmid, 0, 0);
    _shmInfo.readOnly = false;
    XShmAttach(_display, &_shmInfo);

    delete[] _buffer;
    _buffer = new u32[_bufferWidth * _bufferHeight];
  }

  auto renderRGB24(u32 width, u32 height) -> void {
    for(u32 y : range(height)) {
      auto input = (const u32*)_buffer + y * width;
      auto output = (u32*)_image->data + y * (_image->pitches[0] >> 2);

      for(u32 x : range(width)) {
        u32 p = *input++;
        *output++ = p;
      }
    }
  }

  auto renderRGB24P(u32 width, u32 height) -> void {
    for(u32 y : range(height)) {
      auto input = (const u32*)_buffer + y * width;
      auto output = (u8*)_image->data + y * _image->pitches[0];

      for(u32 x : range(width)) {
        u32 p = *input++;
        *output++ = p >>  0;
        *output++ = p >>  8;
        *output++ = p >> 16;
      }
    }
  }

  auto renderRGB16(u32 width, u32 height) -> void {
    for(u32 y : range(height)) {
      auto input = (const u32*)_buffer + y * width;
      auto output = (u16*)_image->data + y * (_image->pitches[0] >> 1);

      for(u32 x : range(width)) {
        u32 p = toRGB16(*input++);
        *output++ = p;
      }

      input += _bufferWidth - width;
      output += _bufferWidth - width;
    }
  }

  auto renderRGB15(u32 width, u32 height) -> void {
    for(u32 y : range(height)) {
      auto input = (const u32*)_buffer + y * width;
      auto output = (u16*)_image->data + y * (_image->pitches[0] >> 1);

      for(u32 x : range(width)) {
        u32 p = toRGB15(*input++);
        *output++ = p;
      }
    }
  }

  auto renderUYVY(u32 width, u32 height) -> void {
    for(u32 y : range(height)) {
      auto input = (const u32*)_buffer + y * width;
      auto output = (u16*)_image->data + y * (_image->pitches[0] >> 1);

      for(u32 x : range(width >> 1)) {
        u32 p0 = toRGB16(*input++);
        u32 p1 = toRGB16(*input++);

        *output++ = _ytable[p0] << 8 | ((_utable[p0] + _utable[p1]) >> 1) << 0;
        *output++ = _ytable[p1] << 8 | ((_vtable[p0] + _vtable[p1]) >> 1) << 0;
      }
    }
  }

  auto renderYUY2(u32 width, u32 height) -> void {
    for(u32 y : range(height)) {
      auto input = (const u32*)_buffer + y * width;
      auto output = (u16*)_image->data + y * (_image->pitches[0] >> 1);

      for(u32 x : range(width >> 1)) {
        u32 p0 = toRGB16(*input++);
        u32 p1 = toRGB16(*input++);

        *output++ = ((_utable[p0] + _utable[p1]) >> 1) << 8 | _ytable[p0] << 0;
        *output++ = ((_vtable[p0] + _vtable[p1]) >> 1) << 8 | _ytable[p1] << 0;
      }
    }
  }

  auto renderYV12(u32 width, u32 height) -> void {
    for(u32 y : range(height >> 1)) {
      auto input0 = (const u32*)_buffer + (2 * y + 0) * width;
      auto input1 = (const u32*)_buffer + (2 * y + 1) * width;
      auto youtput0 = (u16*)_image->data + (_image->offsets[0] >> 1) + (2 * y + 0) * (_image->pitches[0] >> 1);
      auto youtput1 = (u16*)_image->data + (_image->offsets[0] >> 1) + (2 * y + 1) * (_image->pitches[0] >> 1);
      auto voutput = (u8*)_image->data + _image->offsets[1] + y * _image->pitches[1];
      auto uoutput = (u8*)_image->data + _image->offsets[2] + y * _image->pitches[2];

      for(u32 x : range(width >> 1)) {
        u16 p0 = toRGB16(*input0++);
        u16 p1 = toRGB16(*input0++);
        u16 p2 = toRGB16(*input1++);
        u16 p3 = toRGB16(*input1++);

        *youtput0++ = _ytable[p0] << 0 | _ytable[p1] << 8;
        *youtput1++ = _ytable[p2] << 0 | _ytable[p3] << 8;
        *voutput++ = (_vtable[p0] + _vtable[p1] + _vtable[p2] + _vtable[p3]) >> 2;
        *uoutput++ = (_utable[p0] + _utable[p1] + _utable[p2] + _utable[p3]) >> 2;
      }
    }
  }

  auto renderI420(u32 width, u32 height) -> void {
    for(u32 y : range(height >> 1)) {
      auto input0 = (const u32*)_buffer + (2 * y + 0) * width;
      auto input1 = (const u32*)_buffer + (2 * y + 1) * width;
      auto youtput0 = (u16*)_image->data + (_image->offsets[0] >> 1) + (2 * y + 0) * (_image->pitches[0] >> 1);
      auto youtput1 = (u16*)_image->data + (_image->offsets[0] >> 1) + (2 * y + 1) * (_image->pitches[0] >> 1);
      auto uoutput = (u8*)_image->data + _image->offsets[1] + y * _image->pitches[1];
      auto voutput = (u8*)_image->data + _image->offsets[2] + y * _image->pitches[2];

      for(u32 x : range(width >> 1)) {
        u16 p0 = toRGB16(*input0++);
        u16 p1 = toRGB16(*input0++);
        u16 p2 = toRGB16(*input1++);
        u16 p3 = toRGB16(*input1++);

        *youtput0++ = _ytable[p0] << 0 | _ytable[p1] << 8;
        *youtput1++ = _ytable[p2] << 0 | _ytable[p3] << 8;
        *uoutput++ = (_utable[p0] + _utable[p1] + _utable[p2] + _utable[p3]) >> 2;
        *voutput++ = (_vtable[p0] + _vtable[p1] + _vtable[p2] + _vtable[p3]) >> 2;
      }
    }
  }

  inline auto toRGB15(u32 rgb32) const -> u16 {
    return ((rgb32 >> 9) & 0x7c00) + ((rgb32 >> 6) & 0x03e0) + ((rgb32 >> 3) & 0x001f);
  }

  inline auto toRGB16(u32 rgb32) const -> u16 {
    return ((rgb32 >> 8) & 0xf800) + ((rgb32 >> 5) & 0x07e0) + ((rgb32 >> 3) & 0x001f);
  }

  auto initializeTables() -> void {
    _ytable = new u8[65536];
    _utable = new u8[65536];
    _vtable = new u8[65536];

    for(u32 n : range(65536)) {
      //extract RGB565 color data from i
      u8 r = (n >> 11) & 31, g = (n >> 5) & 63, b = (n) & 31;
      r = (r << 3) | (r >> 2);  //R5->R8
      g = (g << 2) | (g >> 4);  //G6->G8
      b = (b << 3) | (b >> 2);  //B5->B8

      //ITU-R Recommendation BT.601
      //double lr = 0.299, lg = 0.587, lb = 0.114;
      s32 y = s32( +(f64(r) * 0.257) + (f64(g) * 0.504) + (f64(b) * 0.098) +  16.0 );
      s32 u = s32( -(f64(r) * 0.148) - (f64(g) * 0.291) + (f64(b) * 0.439) + 128.0 );
      s32 v = s32( +(f64(r) * 0.439) - (f64(g) * 0.368) - (f64(b) * 0.071) + 128.0 );

      //ITU-R Recommendation BT.709
      //f64 lr = 0.2126, lg = 0.7152, lb = 0.0722;
      //s32 y = s32( f64(r) * lr + f64(g) * lg + f64(b) * lb );
      //s32 u = s32( (f64(b) - y) / (2.0 - 2.0 * lb) + 128.0 );
      //s32 v = s32( (f64(r) - y) / (2.0 - 2.0 * lr) + 128.0 );

      _ytable[n] = y < 0 ? 0 : y > 255 ? 255 : y;
      _utable[n] = u < 0 ? 0 : u > 255 ? 255 : u;
      _vtable[n] = v < 0 ? 0 : v > 255 ? 255 : v;
    }
  }

  bool _ready = false;

  u32 _width = 0;
  u32 _height = 0;

  u32* _buffer = nullptr;
  u32 _bufferWidth = 0;
  u32 _bufferHeight = 0;

  u8* _ytable = nullptr;
  u8* _utable = nullptr;
  u8* _vtable = nullptr;

  Display* _display = nullptr;
  u32 _monitorX = 0;
  u32 _monitorY = 0;
  u32 _monitorWidth = 0;
  u32 _monitorHeight = 0;
  u32 _screen = 0;
  GC _gc = 0;
  Window _parent = 0;
  Window _window = 0;
  Colormap _colormap = 0;
  XShmSegmentInfo _shmInfo;

  s32 _port = -1;
  XvImage* _image = nullptr;

  vector<s32> _formatIDs;
  vector<string> _formatNames;

  s32 _formatID = 0;
  string _formatName;
};
