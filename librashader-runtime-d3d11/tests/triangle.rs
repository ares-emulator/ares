mod hello_triangle;

use librashader_runtime::image::{Image, UVDirection};
use librashader_runtime_d3d11::options::FilterChainOptionsD3D11;

// "../test/slang-shaders/scalefx/scalefx-9x.slangp",
// "../test/slang-shaders/bezel/koko-aio/monitor-bloom.slangp",
// "../test/slang-shaders/presets/crt-geom-ntsc-upscale-sharp.slangp",
// const FILTER_PATH: &str =
//     "../test/slang-shaders/handheld/console-border/gbc-lcd-grid-v2.slangp";
// "../test/null.slangp",
// const FILTER_PATH: &str =
//     "../test/Mega_Bezel_Packs/Duimon-Mega-Bezel/Presets/Advanced/Nintendo_GBA_SP/GBA_SP-[ADV]-[LCD-GRID].slangp";

const FILTER_PATH: &str =
    "../test/shaders_slang/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp";

// const FILTER_PATH: &str = "../test/slang-shaders/test/history.slangp";
// const FILTER_PATH: &str = "../test/slang-shaders/test/feedback.slangp";

// const FILTER_PATH: &str = "../test/slang-shaders/crt/crt-royale.slangp";
const IMAGE_PATH: &str = "../triangle.png";
#[test]
fn triangle_d3d11_args() {
    let mut args = std::env::args();
    let _ = args.next();
    let _ = args.next();
    let filter = args.next();
    let image = args
        .next()
        .and_then(|f| Image::load(f, UVDirection::TopLeft).ok())
        .or_else(|| Some(Image::load(IMAGE_PATH, UVDirection::TopLeft).unwrap()))
        .unwrap();

    let sample = hello_triangle::d3d11_hello_triangle::Sample::new(
        filter.as_deref().unwrap_or(FILTER_PATH),
        Some(&FilterChainOptionsD3D11 {
            force_no_mipmaps: false,
            disable_cache: false,
        }),
        // replace below with 'None' for the triangle
        Some(image),
    )
    .unwrap();
    // let sample = hello_triangle_old::d3d11_hello_triangle::Sample::new(
    //     "../test/slang-shaders/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp",
    //     Some(&FilterChainOptions {
    //         use_deferred_context: true,
    //     })
    // )
    // .unwrap();

    // let sample = hello_triangle_old::d3d11_hello_triangle::Sample::new("../test/basic.slangp").unwrap();

    hello_triangle::main(sample).unwrap();
}

#[test]
fn triangle_d3d11() {
    let sample = hello_triangle::d3d11_hello_triangle::Sample::new(
        FILTER_PATH,
        Some(&FilterChainOptionsD3D11 {
            force_no_mipmaps: false,
            disable_cache: false,
        }),
        // replace below with 'None' for the triangle
        // None,
        Some(Image::load(IMAGE_PATH, UVDirection::TopLeft).unwrap()),
    )
    .unwrap();
    // let sample = hello_triangle_old::d3d11_hello_triangle::Sample::new(
    //     "../test/slang-shaders/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp",
    //     Some(&FilterChainOptions {
    //         use_deferred_context: true,
    //     })
    // )
    // .unwrap();

    // let sample = hello_triangle_old::d3d11_hello_triangle::Sample::new("../test/basic.slangp").unwrap();

    hello_triangle::main(sample).unwrap();
}
