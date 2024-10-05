use crate::draw_quad::DrawQuad;
use crate::error::assume_d3d12_init;
use crate::error::FilterChainError::Direct3DOperationError;
use crate::{error, util};
use librashader_cache::{cache_pipeline, cache_shader_object};
use librashader_common::map::FastHashMap;
use librashader_reflect::back::dxil::DxilObject;
use librashader_reflect::back::hlsl::CrossHlslContext;
use librashader_reflect::back::ShaderCompilerOutput;
use std::hash::{Hash, Hasher};
use std::mem::ManuallyDrop;
use std::ops::Deref;
use widestring::u16cstr;
use windows::core::Interface;
use windows::Win32::Foundation::BOOL;
use windows::Win32::Graphics::Direct3D::Dxc::{
    CLSID_DxcLibrary, DxcCreateInstance, IDxcBlob, IDxcCompiler, IDxcUtils, IDxcValidator, DXC_CP,
};
use windows::Win32::Graphics::Direct3D12::{
    D3D12SerializeVersionedRootSignature, ID3D12Device, ID3D12PipelineState, ID3D12RootSignature,
    D3D12_BLEND_DESC, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD, D3D12_BLEND_SRC_ALPHA,
    D3D12_CACHED_PIPELINE_STATE, D3D12_COLOR_WRITE_ENABLE_ALL, D3D12_CULL_MODE_NONE,
    D3D12_DESCRIPTOR_RANGE1, D3D12_DESCRIPTOR_RANGE_FLAGS,
    D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_FILL_MODE_SOLID,
    D3D12_GRAPHICS_PIPELINE_STATE_DESC, D3D12_INPUT_LAYOUT_DESC, D3D12_LOGIC_OP_NOOP,
    D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE, D3D12_RASTERIZER_DESC, D3D12_RENDER_TARGET_BLEND_DESC,
    D3D12_ROOT_DESCRIPTOR1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_ROOT_DESCRIPTOR_TABLE1,
    D3D12_ROOT_PARAMETER1, D3D12_ROOT_PARAMETER1_0, D3D12_ROOT_PARAMETER_TYPE_CBV,
    D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, D3D12_ROOT_SIGNATURE_DESC1,
    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, D3D12_SHADER_BYTECODE,
    D3D12_SHADER_VISIBILITY_ALL, D3D12_SHADER_VISIBILITY_PIXEL,
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC, D3D12_VERSIONED_ROOT_SIGNATURE_DESC_0,
    D3D_ROOT_SIGNATURE_VERSION_1_1,
};
use windows::Win32::Graphics::Dxgi::Common::{DXGI_FORMAT, DXGI_FORMAT_UNKNOWN, DXGI_SAMPLE_DESC};

// bruh why does DXGI_FORMAT not impl hash
#[repr(transparent)]
#[derive(PartialEq, Eq)]
struct HashDxgiFormat(DXGI_FORMAT);
impl Hash for HashDxgiFormat {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.0 .0.hash(state);
    }
}

pub struct D3D12GraphicsPipeline {
    render_pipelines: FastHashMap<HashDxgiFormat, ID3D12PipelineState>,
    vertex: Vec<u8>,
    fragment: Vec<u8>,
    cache_disabled: bool,
}

const D3D12_SLANG_ROOT_PARAMETERS: &[D3D12_ROOT_PARAMETER1; 4] = &[
    // srvs
    D3D12_ROOT_PARAMETER1 {
        ParameterType: D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        Anonymous: D3D12_ROOT_PARAMETER1_0 {
            DescriptorTable: D3D12_ROOT_DESCRIPTOR_TABLE1 {
                NumDescriptorRanges: 1,
                pDescriptorRanges: &D3D12_DESCRIPTOR_RANGE1 {
                    RangeType: D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                    NumDescriptors: 16,
                    BaseShaderRegister: 0,
                    RegisterSpace: 0,
                    Flags: D3D12_DESCRIPTOR_RANGE_FLAGS(
                        D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE.0
                            | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE.0,
                    ),
                    OffsetInDescriptorsFromTableStart: 0,
                },
            },
        },
        ShaderVisibility: D3D12_SHADER_VISIBILITY_PIXEL,
    },
    // samplers
    D3D12_ROOT_PARAMETER1 {
        ParameterType: D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        Anonymous: D3D12_ROOT_PARAMETER1_0 {
            DescriptorTable: D3D12_ROOT_DESCRIPTOR_TABLE1 {
                NumDescriptorRanges: 1,
                pDescriptorRanges: &D3D12_DESCRIPTOR_RANGE1 {
                    RangeType: D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
                    NumDescriptors: 16,
                    BaseShaderRegister: 0,
                    RegisterSpace: 0,
                    Flags: D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
                    OffsetInDescriptorsFromTableStart: 0,
                },
            },
        },
        ShaderVisibility: D3D12_SHADER_VISIBILITY_PIXEL,
    },
    // UBO
    D3D12_ROOT_PARAMETER1 {
        ParameterType: D3D12_ROOT_PARAMETER_TYPE_CBV,
        Anonymous: D3D12_ROOT_PARAMETER1_0 {
            Descriptor: D3D12_ROOT_DESCRIPTOR1 {
                ShaderRegister: 0,
                RegisterSpace: 0,
                Flags: D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            },
        },
        ShaderVisibility: D3D12_SHADER_VISIBILITY_ALL,
    },
    // push
    D3D12_ROOT_PARAMETER1 {
        ParameterType: D3D12_ROOT_PARAMETER_TYPE_CBV,
        Anonymous: D3D12_ROOT_PARAMETER1_0 {
            Descriptor: D3D12_ROOT_DESCRIPTOR1 {
                ShaderRegister: 1,
                RegisterSpace: 0,
                Flags: D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
            },
        },
        ShaderVisibility: D3D12_SHADER_VISIBILITY_ALL,
    },
];

const D3D12_SLANG_VERSIONED_ROOT_SIGNATURE: &D3D12_VERSIONED_ROOT_SIGNATURE_DESC =
    &D3D12_VERSIONED_ROOT_SIGNATURE_DESC {
        Version: D3D_ROOT_SIGNATURE_VERSION_1_1,
        Anonymous: D3D12_VERSIONED_ROOT_SIGNATURE_DESC_0 {
            Desc_1_1: D3D12_ROOT_SIGNATURE_DESC1 {
                NumParameters: D3D12_SLANG_ROOT_PARAMETERS.len() as u32,
                pParameters: D3D12_SLANG_ROOT_PARAMETERS.as_ptr(),
                NumStaticSamplers: 0,
                pStaticSamplers: std::ptr::null(),
                Flags: D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
            },
        },
    };

pub struct D3D12RootSignature {
    pub(crate) handle: ID3D12RootSignature,
}

impl D3D12RootSignature {
    pub fn new(device: &ID3D12Device) -> error::Result<D3D12RootSignature> {
        let signature = unsafe {
            let mut rs_blob = None;

            D3D12SerializeVersionedRootSignature(
                D3D12_SLANG_VERSIONED_ROOT_SIGNATURE,
                &mut rs_blob,
                None,
            )?;

            assume_d3d12_init!(rs_blob, "D3D12SerializeVersionedRootSignature");
            let blob = std::slice::from_raw_parts(
                rs_blob.GetBufferPointer().cast(),
                rs_blob.GetBufferSize(),
            );
            let root_signature: ID3D12RootSignature = device.CreateRootSignature(0, blob)?;
            root_signature
        };

        Ok(D3D12RootSignature { handle: signature })
    }
}
impl D3D12GraphicsPipeline {
    fn make_pipeline_state(
        device: &ID3D12Device,
        vertex_dxil: &IDxcBlob,
        fragment_dxil: &IDxcBlob,
        root_signature: &D3D12RootSignature,
        render_format: DXGI_FORMAT,
        disable_cache: bool,
    ) -> error::Result<ID3D12PipelineState> {
        let input_element = DrawQuad::get_spirv_cross_vbo_desc();

        let pipeline_state: ID3D12PipelineState = unsafe {
            let pipeline_desc = D3D12_GRAPHICS_PIPELINE_STATE_DESC {
                pRootSignature: ManuallyDrop::new(Some(root_signature.handle.clone())),
                VS: D3D12_SHADER_BYTECODE {
                    pShaderBytecode: vertex_dxil.GetBufferPointer(),
                    BytecodeLength: vertex_dxil.GetBufferSize(),
                },
                PS: D3D12_SHADER_BYTECODE {
                    pShaderBytecode: fragment_dxil.GetBufferPointer(),
                    BytecodeLength: fragment_dxil.GetBufferSize(),
                },
                StreamOutput: Default::default(),
                BlendState: D3D12_BLEND_DESC {
                    RenderTarget: [
                        D3D12_RENDER_TARGET_BLEND_DESC {
                            BlendEnable: BOOL::from(false),
                            LogicOpEnable: BOOL::from(false),
                            SrcBlend: D3D12_BLEND_SRC_ALPHA,
                            DestBlend: D3D12_BLEND_INV_SRC_ALPHA,
                            BlendOp: D3D12_BLEND_OP_ADD,
                            SrcBlendAlpha: D3D12_BLEND_SRC_ALPHA,
                            DestBlendAlpha: D3D12_BLEND_INV_SRC_ALPHA,
                            BlendOpAlpha: D3D12_BLEND_OP_ADD,
                            LogicOp: D3D12_LOGIC_OP_NOOP,
                            RenderTargetWriteMask: D3D12_COLOR_WRITE_ENABLE_ALL.0 as u8,
                        },
                        Default::default(),
                        Default::default(),
                        Default::default(),
                        Default::default(),
                        Default::default(),
                        Default::default(),
                        Default::default(),
                    ],
                    ..Default::default()
                },
                SampleMask: u32::MAX,
                RasterizerState: D3D12_RASTERIZER_DESC {
                    FillMode: D3D12_FILL_MODE_SOLID,
                    CullMode: D3D12_CULL_MODE_NONE,
                    ..Default::default()
                },
                DepthStencilState: Default::default(),
                InputLayout: D3D12_INPUT_LAYOUT_DESC {
                    pInputElementDescs: input_element.as_ptr(),
                    NumElements: input_element.len() as u32,
                },
                PrimitiveTopologyType: D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                NumRenderTargets: 1,
                RTVFormats: [
                    render_format,
                    DXGI_FORMAT_UNKNOWN,
                    DXGI_FORMAT_UNKNOWN,
                    DXGI_FORMAT_UNKNOWN,
                    DXGI_FORMAT_UNKNOWN,
                    DXGI_FORMAT_UNKNOWN,
                    DXGI_FORMAT_UNKNOWN,
                    DXGI_FORMAT_UNKNOWN,
                ],
                SampleDesc: DXGI_SAMPLE_DESC {
                    Count: 1,
                    Quality: 0,
                },
                NodeMask: 0,
                ..Default::default()
            };

            let pipeline = cache_pipeline(
                "d3d12",
                &[vertex_dxil, fragment_dxil, &render_format.0],
                |cached: Option<Vec<u8>>| {
                    if let Some(cached) = cached {
                        let pipeline_desc = D3D12_GRAPHICS_PIPELINE_STATE_DESC {
                            CachedPSO: D3D12_CACHED_PIPELINE_STATE {
                                pCachedBlob: cached.as_ptr().cast(),
                                CachedBlobSizeInBytes: cached.len(),
                            },
                            pRootSignature: ManuallyDrop::new(Some(root_signature.handle.clone())),
                            ..pipeline_desc
                        };
                        let pipeline_state = device.CreateGraphicsPipelineState(&pipeline_desc);
                        drop(ManuallyDrop::into_inner(pipeline_desc.pRootSignature));
                        pipeline_state
                    } else {
                        device.CreateGraphicsPipelineState(&pipeline_desc)
                    }
                },
                |pso: &ID3D12PipelineState| {
                    let cached_pso = pso.GetCachedBlob()?;
                    Ok(cached_pso)
                },
                disable_cache,
            )?;

            // cleanup handle
            drop(ManuallyDrop::into_inner(pipeline_desc.pRootSignature));

            pipeline
        };

        Ok(pipeline_state)
    }

    pub fn pipeline_state(&self, format: DXGI_FORMAT) -> &ID3D12PipelineState {
        let Some(pipeline) = self
            .render_pipelines
            .get(&HashDxgiFormat(format))
            .or_else(|| self.render_pipelines.values().next())
        else {
            panic!("No available render pipeline found");
        };

        pipeline
    }

    pub fn new_from_blobs(
        device: &ID3D12Device,
        vertex_dxil: IDxcBlob,
        fragment_dxil: IDxcBlob,
        root_signature: &D3D12RootSignature,
        render_format: DXGI_FORMAT,
        disable_cache: bool,
    ) -> error::Result<D3D12GraphicsPipeline> {
        let pipeline_state = Self::make_pipeline_state(
            device,
            &vertex_dxil,
            &fragment_dxil,
            root_signature,
            render_format,
            disable_cache,
        )?;

        unsafe {
            let vertex = Vec::from(std::slice::from_raw_parts(
                vertex_dxil.GetBufferPointer().cast(),
                vertex_dxil.GetBufferSize(),
            ));
            let fragment = Vec::from(std::slice::from_raw_parts(
                fragment_dxil.GetBufferPointer().cast(),
                fragment_dxil.GetBufferSize(),
            ));

            let mut render_pipelines = FastHashMap::default();
            render_pipelines.insert(HashDxgiFormat(render_format), pipeline_state);
            Ok(D3D12GraphicsPipeline {
                render_pipelines,
                vertex,
                fragment,
                cache_disabled: disable_cache,
            })
        }
    }

    pub fn recompile(
        &mut self,
        format: DXGI_FORMAT,
        root_sig: &D3D12RootSignature,
        device: &ID3D12Device,
    ) -> error::Result<()> {
        let (vertex, fragment) = unsafe {
            let library: IDxcUtils = DxcCreateInstance(&CLSID_DxcLibrary)?;
            let vertex = library.CreateBlobFromPinned(
                self.vertex.as_ptr().cast(),
                self.vertex.len() as u32,
                DXC_CP(0),
            )?;
            let fragment = library.CreateBlobFromPinned(
                self.fragment.as_ptr().cast(),
                self.fragment.len() as u32,
                DXC_CP(0),
            )?;
            (vertex, fragment)
        };

        let new_pipeline = Self::make_pipeline_state(
            device,
            &vertex.cast()?,
            &fragment.cast()?,
            root_sig,
            format,
            self.cache_disabled,
        )?;

        self.render_pipelines
            .insert(HashDxgiFormat(format), new_pipeline);

        Ok(())
    }

    pub fn has_format(&self, format: DXGI_FORMAT) -> bool {
        self.render_pipelines.contains_key(&HashDxgiFormat(format))
    }

    pub fn new_from_dxil(
        device: &ID3D12Device,
        library: &IDxcUtils,
        validator: &IDxcValidator,
        shader_assembly: &ShaderCompilerOutput<DxilObject, ()>,
        root_signature: &D3D12RootSignature,
        render_format: DXGI_FORMAT,
        disable_cache: bool,
    ) -> error::Result<D3D12GraphicsPipeline> {
        if shader_assembly.vertex.requires_runtime_data() {
            return Err(Direct3DOperationError(
                "Compiled DXIL Vertex shader needs unexpected runtime data",
            ));
        }
        if shader_assembly.fragment.requires_runtime_data() {
            return Err(Direct3DOperationError(
                "Compiled DXIL fragment shader needs unexpected runtime data",
            ));
        }

        let vertex_dxil = cache_shader_object(
            "dxil",
            &[shader_assembly.vertex.deref()],
            |&[source]| util::dxc_validate_shader(library, validator, source),
            |f| Ok(f),
            disable_cache,
        )?;

        let fragment_dxil = cache_shader_object(
            "dxil",
            &[shader_assembly.fragment.deref()],
            |&[source]| util::dxc_validate_shader(library, validator, source),
            |f| Ok(f),
            disable_cache,
        )?;

        Self::new_from_blobs(
            device,
            vertex_dxil,
            fragment_dxil,
            root_signature,
            render_format,
            disable_cache,
        )
    }

    pub fn new_from_hlsl(
        device: &ID3D12Device,
        library: &IDxcUtils,
        dxc: &IDxcCompiler,
        shader_assembly: &ShaderCompilerOutput<String, CrossHlslContext>,
        root_signature: &D3D12RootSignature,
        render_format: DXGI_FORMAT,
        disable_cache: bool,
    ) -> error::Result<D3D12GraphicsPipeline> {
        let vertex_dxil = cache_shader_object(
            "dxil",
            &[shader_assembly.vertex.as_bytes()],
            |&[source]| util::dxc_compile_shader(library, dxc, source, u16cstr!("vs_6_0")),
            |f| Ok(f),
            disable_cache,
        )?;

        let fragment_dxil = cache_shader_object(
            "dxil",
            &[shader_assembly.fragment.as_bytes()],
            |&[source]| util::dxc_compile_shader(library, dxc, source, u16cstr!("ps_6_0")),
            |f| Ok(f),
            disable_cache,
        )?;

        Self::new_from_blobs(
            device,
            vertex_dxil,
            fragment_dxil,
            root_signature,
            render_format,
            disable_cache,
        )
    }
}
