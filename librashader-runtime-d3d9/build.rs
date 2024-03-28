pub fn main() {
    #[cfg(all(target_os = "windows"))]
    {
        // println!("cargo:rustc-link-arg=/DELAYLOAD:D3DX9_43.dll");
    }
}
