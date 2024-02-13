// librashader-capi-tests.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#define LIBRA_RUNTIME_VULKAN
#define LIBRA_RUNTIME_OPENGL
#define LIBRA_RUNTIME_D3D11
#define LIBRA_RUNTIME_D3D12


#include "../../../../include/librashader_ld.h"
int main()
{
    std::cout << "Hello World!\n";
    std::cout << std::filesystem::current_path() << std::endl;
    auto instance = librashader_load_instance();

    libra_preset_ctx_t context;

    instance.preset_ctx_create(&context);
    instance.preset_ctx_set_core_name(&context, "Hello");

    libra_shader_preset_t preset;
    auto error = instance.preset_create_with_context(
        "../../../shaders_slang/border/gameboy-player/"
        "gameboy-player-crt-royale.slangp",
        &context,
        &preset);

   /* libra_shader_preset_t preset2;
    libra_preset_create(
        "../../../slang-shaders/border/gameboy-player/"
        "gameboy-player-crt-royale.slangp",
        &preset2);*/
    
    instance.preset_print(&preset);
    std::cout << "printed\n";
    libra_preset_param_list_t parameters;
    error = instance.preset_get_runtime_params(&preset, &parameters);

    libra_preset_param_t next = parameters.parameters[471];

    instance.preset_free_runtime_params(parameters);

    /*libra_shader_preset_t preset;
    auto error = libra_preset_create("../../../slang-shaders/border/gameboy-player/gameboy-player-crt-royale.slangp", &preset);
    if (error != NULL) {
        std::cout << "error happened\n";
    }
    libra_preset_print(&preset);
    
    libra_gl_filter_chain_t chain;

    if (error != NULL) {
        libra_error_print(error);
        char* error_str;
        libra_error_write(error, &error_str);
        printf("%s", error_str);
        libra_error_free_string(&error_str);
        printf("%s", error_str);
    }*/
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
