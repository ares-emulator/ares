use crate::error;
use crate::error::assume_d3d11_init;
use windows::Win32::Foundation::BOOL;
use windows::Win32::Graphics::Direct3D11::{
    ID3D11BlendState, ID3D11Device, ID3D11DeviceContext, ID3D11RasterizerState, D3D11_BLEND_DESC,
    D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_BLEND_SRC_ALPHA,
    D3D11_COLOR_WRITE_ENABLE_ALL, D3D11_CULL_NONE, D3D11_DEFAULT_SAMPLE_MASK, D3D11_FILL_SOLID,
    D3D11_RASTERIZER_DESC, D3D11_RENDER_TARGET_BLEND_DESC,
};

pub struct D3D11State {
    blend: ID3D11BlendState,
    rs: ID3D11RasterizerState,
}

pub struct D3D11StateSaveGuard<'a> {
    ctx: &'a ID3D11DeviceContext,
    saved_blend: Option<ID3D11BlendState>,
    saved_blend_factor: [f32; 4],
    saved_blend_mask: u32,
    saved_rs: Option<ID3D11RasterizerState>,
    _state: &'a D3D11State,
}

impl D3D11State {
    pub fn new(device: &ID3D11Device) -> error::Result<D3D11State> {
        let blend = unsafe {
            let mut blend_desc = D3D11_BLEND_DESC {
                AlphaToCoverageEnable: BOOL::from(false),
                IndependentBlendEnable: BOOL::from(false),
                ..Default::default()
            };

            let rtv_blend_desc = D3D11_RENDER_TARGET_BLEND_DESC {
                BlendEnable: BOOL::from(false),
                SrcBlend: D3D11_BLEND_ONE,
                DestBlend: D3D11_BLEND_ONE,
                BlendOp: D3D11_BLEND_OP_ADD,
                SrcBlendAlpha: D3D11_BLEND_SRC_ALPHA,
                DestBlendAlpha: D3D11_BLEND_INV_SRC_ALPHA,
                BlendOpAlpha: D3D11_BLEND_OP_ADD,
                RenderTargetWriteMask: D3D11_COLOR_WRITE_ENABLE_ALL.0 as u8,
            };

            blend_desc.RenderTarget[0] = rtv_blend_desc;

            let mut blend = None;
            device.CreateBlendState(&blend_desc, Some(&mut blend))?;
            assume_d3d11_init!(blend, "CreateBlendState");
            blend
        };

        let rs = unsafe {
            let rs_desc = D3D11_RASTERIZER_DESC {
                FillMode: D3D11_FILL_SOLID,
                CullMode: D3D11_CULL_NONE,
                FrontCounterClockwise: BOOL::from(false),
                DepthBias: 0,
                DepthBiasClamp: 0.0,
                SlopeScaledDepthBias: 0.0,
                DepthClipEnable: BOOL::from(false),
                ScissorEnable: BOOL::from(false),
                MultisampleEnable: BOOL::from(false),
                AntialiasedLineEnable: BOOL::from(false),
            };
            let mut rs = None;
            device.CreateRasterizerState(&rs_desc, Some(&mut rs))?;
            assume_d3d11_init!(rs, "CreateRasterizerState");
            rs
        };

        Ok(D3D11State { blend, rs })
    }

    /// Enters the state necessary for rendering filter passes.
    pub fn enter_filter_state<'a>(
        &'a self,
        context: &'a ID3D11DeviceContext,
    ) -> D3D11StateSaveGuard<'a> {
        // save previous state
        let guard = unsafe {
            let mut saved_blend = None;
            let mut saved_blend_factor = [0f32; 4];
            let mut saved_blend_mask = 0;
            context.OMGetBlendState(
                Some(&mut saved_blend),
                Some(&mut saved_blend_factor),
                Some(&mut saved_blend_mask),
            );
            let saved_rs = context.RSGetState().ok();

            D3D11StateSaveGuard {
                ctx: context,
                saved_blend,
                saved_blend_factor,
                saved_blend_mask,
                saved_rs,
                _state: self,
            }
        };

        unsafe {
            context.RSSetState(&self.rs);
            context.OMSetBlendState(&self.blend, None, D3D11_DEFAULT_SAMPLE_MASK);
        }
        guard
    }
}

impl Drop for D3D11StateSaveGuard<'_> {
    fn drop(&mut self) {
        unsafe {
            self.ctx.RSSetState(self.saved_rs.as_ref());
            self.ctx.OMSetBlendState(
                self.saved_blend.as_ref(),
                Some(&self.saved_blend_factor),
                self.saved_blend_mask,
            );
        }
    }
}
