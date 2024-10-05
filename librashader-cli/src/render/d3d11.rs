use crate::render::{CommonFrameOptions, RenderTest};
use anyhow::anyhow;
use image::RgbaImage;
use librashader::runtime::d3d11::*;
use librashader::runtime::{FilterChainParameters, RuntimeParameters};
use librashader::runtime::{Size, Viewport};
use std::path::Path;

impl RenderTest for Direct3D11 {
    fn new(path: &Path) -> anyhow::Result<Self>
    where
        Self: Sized,
    {
        Direct3D11::new(path)
    }

    fn render_with_preset_and_params(
        &mut self,
        preset: ShaderPreset,
        frame_count: usize,
        param_setter: Option<&dyn Fn(&RuntimeParameters)>,
        frame_options: Option<CommonFrameOptions>,
    ) -> anyhow::Result<image::RgbaImage> {
        let (renderbuffer, rtv) = self.create_renderbuffer(self.image_bytes.size)?;

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
            let viewport = Viewport::new_render_target_sized_origin(&rtv, None)?;
            let options = frame_options.map(|options| FrameOptions {
                clear_history: options.clear_history,
                frame_direction: options.frame_direction,
                rotation: options.rotation,
                total_subframes: options.total_subframes,
                current_subframe: options.current_subframe,
            });

            for frame in 0..=frame_count {
                filter_chain.frame(None, &self.image_srv, &viewport, frame, options.as_ref())?;
            }

            let mut renderbuffer_desc = Default::default();
            renderbuffer.GetDesc(&mut renderbuffer_desc);

            self.immediate_context.Flush();

            let mut staging = None;
            self.device.CreateTexture2D(
                &D3D11_TEXTURE2D_DESC {
                    MipLevels: 1,
                    BindFlags: 0,
                    MiscFlags: 0,
                    Usage: D3D11_USAGE_STAGING,
                    CPUAccessFlags: D3D11_CPU_ACCESS_READ.0 as u32,
                    ..renderbuffer_desc
                },
                None,
                Some(&mut staging),
            )?;

            let staging = staging.ok_or(anyhow!("Unable to create staging texture"))?;

            self.immediate_context.CopyResource(&staging, &renderbuffer);

            let mut map_info = Default::default();
            self.immediate_context
                .Map(&staging, 0, D3D11_MAP_READ, 0, Some(&mut map_info))?;

            let slice = std::slice::from_raw_parts(
                map_info.pData as *const u8,
                (renderbuffer_desc.Height * map_info.RowPitch) as usize,
            );

            let image = RgbaImage::from_raw(
                renderbuffer_desc.Width,
                renderbuffer_desc.Height,
                Vec::from(slice),
            )
            .ok_or(anyhow!("Unable to create image from data"))?;
            self.immediate_context.Unmap(&staging, 0);

            Ok(image)
        }
    }
}

use librashader::presets::ShaderPreset;
use librashader_runtime::image::{Image, UVDirection};
use windows::{
    Win32::Foundation::*, Win32::Graphics::Direct3D::*, Win32::Graphics::Direct3D11::*,
    Win32::Graphics::Dxgi::Common::*, Win32::Graphics::Dxgi::*,
};

pub struct Direct3D11 {
    device: ID3D11Device,
    immediate_context: ID3D11DeviceContext,
    _image_tex: ID3D11Texture2D,
    image_srv: ID3D11ShaderResourceView,
    image_bytes: Image,
}

impl Direct3D11 {
    fn create_device() -> anyhow::Result<(IDXGIFactory4, ID3D11Device, ID3D11DeviceContext)> {
        let dxgi_factory: IDXGIFactory4 = unsafe { CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG) }?;
        let feature_levels = vec![D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_11_1];

        let mut out_device = None;
        let mut out_context = None;
        let mut _out_feature_level = D3D_FEATURE_LEVEL_11_0;

        unsafe {
            D3D11CreateDevice(
                None,
                D3D_DRIVER_TYPE_HARDWARE,
                HMODULE::default(),
                D3D11_CREATE_DEVICE_DEBUG,
                Some(&feature_levels),
                D3D11_SDK_VERSION,
                Some(&mut out_device),
                Some(&mut _out_feature_level),
                Some(&mut out_context),
            )
        }?;
        Ok((dxgi_factory, out_device.unwrap(), out_context.unwrap()))
    }

    pub fn new(image_path: &Path) -> anyhow::Result<Self> {
        let (_factory, device, imm_context) = Self::create_device()?;

        let (image, image_tex, srv) = Self::load_image(&device, image_path)?;
        Ok(Self {
            device,
            immediate_context: imm_context,
            image_bytes: image,
            _image_tex: image_tex,
            image_srv: srv,
        })
    }

    fn load_image(
        device: &ID3D11Device,
        image_path: &Path,
    ) -> anyhow::Result<(Image, ID3D11Texture2D, ID3D11ShaderResourceView)> {
        let image = Image::load(image_path, UVDirection::TopLeft)?;
        let desc = D3D11_TEXTURE2D_DESC {
            Width: image.size.width,
            Height: image.size.height,
            // todo: set this to 0
            MipLevels: 1,
            ArraySize: 1,
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                Quality: 0,
            },
            CPUAccessFlags: 0,
            Format: DXGI_FORMAT_R8G8B8A8_UNORM,
            Usage: D3D11_USAGE_DEFAULT,
            BindFlags: D3D11_BIND_SHADER_RESOURCE.0 as u32,
            ..Default::default()
        };

        unsafe {
            let mut resource = None;
            device.CreateTexture2D(
                &desc,
                Some(&D3D11_SUBRESOURCE_DATA {
                    pSysMem: image.bytes.as_ptr().cast(),
                    SysMemPitch: image.pitch as u32,
                    SysMemSlicePitch: 0,
                }),
                Some(&mut resource),
            )?;
            let resource = resource.ok_or(anyhow!("Failed to create texture"))?;

            let mut srv = None;
            device.CreateShaderResourceView(
                &resource,
                Some(&D3D11_SHADER_RESOURCE_VIEW_DESC {
                    Format: desc.Format,
                    ViewDimension: D3D_SRV_DIMENSION_TEXTURE2D,
                    Anonymous: D3D11_SHADER_RESOURCE_VIEW_DESC_0 {
                        Texture2D: D3D11_TEX2D_SRV {
                            MostDetailedMip: 0,
                            MipLevels: u32::MAX,
                        },
                    },
                }),
                Some(&mut srv),
            )?;

            let srv = srv.ok_or(anyhow!("Failed to create SRV"))?;

            Ok((image, resource, srv))
        }
    }

    fn create_renderbuffer(
        &self,
        size: Size<u32>,
    ) -> anyhow::Result<(ID3D11Texture2D, ID3D11RenderTargetView)> {
        let desc = D3D11_TEXTURE2D_DESC {
            Width: size.width,
            Height: size.height,
            // todo: set this to 0
            MipLevels: 1,
            ArraySize: 1,
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                Quality: 0,
            },
            CPUAccessFlags: 0,
            Format: DXGI_FORMAT_R8G8B8A8_UNORM,
            Usage: D3D11_USAGE_DEFAULT,
            BindFlags: D3D11_BIND_RENDER_TARGET.0 as u32,
            ..Default::default()
        };

        unsafe {
            let mut resource = None;
            self.device
                .CreateTexture2D(&desc, None, Some(&mut resource))?;
            let resource = resource.ok_or(anyhow!("Failed to create texture"))?;

            let mut rtv = None;
            self.device
                .CreateRenderTargetView(&resource, None, Some(&mut rtv))?;

            let rtv = rtv.ok_or(anyhow!("Failed to create RTV"))?;

            Ok((resource, rtv))
        }
    }
}
