mod hello_triangle;

use hello_triangle::vulkan_base::VulkanBase;
use librashader_runtime_vk::options::FilterChainOptionsVulkan;
use librashader_runtime_vk::FilterChainVulkan;

#[test]
fn triangle_vk() {
    let entry = unsafe { ash::Entry::load().unwrap() };
    let base = VulkanBase::new(entry).unwrap();

    unsafe {
        let filter = FilterChainVulkan::load_from_path(
            "../test/shaders_slang/test/feedback.slangp",
            // "../test/shaders_slang/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp",
            // "../test/Mega_Bezel_Packs/Duimon-Mega-Bezel/Presets/Advanced/Nintendo_GBA_SP/GBA_SP-[ADV]-[LCD-GRID]-[Night].slangp",
            &base,
            // "../test/basic.slangp",
            Some(&FilterChainOptionsVulkan {
                frames_in_flight: 3,
                force_no_mipmaps: false,
                use_dynamic_rendering: false,
                disable_cache: true,
            }),
        )
        .unwrap();

        hello_triangle::main(base, filter)
    }

    // let base = hello_triangle_old::ExampleBase::new(900, 600);
    // // let mut filter = FilterChainVulkan::load_from_path(
    // //     (base.device.clone(), base.present_queue.clone(), base.device_memory_properties.clone()),
    // //     "../test/slang-shaders/border/gameboy-player/gameboy-player-crt-royale.slangp",
    // //     None
    // // )
    //
    // let mut filter = FilterChainVulkan::load_from_path(
    //     (
    //         base.device.clone(),
    //         base.present_queue.clone(),
    //         base.device_memory_properties.clone(),
    //     ),
    //     "../test/slang-shaders/border/gameboy-player/gameboy-player-crt-royale.slangp",
    //     None,
    // )
    // // FilterChain::load_from_path("../test/slang-shaders/bezel/Mega_Bezel/Presets/MBZ__0__SMOOTH-ADV.slangp", None)
    // .unwrap();
    // hello_triangle_old::main(base, filter);
}
