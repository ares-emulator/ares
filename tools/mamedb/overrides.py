"""ROM layouts missing from MAME machine XML. Software-list layouts take priority."""

from dataclasses import dataclass
from typing import Tuple


@dataclass(frozen=True)
class Layout:
    type: str
    sizes: Tuple[int, ...]


# Program replacements whose wiring matches the XML parent's software layout.
ROM_REPLACEMENTS = {
    "mslug2t": {"241-p1.p1": "941-p1.p1"},  # Only PROM differs
}


# MAME -listxml omits word swapping, byte interleaving and ROM_CONTINUE.
# Layout reference: MAME src/mame/snk/neogeo.cpp, corresponding ROM_START blocks.
ROM_LAYOUTS = {}

# All V-Liner revisions use the same mapping.
for name in ("vliner", "vliner53", "vliner54", "vliner6e", "vliner7e"):
    ROM_LAYOUTS[name] = {
        "cslot1-maincpu": Layout("load16_word_swap", (0x80000,)),
        "cslot1-sprites": Layout("load16_byte", (0x80000,) * 2),
    }
