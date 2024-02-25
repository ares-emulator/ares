pub fn main() {
    #[cfg(all(target_os = "windows", feature = "runtime-d3d12"))]
    {
        println!("cargo:rustc-link-lib=dylib=delayimp");
        println!("cargo:rustc-link-arg=/DELAYLOAD:dxcompiler.dll");
        println!("cargo:rustc-link-arg=/DELAYLOAD:d3d12.dll");
    }
}
