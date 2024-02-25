use librashader_reflect::reflect::semantics::BindingStage;
use wgpu::ShaderStages;

pub fn binding_stage_to_wgpu_stage(stage_mask: BindingStage) -> ShaderStages {
    let mut mask = ShaderStages::empty();
    if stage_mask.contains(BindingStage::VERTEX) {
        mask |= ShaderStages::VERTEX;
    }

    if stage_mask.contains(BindingStage::FRAGMENT) {
        mask |= ShaderStages::FRAGMENT;
    }

    mask
}
