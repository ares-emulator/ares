# MAME software-list name -> ares board name.
SOFTWARE_LISTS = {
    "neogeo": "snk/neogeo",
}

DRIVERS = [
    "sega/sg1000a.cpp",
    "sega/segae.cpp",
    "nintendo/aleck64.cpp",
    "snk/neogeo.cpp",
]

# Extra source files needed for input definitions and ROM_COPY records.
SOURCE_FILES = {
    "snk/neogeo": [
        "src/mame/snk/neogeo.cpp",
        "src/devices/bus/neogeo_ctrl/joystick.cpp",
    ],
    "sega/segae": [
        "src/mame/sega/segae.cpp",
    ],
    "sega/sg1000a": [
        "src/mame/sega/sg1000a.cpp",
    ],
    "nintendo/aleck64": [
        "src/mame/nintendo/aleck64.cpp",
    ],
}
