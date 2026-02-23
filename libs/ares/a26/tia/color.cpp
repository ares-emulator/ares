auto TIA::color(n32 color) -> n64 {
  u64 ntscColors[128] = {
    0x0000'0000'0000ull, 0x4040'4040'4040ull, 0x6c6c'6c6c'6c6cull, 0x9090'9090'9090ull, 0xb0b0'b0b0'b0b0ull, 0xc8c8'c8c8'c8c8ull, 0xdcdc'dcdc'dcdcull, 0xecec'ecec'ececull,
    0x4444'4444'0000ull, 0x6464'6464'1010ull, 0x8484'8484'2424ull, 0xa0a0'a0a0'3434ull, 0xb8b8'b8b8'4040ull, 0xd0d0'd0d0'5050ull, 0xe8e8'e8e8'5c5cull, 0xfcfc'fcfc'6868ull,
    0x7070'2828'0000ull, 0x8484'4444'1414ull, 0x9898'5c5c'2828ull, 0xacac'7878'3c4cull, 0xbcbc'8c8c'4c4cull, 0xcccc'a0a0'5c5cull, 0xdcdc'b4b4'6868ull, 0xecec'c8c8'7878ull,
    0x8484'1818'0000ull, 0x9898'3434'1818ull, 0xacac'5050'3030ull, 0xc0c0'6868'4848ull, 0xd0d0'8080'5c5cull, 0xe0e0'9494'7070ull, 0xecec'a8a8'8080ull, 0xfcfc'bcbc'9494ull,
    0x8888'0000'0000ull, 0x9c9c'2020'2020ull, 0xb0b0'3c3c'3c3cull, 0xc0c0'5858'5858ull, 0xd0d0'7070'7070ull, 0xe0e0'8888'8888ull, 0xecec'a0a0'a0a0ull, 0xfcfc'b4b4'b4b4ull,
    0x7878'0000'5c5cull, 0x8c8c'2020'7474ull, 0xa0a0'3c3c'8888ull, 0xb0b0'5858'9c9cull, 0xc0c0'7070'b0b0ull, 0xd0d0'8484'c0c0ull, 0xdcdc'9c9c'd0d0ull, 0xecec'b0b0'e0e0ull,
    0x4848'0000'7878ull, 0x6060'2020'9090ull, 0x7878'3c3c'a4a4ull, 0x8c8c'5858'b8b8ull, 0xa0a0'7070'ccccull, 0xb4b4'8484'dcdcull, 0xc4c4'9c9c'ececull, 0xd4d4'b0b0'fcfcull,
    0x1414'0000'8484ull, 0x3030'2020'9898ull, 0x4c4c'3c3c'acacull, 0x6868'5858'c0c0ull, 0x7c7c'7070'd0d0ull, 0x9494'8888'e0e0ull, 0xa8a8'a0a0'ececull, 0xbcbc'b4b4'fcfcull,
    0x0000'0000'8888ull, 0x1c1c'2020'9c9cull, 0x3838'4040'b0b0ull, 0x5050'5c5c'c0c0ull, 0x6868'7474'd0d0ull, 0x7c7c'8c8c'e0e0ull, 0x9090'a4a4'ececull, 0xa4a4'b8b8'fcfcull,
    0x0000'1818'7c7cull, 0x1c1c'3838'9090ull, 0x3838'5454'a8a8ull, 0x5050'7070'bcbcull, 0x6868'8888'ccccull, 0x7c7c'9c9c'dcdcull, 0x9090'b4b4'ececull, 0xa4a4'c8c8'fcfcull,
    0x0000'2c2c'5c5cull, 0x1c1c'4c4c'7878ull, 0x3838'6868'9090ull, 0x5050'8484'acacull, 0x6868'9c9c'c0c0ull, 0x7c7c'b4b4'd4d4ull, 0x9090'cccc'e8e8ull, 0xa4a4'e0e0'fcfcull,
    0x0000'3c3c'2c2cull, 0x1c1c'5c5c'4848ull, 0x3838'7c7c'6464ull, 0x5050'9c9c'8080ull, 0x6868'b4b4'9494ull, 0x7c7c'd0d0'acacull, 0x9090'e4e4'c0c0ull, 0xa4a4'fcfc'd4d4ull,
    0x0000'3c3c'0000ull, 0x2020'5c5c'2020ull, 0x4040'7c7c'4040ull, 0x5c5c'9c9c'5c5cull, 0x7474'b4b4'7474ull, 0x8c8c'd0d0'8c8cull, 0xa4a4'e4e4'a4a4ull, 0xb8b8'fcfc'b8b8ull,
    0x1414'3838'0000ull, 0x3434'5c5c'1c1cull, 0x5050'7c7c'3838ull, 0x6c6c'9898'5050ull, 0x8484'b4b4'6868ull, 0x9c9c'cccc'7c7cull, 0xb4b4'e4e4'9090ull, 0xc8c8'fcfc'a4a4ull,
    0x2c2c'3030'0000ull, 0x4c4c'5050'1c1cull, 0x6868'7070'3434ull, 0x8484'8c8c'4c4cull, 0x9c9c'a8a8'6464ull, 0xb4b4'c0c0'7878ull, 0xcccc'd4d4'8888ull, 0xe0e0'ecec'9c9cull,
    0x4444'2828'0000ull, 0x6464'4848'1818ull, 0x8484'6868'3030ull, 0xa0a0'8484'4444ull, 0xb8b8'9c9c'5858ull, 0xd0d0'b4b4'6c6cull, 0x8e8c'cccc'7c7cull, 0xfcfc'c0c0'8c8cull,
  };

  u64 palColors[128] = {
    0x0000'0000'0000ull, 0x2828'2828'2828ull, 0x5050'5050'5050ull, 0x7474'7474'7474ull, 0x9494'9494'9494ull, 0xb4b4'b4b4'b4b4ull, 0xd0d0'd0d0'd0d0ull, 0xecec'ecec'ececull,
    0x0000'0000'0000ull, 0x2828'2828'2828ull, 0x5050'5050'5050ull, 0x7474'7474'7474ull, 0x9494'9494'9494ull, 0xb4b4'b4b4'b4b4ull, 0xd0d0'd0d0'd0d0ull, 0xecec'ecec'ececull,
    0x8080'5858'0000ull, 0x9494'7070'2020ull, 0xa8a8'8484'3c3cull, 0xbcbc'9c9c'5858ull, 0xcccc'acac'7070ull, 0xdcdc'c0c0'8484ull, 0xecec'd0d0'9c9cull, 0xfcfc'e0e0'b0b0ull,
    0x4444'5c5c'0000ull, 0x5c5c'7878'2020ull, 0x7474'9090'3c3cull, 0x8c8c'acac'5858ull, 0xa0a0'c0c0'7070ull, 0xb0b0'd4d4'8484ull, 0xc4c4'e8e8'9c9cull, 0xd4d4'fcfc'b0b0ull,
    0x7070'3434'0000ull, 0x8888'5050'2020ull, 0xa0a0'6868'3c3cull, 0xb4b4'8484'5858ull, 0xc8c8'9898'7070ull, 0xdcdc'acac'8484ull, 0xecec'c0c0'9c9cull, 0xfcfc'd4d4'b0b0ull,
    0x0000'6464'1414ull, 0x2020'8080'3434ull, 0x3c3c'9898'5050ull, 0x5858'b0b0'6c6cull, 0x7070'c4c4'8484ull, 0x8484'd8d8'9c9cull, 0x9c9c'e8e8'b4b4ull, 0xb0b0'fcfc'c8c8ull,
    0x7070'0000'1414ull, 0x8888'2020'3434ull, 0xa0a0'3c3c'5050ull, 0xb4b4'5858'6c6cull, 0xc8c8'7070'8484ull, 0xdcdc'8484'9c9cull, 0xecec'9c9c'b4b4ull, 0xfcfc'b0b0'c8c8ull,
    0x0000'5c5c'5c5cull, 0x2020'7474'7474ull, 0x3c3c'8c8c'8c8cull, 0x5858'a4a4'a4a4ull, 0x7070'b8b8'b8b8ull, 0x8484'c8c8'c8c8ull, 0x9c9c'dcdc'dcdcull, 0xb0b0'ecec'ececull,
    0x7070'0000'5c5cull, 0x8484'2020'7474ull, 0x9494'3c3c'8888ull, 0xa8a8'5858'9c9cull, 0xb4b4'7070'b0b0ull, 0xc4c4'8484'c0c0ull, 0xd0d0'9c9c'd0d0ull, 0xe0e0'b0b0'e0e0ull,
    0x0000'3c3c'7070ull, 0x1c1c'5858'8888ull, 0x3838'7474'a0a0ull, 0x5050'8c8c'b4b4ull, 0x6868'a4a4'c8c8ull, 0x7c7c'b8b8'dcdcull, 0x9090'cccc'ececull, 0xa4a4'e0e0'fcfcull,
    0x5858'0000'7070ull, 0x6c6c'2020'8888ull, 0x8080'3c3c'a0a0ull, 0x9494'5858'b4b4ull, 0xa4a4'7070'c8c8ull, 0xb4b4'8484'dcdcull, 0xc4c4'9c9c'ececull, 0xd4d4'b0b0'fcfcull,
    0x0000'2020'7070ull, 0x1c1c'3c3c'8888ull, 0x3838'5858'a0a0ull, 0x5050'7474'b4b4ull, 0x6868'8888'c8c8ull, 0x7c7c'a0a0'dcdcull, 0x9090'b4b4'ececull, 0xa4a4'c8c8'fcfcull,
    0x3c3c'0000'8080ull, 0x5454'2020'9494ull, 0x6c6c'3c3c'a8a8ull, 0x8080'5858'bcbcull, 0x9494'7070'ccccull, 0xa8a8'8484'dcdcull, 0xb8b8'9c9c'ececull, 0xc8c8'b0b0'fcfcull,
    0x0000'0000'8888ull, 0x2020'2020'9c9cull, 0x3c3c'3c3c'b0b0ull, 0x5858'5858'c0c0ull, 0x7070'7070'd0d0ull, 0x8484'8484'e0e0ull, 0x9c9c'9c9c'ececull, 0xb0b0'b0b0'fcfcull,
    0x0000'0000'0000ull, 0x2828'2828'2828ull, 0x5050'5050'5050ull, 0x7474'7474'7474ull, 0x9494'9494'9494ull, 0xb4b4'b4b4'b4b4ull, 0xd0d0'd0d0'd0d0ull, 0xecec'ecec'ececull,
    0x0000'0000'0000ull, 0x2828'2828'2828ull, 0x5050'5050'5050ull, 0x7474'7474'7474ull, 0x9494'9494'9494ull, 0xb4b4'b4b4'b4b4ull, 0xd0d0'd0d0'd0d0ull, 0xecec'ecec'ececull,
  };

  return Region::PAL() ? palColors[color & 0x7f] : ntscColors[color & 0x7f];
}

