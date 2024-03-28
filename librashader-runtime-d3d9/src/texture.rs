use crate::error;
use crate::error::{assume_d3d_init, FilterChainError};

use crate::util::GetSize;
use librashader_common::{FilterMode, ImageFormat, Size, WrapMode};
use librashader_presets::Scale2D;
use librashader_runtime::binding::TextureInput;
use librashader_runtime::scaling::{ScaleFramebuffer, ViewportSize};
use windows::Win32::Graphics::Direct3D9::{
    IDirect3DDevice9, IDirect3DSurface9, IDirect3DTexture9, D3DCLEAR_TARGET, D3DFORMAT,
    D3DPOOL_DEFAULT, D3DTEXF_LINEAR, D3DUSAGE_RENDERTARGET,
};

/// An image view for use as a shader resource.
///
/// Contains an `ID3D11ShaderResourceView`, and a size.
#[derive(Debug, Clone)]
pub struct D3D9Texture {
    /// A handle to the shader resource view.
    pub handle: IDirect3DTexture9,
    pub mipmap: bool,
    pub original_format: ImageFormat,
}

impl TextureInput for D3D9InputTexture {
    fn size(&self) -> Size<u32> {
        GetSize::size(&self.handle).unwrap_or_else(|_| Size::new(0, 0))
    }
}

impl AsRef<D3D9InputTexture> for D3D9InputTexture {
    fn as_ref(&self) -> &D3D9InputTexture {
        &self
    }
}

#[derive(Debug, Clone)]
pub struct D3D9InputTexture {
    /// A handle to the shader resource view.
    pub handle: IDirect3DTexture9,
    pub filter: FilterMode,
    pub wrap: WrapMode,
    pub mipmode: FilterMode,
    pub is_srgb: bool,
}

impl D3D9Texture {
    pub fn new(
        device: &IDirect3DDevice9,
        size: Size<u32>,
        format: ImageFormat,
        mipmap: bool,
    ) -> error::Result<Self> {
        let mut texture = None;
        // eprintln!("creating texture");
        unsafe {
            device.CreateTexture(
                size.width,
                size.height,
                if mipmap { 0 } else { 1 },
                D3DUSAGE_RENDERTARGET as u32,
                format.into(),
                D3DPOOL_DEFAULT,
                &mut texture,
                std::ptr::null_mut(),
            )?;
        }

        // eprintln!("creating texture ok");

        assume_d3d_init!(texture, "CreateTexture");

        Ok(Self {
            handle: texture,
            mipmap,
            original_format: format,
        })
    }

    pub fn init(&mut self, size: Size<u32>, format: ImageFormat) -> error::Result<()> {
        // let format = format.into();

        unsafe {
            let mut texture = None;
            self.handle.GetDevice()?.CreateTexture(
                size.width,
                size.height,
                if self.mipmap { 0 } else { 1 },
                D3DUSAGE_RENDERTARGET as u32,
                format.into(),
                D3DPOOL_DEFAULT,
                &mut texture,
                std::ptr::null_mut(),
            )?;
            assume_d3d_init!(mut texture, "CreateTexture2D");
            std::mem::swap(&mut self.handle, &mut texture);
            drop(texture)
        }

        Ok(())
    }

    pub fn size(&self) -> error::Result<Size<u32>> {
        let mut desc = Default::default();
        unsafe {
            self.handle.GetLevelDesc(0, &mut desc)?;
        }

        Ok(Size {
            height: desc.Height,
            width: desc.Width,
        })
    }

    pub fn as_output(&self) -> error::Result<IDirect3DSurface9> {
        unsafe { Ok(self.handle.GetSurfaceLevel(0)?) }
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

        if self.size()? != size || should_mipmap != self.mipmap {
            self.mipmap = should_mipmap;
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

    pub fn clear(&mut self, device: &IDirect3DDevice9) -> error::Result<()> {
        unsafe {
            let surface = self.handle.GetSurfaceLevel(0)?;
            device.SetRenderTarget(0, &surface)?;
            device.Clear(0, std::ptr::null_mut(), D3DCLEAR_TARGET as u32, 0x0, 0.0, 0)?;
        }
        Ok(())
    }

    pub fn copy_from(
        &mut self,
        device: &IDirect3DDevice9,
        input: &IDirect3DTexture9,
    ) -> error::Result<()> {
        let mut desc = Default::default();
        unsafe {
            input.GetLevelDesc(0, &mut desc)?;
        }

        let size = Size {
            width: desc.Width,
            height: desc.Height,
        };

        if self.size()? != size || D3DFORMAT::from(self.original_format) != desc.Format {
            eprintln!("[history] resizing");
            self.init(size, ImageFormat::from(desc.Format))?;
        }

        unsafe {
            let dest = self.handle.GetSurfaceLevel(0)?;
            let source = input.GetSurfaceLevel(0)?;

            device.StretchRect(
                &source,
                std::ptr::null(),
                &dest,
                std::ptr::null(),
                D3DTEXF_LINEAR,
            )?;
        }

        Ok(())
    }

    pub fn as_input(
        &self,
        filter: FilterMode,
        mipmode: FilterMode,
        wrap: WrapMode,
    ) -> D3D9InputTexture {
        D3D9InputTexture {
            handle: self.handle.clone(),
            filter,
            wrap,
            mipmode,
            is_srgb: self.original_format == ImageFormat::R8G8B8A8Srgb,
        }
    }
}

impl ScaleFramebuffer for D3D9Texture {
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
