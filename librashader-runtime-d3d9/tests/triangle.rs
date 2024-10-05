mod hello_triangle;

const FILTER_PATH: &str = "../test/shaders_slang/crt/crt-geom.slangp";

#[test]
fn triangle_d3d9() {
    let sample = hello_triangle::d3d9_hello_triangle::Sample::new(
        FILTER_PATH,
        // Some(&FilterChainOptionsD3D9 {
        //     force_no_mipmaps: false,
        //     disable_cache: false,
        // }),
        // replace below with 'None' for the triangle
        // None,
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
