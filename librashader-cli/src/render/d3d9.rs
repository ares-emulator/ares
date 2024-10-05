use crate::render::{CommonFrameOptions, RenderTest};
use anyhow::anyhow;
use image::RgbaImage;
use librashader::presets::ShaderPreset;
use librashader::runtime::d3d9::{FilterChain, FilterChainOptions, FrameOptions};
use librashader::runtime::Viewport;
use librashader::runtime::{FilterChainParameters, RuntimeParameters};
use librashader_runtime::image::{Image, PixelFormat, UVDirection, BGRA8};
use std::path::Path;
use windows::Win32::Foundation::{HWND, TRUE};
use windows::Win32::Graphics::Direct3D9::{
    Direct3DCreate9, IDirect3D9, IDirect3DDevice9, IDirect3DTexture9, D3DADAPTER_DEFAULT,
    D3DCREATE_HARDWARE_VERTEXPROCESSING, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8, D3DLOCKED_RECT,
    D3DPOOL_DEFAULT, D3DPOOL_MANAGED, D3DPOOL_SYSTEMMEM, D3DPRESENT_INTERVAL_IMMEDIATE,
    D3DPRESENT_PARAMETERS, D3DSURFACE_DESC, D3DSWAPEFFECT_DISCARD, D3DUSAGE_RENDERTARGET,
    D3D_SDK_VERSION,
};

pub struct Direct3D9 {
    pub texture: IDirect3DTexture9,
    pub image: Image<BGRA8>,
    pub direct3d: IDirect3D9,
    pub device: IDirect3DDevice9,
}

impl RenderTest for Direct3D9 {
    fn new(path: &Path) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        Direct3D9::new(path)
    }

    fn render_with_preset_and_params(
        &mut self,
        preset: ShaderPreset,
        frame_count: usize,
        param_setter: Option<&dyn Fn(&RuntimeParameters)>,
        frame_options: Option<CommonFrameOptions>,
    ) -> anyhow::Result<image::RgbaImage> {
        unsafe {
            let mut filter_chain = FilterChain::load_from_preset(
                preset,
                &self.device,
                Some(&FilterChainOptions {
                    force_no_mipmaps: false,
                    disable_cache: false,
                }),
            )?;

            if let Some(setter) = param_setter {
                setter(filter_chain.parameters());
            }

            let mut render_texture = None;

            self.device.CreateTexture(
                self.image.size.width,
                self.image.size.height,
                1,
                D3DUSAGE_RENDERTARGET as u32,
                D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT,
                &mut render_texture,
                std::ptr::null_mut(),
            )?;

            let render_texture = render_texture
                .ok_or_else(|| anyhow!("Unable to create Direct3D 9 render texture"))?;

            let mut copy_texture = None;

            self.device.CreateOffscreenPlainSurface(
                self.image.size.width,
                self.image.size.height,
                D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM,
                &mut copy_texture,
                std::ptr::null_mut(),
            )?;

            let copy_texture =
                copy_texture.ok_or_else(|| anyhow!("Unable to create Direct3D 9 copy texture"))?;

            let surface = render_texture.GetSurfaceLevel(0)?;

            let options = frame_options.map(|options| FrameOptions {
                clear_history: options.clear_history,
                frame_direction: options.frame_direction,
                rotation: options.rotation,
                total_subframes: options.total_subframes,
                current_subframe: options.current_subframe,
            });

            let viewport = Viewport::new_render_target_sized_origin(&surface, None)?;

            for frame in 0..=frame_count {
                filter_chain.frame(&self.texture, &viewport, frame, options.as_ref())?;
            }

            self.device.GetRenderTargetData(&surface, &copy_texture)?;

            let mut desc = D3DSURFACE_DESC::default();
            surface.GetDesc(&mut desc)?;

            let mut lock = D3DLOCKED_RECT::default();
            copy_texture.LockRect(&mut lock, std::ptr::null_mut(), 0)?;
            let mut buffer = vec![0u8; desc.Height as usize * lock.Pitch as usize];

            std::ptr::copy_nonoverlapping(lock.pBits.cast(), buffer.as_mut_ptr(), buffer.len());
            copy_texture.UnlockRect()?;

            BGRA8::convert(&mut buffer);

            let image = RgbaImage::from_raw(self.image.size.width, self.image.size.height, buffer)
                .ok_or(anyhow!("Unable to create image from data"))?;

            Ok(image)
        }
    }
}

impl Direct3D9 {
    pub fn new(image_path: impl AsRef<Path>) -> anyhow::Result<Self> {
        let direct3d = unsafe {
            Direct3DCreate9(D3D_SDK_VERSION)
                .ok_or_else(|| anyhow!("Unable to create Direct3D 9 device"))?
        };

        let image = Image::<BGRA8>::load(image_path, UVDirection::TopLeft)?;

        let mut present_params: D3DPRESENT_PARAMETERS = Default::default();
        present_params.BackBufferWidth = image.size.width;
        present_params.BackBufferHeight = image.size.height;
        present_params.Windowed = TRUE;
        present_params.SwapEffect = D3DSWAPEFFECT_DISCARD;
        present_params.BackBufferFormat = D3DFMT_A8R8G8B8;
        present_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE as u32;

        let device = unsafe {
            let mut device = None;
            direct3d.CreateDevice(
                D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL,
                HWND(std::ptr::null_mut()),
                D3DCREATE_HARDWARE_VERTEXPROCESSING as u32,
                &mut present_params,
                &mut device,
            )?;
            device.ok_or_else(|| anyhow!("Unable to create Direct3D 9 device"))?
        };

        let texture = unsafe {
            let mut texture = None;
            device.CreateTexture(
                image.size.width,
                image.size.height,
                1,
                0,
                D3DFMT_A8R8G8B8,
                D3DPOOL_MANAGED,
                &mut texture,
                std::ptr::null_mut(),
            )?;

            texture.ok_or_else(|| anyhow!("Unable to create Direct3D 9 texture"))?
        };

        unsafe {
            let mut lock = D3DLOCKED_RECT::default();
            texture.LockRect(0, &mut lock, std::ptr::null_mut(), 0)?;
            std::ptr::copy_nonoverlapping(
                image.bytes.as_ptr(),
                lock.pBits.cast(),
                image.bytes.len(),
            );
            texture.UnlockRect(0)?;
        }

        Ok(Self {
            texture,
            image,
            direct3d,
            device,
        })
    }
}
