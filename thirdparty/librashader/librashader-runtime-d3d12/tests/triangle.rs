mod hello_triangle;

use crate::hello_triangle::{DXSample, SampleCommandLine};

#[test]
fn triangle_d3d12() {
    let sample = hello_triangle::d3d12_hello_triangle::Sample::new(
        "../test/shaders_slang/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp",
        // "../test/shaders_slang/crt/crt-lottes.slangp",
        // "../test/basic.slangp",
        // "../test/shaders_slang/handheld/console-border/gbc-lcd-grid-v2.slangp",
        // "../test/Mega_Bezel_Packs/Duimon-Mega-Bezel/Presets/Advanced/Nintendo_GBA_SP/GBA_SP-[ADV]-[LCD-GRID]-[Night].slangp",
        // "../test/slang-shaders/test/feedback.slangp",
        // "../test/slang-shaders/test/history.slangp",
        // "../test/shaders_slang/crt/crt-royale.slangp",
        // "../test/slang-shaders/vhs/VHSPro.slangp",
        &SampleCommandLine {
            use_warp_device: false,
        },
    )
    .unwrap();
    hello_triangle::main(sample).unwrap()
}
