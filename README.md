# ares

**ares** is a multi-system emulator that began development on October 14th, 2004.
It is a descendent of higan and bsnes, and focuses on accuracy and preservation.

Official Releases
-----------------

Official releases are available from
[the GitHub releases page](https://github.com/higan-emu/ares/releases).

Nightly Builds
--------------

Automated, untested builds of ares are available for Windows and macOS as a [pre-release](https://github.com/higan-emu/ares/releases/tag/nightly). 
Only the latest nightly build is kept.

Features
--------

* Native multi-platform UI
* Adaptive sync
* Dynamic rate control
* Save states
* Run-ahead
* Rewind and fast-forward
* Pixel shaders
* Color correction
* 6th-order IIR audio filtering
* Input multi-mapping
* Built-in games database
* Debugger

Systems:
--------
ares currently emulates the following 27 hardware devices:

<table>
<tbody><tr>
  <th>Status</th>
  <th>Name</th>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/famicom_gimmick.png">Famicom</a> + <a href="docs/images/gallery/famicom-disk-system_zelda.png">Famicom Disk System</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""></td>
  <td><a href="docs/images/gallery/super-famicom_bahamut-lagoon.png">Super Famicom</a> + <a href="docs/images/gallery/super-game-boy_devichil-black-book.png">Super Game Boy</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/nintendo-64_zelda-ocarina-of-time.png">Nintendo 64</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/game-boy_links-awakening.png">Game Boy</a> + <a href="docs/images/gallery/game-boy-color_devichil-white-book.png">Game Boy Color</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/game-boy-advance_golden-sun.png">Game Boy Advance</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/sg-1000_ninja-princess.png">SG-1000</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""></td>
  <td><a href="docs/images/gallery/master-system_wonder-boy-iii.png">Master System</a> + <a href="docs/images/gallery/game-gear_sonic.png">Game Gear</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/mega-drive_sonic-3.png">Mega Drive</a> + <a href="docs/images/gallery/mega-32x_chaotix.png">Mega 32X</a> + <a href="docs/images/gallery/mega-cd_lunar-silver-star-small.png">Mega CD</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/playstation_wild-arms.png">PlayStation</a>
</td></tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/pc-engine_bomberman-94.png">PC Engine</a> + <a href="docs/images/gallery/pc-engine-cd_rondo-of-blood.png">PC Engine CD</a> + <a href="docs/images/gallery/supergrafx_daimakaimura.png">SuperGrafx</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/neo-geo-aes_metal-slug.png">Neo Geo AES</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/msx_parodius.png">MSX</a> + <a href="docs/images/gallery/msx2_akumajou-dracula.png">MSX2</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/colecovision_frogger.png">ColecoVision</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/starless-small.png" alt=""></td>
  <td><a href="docs/images/gallery/neo-geo-pocket_samurai-shodown.png">Neo Geo Pocket</a> + <a href="docs/images/gallery/neo-geo-pocket-color_last-blade.png">Neo Geo Pocket Color</a></td>
</tr>
<tr>
  <td><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""><img src="docs/images/star-small.png" alt=""></td>
  <td><a href="docs/images/gallery/wonderswan_langrisser.png">WonderSwan</a> + <a href="docs/images/gallery/wonderswan-color_riviera.png">WonderSwan Color</a> + <a href="docs/images/gallery/pocket-challenge-v2_sck1.png">Pocket Challenge V2</a></td>
</tr>
</tbody></table>

