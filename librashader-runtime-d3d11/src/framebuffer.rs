use crate::error::{assume_d3d11_init, FilterChainError};
use crate::texture::D3D11InputView;
use crate::util::d3d11_get_closest_format;
use crate::{error, D3D11OutputView};
use librashader_common::{ImageFormat, Size};
use librashader_presets::Scale2D;
use librashader_runtime::scaling::{MipmapSize, ScaleFramebuffer, ViewportSize};
use windows::core::ComInterface;
use windows::Win32::Graphics::Direct3D::D3D_SRV_DIMENSION_TEXTURE2D;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11Device, ID3D11DeviceContext, ID3D11RenderTargetView, ID3D11ShaderResourceView,
    ID3D11Texture2D, D3D11_BIND_RENDER_TARGET, D3D11_BIND_SHADER_RESOURCE, D3D11_BOX,
    D3D11_CPU_ACCESS_FLAG, D3D11_FORMAT_SUPPORT_RENDER_TARGET, D3D11_FORMAT_SUPPORT_SHADER_SAMPLE,
    D3D11_FORMAT_SUPPORT_TEXTURE2D, D3D11_RENDER_TARGET_VIEW_DESC, D3D11_RENDER_TARGET_VIEW_DESC_0,
    D3D11_RESOURCE_MISC_GENERATE_MIPS, D3D11_RTV_DIMENSION_TEXTURE2D,
    D3D11_SHADER_RESOURCE_VIEW_DESC, D3D11_SHADER_RESOURCE_VIEW_DESC_0, D3D11_TEX2D_RTV,
    D3D11_TEX2D_SRV, D3D11_TEXTURE2D_DESC, D3D11_USAGE_DEFAULT,
};
use windows::Win32::Graphics::Dxgi::Common::{DXGI_FORMAT, DXGI_SAMPLE_DESC};

static CLEAR: &[f32; 4] = &[0.0f32, 0.0, 0.0, 0.0];

#[derive(Debug, Clone)]
pub(crate) struct OwnedImage {
    render: ID3D11Texture2D,
    pub(crate) size: Size<u32>,
    format: DXGI_FORMAT,
    device: ID3D11Device,
    max_mipmap: u32,
}

impl OwnedImage {
    pub fn new(
        device: &ID3D11Device,
        size: Size<u32>,
        format: ImageFormat,
        mipmap: bool,
    ) -> error::Result<OwnedImage> {
        unsafe {
            let format = d3d11_get_closest_format(
                device,
                DXGI_FORMAT::from(format),
                D3D11_FORMAT_SUPPORT_TEXTURE2D.0
                    | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE.0
                    | D3D11_FORMAT_SUPPORT_RENDER_TARGET.0,
            );
            let desc = default_desc(size, format, 1);
            let mut render = None;
            device.CreateTexture2D(&desc, None, Some(&mut render))?;
            assume_d3d11_init!(render, "CreateTexture2D");

            Ok(OwnedImage {
                render,
                size,
                format,
                device: device.clone(),
                max_mipmap: if mipmap {
                    size.calculate_miplevels()
                } else {
                    1
                },
            })
        }
    }

    pub(crate) fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        should_mipmap: bool,
    ) -> error::Result<Size<u32>> {
        let size = source_size.scale_viewport(scaling, *viewport_size, *original_size);

        if self.size != size
            || (should_mipmap && self.max_mipmap == 1)
            || (!should_mipmap && self.max_mipmap != 1)
        {
            self.size = size;
            self.max_mipmap = if should_mipmap {
                size.calculate_miplevels()
            } else {
                1
            };

            self.init(
                size,
                if format == ImageFormat::Unknown {
                    ImageFormat::R8G8B8A8Unorm
                } else {
                    format
                },
            )?;
        }
        Ok(size)
    }

    pub fn clear(&mut self, ctx: &ID3D11DeviceContext) -> error::Result<()> {
        let rtv = self.create_render_target_view()?;
        unsafe {
            ctx.ClearRenderTargetView(&rtv, CLEAR.as_ptr());
        }
        Ok(())
    }

    pub fn init(&mut self, size: Size<u32>, format: ImageFormat) -> error::Result<()> {
        let format = d3d11_get_closest_format(
            &self.device,
            DXGI_FORMAT::from(format),
            D3D11_FORMAT_SUPPORT_TEXTURE2D.0
                | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE.0
                | D3D11_FORMAT_SUPPORT_RENDER_TARGET.0,
        );

        let desc = default_desc(size, format, self.max_mipmap);
        unsafe {
            let mut texture = None;
            self.device
                .CreateTexture2D(&desc, None, Some(&mut texture))?;
            assume_d3d11_init!(mut texture, "CreateTexture2D");
            std::mem::swap(&mut self.render, &mut texture);
            drop(texture)
        }
        self.format = format;
        self.size = size;

        Ok(())
    }

    pub fn create_shader_resource_view(&self) -> error::Result<ID3D11ShaderResourceView> {
        let mut srv = None;
        unsafe {
            self.device.CreateShaderResourceView(
                &self.render,
                Some(&D3D11_SHADER_RESOURCE_VIEW_DESC {
                    Format: self.format,
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
        }
        assume_d3d11_init!(srv, "CreateShaderResourceView");
        Ok(srv)
    }

    pub fn create_render_target_view(&self) -> error::Result<ID3D11RenderTargetView> {
        let mut rtv = None;
        unsafe {
            self.device.CreateRenderTargetView(
                &self.render,
                Some(&D3D11_RENDER_TARGET_VIEW_DESC {
                    Format: self.format,
                    ViewDimension: D3D11_RTV_DIMENSION_TEXTURE2D,
                    Anonymous: D3D11_RENDER_TARGET_VIEW_DESC_0 {
                        Texture2D: D3D11_TEX2D_RTV { MipSlice: 0 },
                    },
                }),
                Some(&mut rtv),
            )?
        }

        assume_d3d11_init!(rtv, "CreateRenderTargetView");
        Ok(rtv)
    }

    pub fn as_output(&self) -> error::Result<D3D11OutputView> {
        let rtv = self.create_render_target_view()?;
        Ok(D3D11OutputView {
            handle: rtv,
            size: self.size,
        })
    }

    pub fn copy_from(
        &mut self,
        ctx: &ID3D11DeviceContext,
        image: &D3D11InputView,
    ) -> error::Result<()> {
        let original_resource: ID3D11Texture2D = unsafe {
            let resource = image.handle.GetResource()?;
            resource.cast()?
        };

        let format = unsafe {
            let mut desc = Default::default();
            original_resource.GetDesc(&mut desc);
            desc.Format
        };

        if self.size != image.size || format != self.format {
            // eprintln!("[history] resizing");
            self.init(image.size, ImageFormat::from(format))?;
        }

        unsafe {
            ctx.CopySubresourceRegion(
                &self.render,
                0,
                0,
                0,
                0,
                &original_resource,
                0,
                Some(&D3D11_BOX {
                    left: 0,
                    top: 0,
                    front: 0,
                    right: self.size.width,
                    bottom: self.size.height,
                    back: 1,
                }),
            )
        }

        let srvs = self.create_shader_resource_view()?;
        unsafe {
            ctx.GenerateMips(&srvs);
        }
        Ok(())
    }
}

fn default_desc(size: Size<u32>, format: DXGI_FORMAT, mip_levels: u32) -> D3D11_TEXTURE2D_DESC {
    D3D11_TEXTURE2D_DESC {
        Width: size.width,
        Height: size.height,
        MipLevels: mip_levels,
        ArraySize: 1,
        Format: format,
        SampleDesc: DXGI_SAMPLE_DESC {
            Count: 1,
            Quality: 0,
        },
        Usage: D3D11_USAGE_DEFAULT,
        BindFlags: D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
        CPUAccessFlags: D3D11_CPU_ACCESS_FLAG(0),
        MiscFlags: D3D11_RESOURCE_MISC_GENERATE_MIPS,
    }
}

impl ScaleFramebuffer for OwnedImage {
    type Error = FilterChainError;
    type Context = ();

    fn scale(
        &mut self,
        scaling: Scale2D,
        format: ImageFormat,
        viewport_size: &Size<u32>,
        source_size: &Size<u32>,
        original_size: &Size<u32>,
        should_mipmap: bool,
        _context: &Self::Context,
    ) -> Result<Size<u32>, Self::Error> {
        self.scale(
            scaling,
            format,
            viewport_size,
            source_size,
            original_size,
            should_mipmap,
        )
    }
}
