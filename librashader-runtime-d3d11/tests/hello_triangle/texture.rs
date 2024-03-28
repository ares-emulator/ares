use librashader_common::{FilterMode, Size, WrapMode};
use librashader_runtime::image::Image;
use librashader_runtime::scaling::MipmapSize;
use windows::Win32::Graphics::Direct3D::D3D_SRV_DIMENSION_TEXTURE2D;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11Device, ID3D11DeviceContext, ID3D11ShaderResourceView, ID3D11Texture2D,
    D3D11_BIND_RENDER_TARGET, D3D11_BIND_SHADER_RESOURCE, D3D11_BOX, D3D11_CPU_ACCESS_WRITE,
    D3D11_RESOURCE_MISC_GENERATE_MIPS, D3D11_SHADER_RESOURCE_VIEW_DESC,
    D3D11_SHADER_RESOURCE_VIEW_DESC_0, D3D11_SUBRESOURCE_DATA, D3D11_TEX2D_SRV,
    D3D11_TEXTURE2D_DESC, D3D11_USAGE_DYNAMIC, D3D11_USAGE_STAGING,
};
use windows::Win32::Graphics::Dxgi::Common::DXGI_SAMPLE_DESC;

/// An image view for use as a shader resource.
///
/// Contains an `ID3D11ShaderResourceView`, and a size.
#[derive(Debug, Clone)]
pub struct D3D11InputView {
    /// A handle to the shader resource view.
    pub handle: ID3D11ShaderResourceView,
    /// The size of the image.
    pub size: Size<u32>,
}

#[derive(Debug, Clone)]
pub struct InputTexture {
    pub view: D3D11InputView,
    pub filter: FilterMode,
    pub wrap_mode: WrapMode,
}

impl AsRef<InputTexture> for InputTexture {
    fn as_ref(&self) -> &InputTexture {
        self
    }
}

#[derive(Debug, Clone)]
pub(crate) struct ExampleTexture {
    // The handle to the Texture2D must be kept alive.
    #[allow(dead_code)]
    pub handle: ID3D11Texture2D,
    #[allow(dead_code)]
    pub desc: D3D11_TEXTURE2D_DESC,
    pub image: InputTexture,
}

impl AsRef<InputTexture> for ExampleTexture {
    fn as_ref(&self) -> &InputTexture {
        &self.image
    }
}

impl ExampleTexture {
    pub fn new(
        device: &ID3D11Device,
        context: &ID3D11DeviceContext,
        source: &Image,
        desc: D3D11_TEXTURE2D_DESC,
        filter: FilterMode,
        wrap_mode: WrapMode,
    ) -> Result<ExampleTexture, windows::core::Error> {
        let mut desc = D3D11_TEXTURE2D_DESC {
            Width: source.size.width,
            Height: source.size.height,
            // todo: set this to 0
            MipLevels: if (desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS.0 as u32) != 0 {
                0
            } else {
                1
            },
            ArraySize: 1,
            SampleDesc: DXGI_SAMPLE_DESC {
                Count: 1,
                Quality: 0,
            },
            CPUAccessFlags: if desc.Usage == D3D11_USAGE_DYNAMIC {
                D3D11_CPU_ACCESS_WRITE.0 as u32
            } else {
                0
            },
            ..desc
        };
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE.0 as u32;

        // determine number of mipmaps required
        if (desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS.0 as u32) != 0 {
            desc.BindFlags |= D3D11_BIND_RENDER_TARGET.0 as u32;
            desc.MipLevels = source.size.calculate_miplevels();
        }

        // Don't need to determine format support because LUTs are always DXGI_FORMAT_R8G8B8A8_UNORM
        // since we load them with the Image module.

        unsafe {
            let mut handle = None;
            device.CreateTexture2D(&desc, None, Some(&mut handle))?;
            let handle = handle.unwrap();

            // need a staging texture to defer mipmap generation
            let mut staging = None;
            device.CreateTexture2D(
                &D3D11_TEXTURE2D_DESC {
                    MipLevels: 1,
                    BindFlags: 0,
                    MiscFlags: 0,
                    Usage: D3D11_USAGE_STAGING,
                    CPUAccessFlags: D3D11_CPU_ACCESS_WRITE.0 as u32,
                    ..desc
                },
                Some(&D3D11_SUBRESOURCE_DATA {
                    pSysMem: source.bytes.as_ptr().cast(),
                    SysMemPitch: source.pitch as u32,
                    SysMemSlicePitch: 0,
                }),
                Some(&mut staging),
            )?;
            let staging = staging.unwrap();

            context.CopySubresourceRegion(
                &handle,
                0,
                0,
                0,
                0,
                &staging,
                0,
                Some(&D3D11_BOX {
                    left: 0,
                    top: 0,
                    front: 0,
                    right: source.size.width,
                    bottom: source.size.height,
                    back: 1,
                }),
            );

            let mut srv = None;
            device.CreateShaderResourceView(
                &handle,
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
            let srv = srv.unwrap();

            if (desc.MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS.0 as u32) != 0 {
                context.GenerateMips(&srv)
            }

            Ok(ExampleTexture {
                handle,
                desc,
                image: InputTexture {
                    view: D3D11InputView {
                        handle: srv,
                        size: source.size,
                    },
                    filter,
                    wrap_mode,
                },
            })
        }
    }
}
