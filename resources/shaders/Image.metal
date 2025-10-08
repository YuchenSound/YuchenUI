#include <metal_stdlib>
using namespace metal;

struct ImageVertex {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct ImageFragmentInput {
    float4 position [[position]];
    float2 texCoord;
};

struct ViewportUniforms {
    float2 viewportSize;
};

vertex ImageFragmentInput vertex_image(
    ImageVertex in [[stage_in]],
    constant ViewportUniforms& uniforms [[buffer(1)]])
{
    ImageFragmentInput out;
    out.position = float4(in.position, 0.0, 1.0);
    out.texCoord = in.texCoord;
    return out;
}

fragment float4 fragment_image(
    ImageFragmentInput in [[stage_in]],
    texture2d<float> colorTexture [[texture(0)]],
    sampler textureSampler [[sampler(0)]])
{
    return colorTexture.sample(textureSampler, in.texCoord);
}
