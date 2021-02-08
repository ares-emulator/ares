struct PPU : Thread, IO {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean colorEmulation;
  Node::Setting::Boolean interframeBlending;
  Node::Setting::String orientation;
  Node::Setting::Boolean showIcons;
  struct Icon {
    Node::Video::Sprite auxiliary0;
    Node::Video::Sprite auxiliary1;
    Node::Video::Sprite auxiliary2;
    Node::Video::Sprite headphones;
    Node::Video::Sprite initialized;
    Node::Video::Sprite lowBattery;
    Node::Video::Sprite orientation0;
    Node::Video::Sprite orientation1;
    Node::Video::Sprite poweredOn;
    Node::Video::Sprite sleeping;
    Node::Video::Sprite volumeA0;
    Node::Video::Sprite volumeA1;
    Node::Video::Sprite volumeA2;
    Node::Video::Sprite volumeB0;
    Node::Video::Sprite volumeB1;
    Node::Video::Sprite volumeB2;
    Node::Video::Sprite volumeB3;
  } icon;

  auto planar() const -> bool { return system.mode().bit(0) == 0; }
  auto packed() const -> bool { return system.mode().bit(0) == 1; }
  auto depth() const -> u32 { return system.mode().bit(1,2) != 3 ? 2 : 4; }
  auto grayscale() const -> bool { return system.mode().bit(1,2) == 0; }
  auto tilemask() const -> u32 { return 1023 >> !system.mode().bit(2); }

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto scanline() -> void;
  auto frame() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;
  auto updateIcons() -> void;
  auto updateOrientation() -> void;

  //io.cpp
  auto portRead(n16 address) -> n8 override;
  auto portWrite(n16 address, n8 data) -> void override;

  //latch.cpp
  auto latchRegisters() -> void;
  auto latchSprites(n8 y) -> void;
  auto latchOAM() -> void;

  //render.cpp
  auto renderFetch(n10 tile, n3 x, n3 y) -> n4;
  auto renderTransparent(bool palette, n4 color) -> bool;
  auto renderPalette(n4 palette, n4 color) -> n12;
  auto renderBack() -> void;
  auto renderScreenOne(n8 x, n8 y) -> void;
  auto renderScreenTwo(n8 x, n8 y) -> void;
  auto renderSprite(n8 x, n8 y) -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //state
  struct Pixel {
    enum class Source : u32 { Back, ScreenOne, ScreenTwo, Sprite };
    Source source;
    n12 color;
  };

  struct State {
    n1 field = 0;
    n8 vtime = 0;
    Pixel pixel;
  } s;

  struct Latches {
    //latchRegisters()
    n8 backColor;

    n1 screenOneEnable;
    n4 screenOneMapBase;
    n8 scrollOneX;
    n8 scrollOneY;

    n1 screenTwoEnable;
    n4 screenTwoMapBase;
    n8 scrollTwoX;
    n8 scrollTwoY;
    n1 screenTwoWindowEnable;
    n1 screenTwoWindowInvert;
    n8 screenTwoWindowX0;
    n8 screenTwoWindowY0;
    n8 screenTwoWindowX1;
    n8 screenTwoWindowY1;

    n1 spriteEnable;
    n1 spriteWindowEnable;
    n8 spriteWindowX0;
    n8 spriteWindowY0;
    n8 spriteWindowX1;
    n8 spriteWindowY1;

    //latchSprites()
    n32 sprite[32];
    u32 spriteCount = 0;

    //latchOAM()
    n32 oam[2][128];
    u32 oamCount = 0;

    //updateOrientation()
    n1 orientation;
  } l;

  struct Registers {
    //$0000  DISP_CTRL
    n1 screenOneEnable;
    n1 screenTwoEnable;
    n1 spriteEnable;
    n1 spriteWindowEnable;
    n1 screenTwoWindowInvert;
    n1 screenTwoWindowEnable;

    //$0001  BACK_COLOR
    n8 backColor;

    //$0003  LINE_CMP
    n8 lineCompare;

    //$0004  SPR_BASE
    n6 spriteBase;

    //$0005  SPR_FIRST
    n7 spriteFirst;

    //$0006  SPR_COUNT
    n8 spriteCount;  //0 - 128

    //$0007  MAP_BASE
    n4 screenOneMapBase;
    n4 screenTwoMapBase;

    //$0008  SCR2_WIN_X0
    n8 screenTwoWindowX0;

    //$0009  SCR2_WIN_Y0
    n8 screenTwoWindowY0;

    //$000a  SCR2_WIN_X1
    n8 screenTwoWindowX1;

    //$000b  SCR2_WIN_Y1
    n8 screenTwoWindowY1;

    //$000c  SPR_WIN_X0
    n8 spriteWindowX0;

    //$000d  SPR_WIN_Y0
    n8 spriteWindowY0;

    //$000e  SPR_WIN_X1
    n8 spriteWindowX1;

    //$000f  SPR_WIN_Y1
    n8 spriteWindowY1;

    //$0010  SCR1_X
    n8 scrollOneX;

    //$0011  SCR1_Y
    n8 scrollOneY;

    //$0012  SCR2_X
    n8 scrollTwoX;

    //$0013  SCR2_Y
    n8 scrollTwoY;

    //$0014  LCD_CTRL
    n1 lcdEnable;
    n1 lcdContrast;  //0 = low, 1 = high (WonderSwan Color only)
    n8 lcdUnknown;

    //$0015  LCD_ICON
    struct Icon {
      n1 sleeping;
      n1 orientation1;
      n1 orientation0;
      n1 auxiliary0;
      n1 auxiliary1;
      n1 auxiliary2;
    } icon;

    //$0016  LCD_VTOTAL
    n8 vtotal = 158;

    //$0017  LCD_VSYNC
    n8 vsync = 155;

    //$001c-001f  PALMONO_POOL
    n4 pool[8];

    //$0020-003f  PALMONO
    struct Palette {
      n3 color[4];
    } palette[16];

    //$00a2  TMR_CTRL
    n1 htimerEnable;
    n1 htimerRepeat;
    n1 vtimerEnable;
    n1 vtimerRepeat;

    //$00a4,$00a5  HTMR_FREQ
    n16 htimerFrequency;

    //$00a6,$00a7  VTMR_FREQ
    n16 vtimerFrequency;

    //$00a8,$00a9  HTMR_CTR
    n16 htimerCounter;

    //$00aa,$00ab  VTMR_CTR
    n16 vtimerCounter;
  } r;
};

extern PPU ppu;
