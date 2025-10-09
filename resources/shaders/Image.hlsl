// Image.hlsl - Image rendering
cbuffer ViewportUniforms : register(b0)
{
    float2 viewportSize;
    float2 _padding;
};

Texture2D colorTexture : register(t0);
SamplerState textureSampler : register(s0);

struct ImageVertex
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct ImageFragmentInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

ImageFragmentInput VSMain(ImageVertex input)
{
    ImageFragmentInput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.texCoord = input.texCoord;
    return output;
}

float4 PSMain(ImageFragmentInput input) : SV_TARGET
{
    return colorTexture.Sample(textureSampler, input.texCoord);
}