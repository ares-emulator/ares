#pragma once

struct InputMouseXlib {
  Input& input;
  InputMouseXlib(Input& input) : input(input) {}

  shared_pointer<HID::Mouse> hid{new HID::Mouse};

  uintptr handle = 0;

  Display* display = nullptr;
  Window rootWindow = 0;
  Cursor invisibleCursor = 0;
  u32 screenWidth = 0;
  u32 screenHeight = 0;

  struct Mouse {
    bool acquired = false;
    s32 numerator = 0;
    s32 denominator = 0;
    s32 threshold = 0;
    u32 relativeX = 0;
    u32 relativeY = 0;
    bool ignoreNextCenter = false;
  } ms;

  auto acquired() -> bool {
    return ms.acquired;
  }

  auto acquire() -> bool {
    if(acquired()) return true;

    if(XGrabPointer(display, handle, True, 0, GrabModeAsync, GrabModeAsync, rootWindow, invisibleCursor, CurrentTime) == GrabSuccess) {
      //backup existing cursor acceleration settings
      XGetPointerControl(display, &ms.numerator, &ms.denominator, &ms.threshold);

      //disable cursor acceleration
      XChangePointerControl(display, True, False, 1, 1, 0);

      //center cursor (so that first relative poll returns 0, 0 if mouse has not moved)
      XWarpPointer(display, None, rootWindow, 0, 0, 0, 0, screenWidth / 2, screenHeight / 2);

      return ms.acquired = true;
    } else {
      return ms.acquired = false;
    }
  }

  auto release() -> bool {
    if(acquired()) {
      //restore cursor acceleration and release cursor
      XChangePointerControl(display, True, True, ms.numerator, ms.denominator, ms.threshold);
      XUngrabPointer(display, CurrentTime);
      ms.acquired = false;
    }
    return true;
  }

  auto assign(u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(vector<shared_pointer<HID::Device>>& devices) -> void {
    Window rootReturn;
    Window childReturn;
    s32 rootXReturn = 0;
    s32 rootYReturn = 0;
    s32 windowXReturn = 0;
    s32 windowYReturn = 0;
    u32 maskReturn = 0;
    XQueryPointer(display, handle, &rootReturn, &childReturn, &rootXReturn, &rootYReturn, &windowXReturn, &windowYReturn, &maskReturn);

    if(acquired()) {
      XWindowAttributes attributes;
      XGetWindowAttributes(display, handle, &attributes);

      if (!ms.ignoreNextCenter || (rootXReturn != screenWidth / 2) || (rootYReturn != screenHeight / 2)) {
        //absolute -> relative conversion
        assign(HID::Mouse::GroupID::Axis, 0, (s16)(rootXReturn - screenWidth  / 2));
        assign(HID::Mouse::GroupID::Axis, 1, (s16)(rootYReturn - screenHeight / 2));

        if(hid->axes().input(0).value() != 0 || hid->axes().input(1).value() != 0) {
          //if mouse moved, re-center mouse for next poll
          XWarpPointer(display, None, rootWindow, 0, 0, 0, 0, screenWidth / 2, screenHeight / 2);

          // XWarpPointer generates an event and results in this method getting called
          // again pretty soon, but the second time there is no movement (since the mouse is centered)
          // and the initial relative motion values are replaced by zeros.
          //
          // If the above occurs before the controller code has copied the value, the motion never
          // reach the game.
          //
          // This flag is used to workaround the above issue.
          ms.ignoreNextCenter = true;
        }
      } else {
        ms.ignoreNextCenter = false;
      }
    } else {
      assign(HID::Mouse::GroupID::Axis, 0, (s16)(rootXReturn - ms.relativeX));
      assign(HID::Mouse::GroupID::Axis, 1, (s16)(rootYReturn - ms.relativeY));

      ms.relativeX = rootXReturn;
      ms.relativeY = rootYReturn;
    }

    assign(HID::Mouse::GroupID::Button, 0, (bool)(maskReturn & Button1Mask));
    assign(HID::Mouse::GroupID::Button, 1, (bool)(maskReturn & Button2Mask));
    assign(HID::Mouse::GroupID::Button, 2, (bool)(maskReturn & Button3Mask));
    assign(HID::Mouse::GroupID::Button, 3, (bool)(maskReturn & Button4Mask));
    assign(HID::Mouse::GroupID::Button, 4, (bool)(maskReturn & Button5Mask));

    devices.append(hid);
  }

  auto initialize(uintptr handle) -> bool {
    terminate();
    if(!handle) return false;

    this->handle = handle;
    display = XOpenDisplay(0);
    rootWindow = DefaultRootWindow(display);

    XWindowAttributes attributes;
    XGetWindowAttributes(display, rootWindow, &attributes);
    screenWidth = attributes.width;
    screenHeight = attributes.height;

    //Xlib: because XShowCursor(display, false) would just be too easy
    //create invisible cursor for use when mouse is acquired
    Pixmap pixmap;
    XColor black, unused;
    static char invisibleData[8] = {0};
    Colormap colormap = DefaultColormap(display, DefaultScreen(display));
    XAllocNamedColor(display, colormap, "black", &black, &unused);
    pixmap = XCreateBitmapFromData(display, handle, invisibleData, 8, 8);
    invisibleCursor = XCreatePixmapCursor(display, pixmap, pixmap, &black, &black, 0, 0);
    XFreePixmap(display, pixmap);
    XFreeColors(display, colormap, &black.pixel, 1, 0);

    ms.acquired = false;
    ms.relativeX = 0;
    ms.relativeY = 0;

    hid->setVendorID(HID::Mouse::GenericVendorID);
    hid->setProductID(HID::Mouse::GenericProductID);
    hid->setPathID(0);

    hid->axes().append("X");
    hid->axes().append("Y");

    hid->buttons().append("Left");
    hid->buttons().append("Middle");
    hid->buttons().append("Right");
    hid->buttons().append("Up");
    hid->buttons().append("Down");

    return true;
  }

  auto terminate() -> void {
    release();
    if(invisibleCursor) {
      XFreeCursor(display, invisibleCursor);
      invisibleCursor = 0;
    }
    if(display) {
      XCloseDisplay(display);
      display = nullptr;
    }
  }
};
