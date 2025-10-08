// Text.hlsl - Text rendering
cbuffer ViewportUniforms : register(b0)
{
    float2 viewportSize;
    float2 _padding;
};

Texture2D glyphTexture : register(t0);
SamplerState glyphSampler : register(s0);

struct TextVertexInput
{
    float2 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
};

struct TextVertexOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color : COLOR;
};

TextVertexOutput VSMain(TextVertexInput input)
{
    TextVertexOutput output;
    
    float2 ndc = (input.position / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    output.position = float4(ndc, 0.0, 1.0);
    output.texCoord = input.texCoord;
    output.color = input.color;
    
    return output;
}

// Basic text rendering
float4 PSMain(TextVertexOutput input) : SV_TARGET
{
    float alpha = glyphTexture.Sample(glyphSampler, input.texCoord).r;
    
    float4 color = input.color;
    color.a *= alpha;
    
    if (color.a < 0.01)
        discard;
    
    return color;
}

// Subpixel rendering
float4 PSSubpixel(TextVertexOutput input) : SV_TARGET
{
    float width, height;
    glyphTexture.GetDimensions(width, height);
    float2 texelSize = 1.0 / float2(width, height);
    
    float3 subpixelWeights = float3(0.3, 0.59, 0.11);
    
    float alpha_r = glyphTexture.Sample(glyphSampler, input.texCoord + float2(-texelSize.x * 0.33, 0)).r;
    float alpha_g = glyphTexture.Sample(glyphSampler, input.texCoord).r;
    float alpha_b = glyphTexture.Sample(glyphSampler, input.texCoord + float2(texelSize.x * 0.33, 0)).r;
    
    float alpha = dot(float3(alpha_r, alpha_g, alpha_b), subpixelWeights);
    
    float4 color = input.color;
    color.a *= alpha;
    
    if (color.a < 0.01)
        discard;
    
    return color;
}

// SDF rendering
float4 PSSDF(TextVertexOutput input) : SV_TARGET
{
    float distance = glyphTexture.Sample(glyphSampler, input.texCoord).r;
    
    const float smoothness = 1.0 / 16.0;
    const float threshold = 0.5;
    
    float alpha = smoothstep(threshold - smoothness, threshold + smoothness, distance);
    
    float4 color = input.color;
    color.a *= alpha;
    
    if (color.a < 0.01)
        discard;
    
    return color;
}