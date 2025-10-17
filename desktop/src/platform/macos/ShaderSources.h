/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Rendering module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

#pragma once

namespace YuchenUI {
namespace ShaderSources {

//==========================================================================================
// Basic Shaders - Rectangle rendering with rounded corners
//==========================================================================================
static constexpr const char* BasicShaders = R"(
#include <metal_stdlib>
using namespace metal;

struct RectVertexInput
{
    float2 position [[attribute(0)]];
    float2 rectOrigin [[attribute(1)]];
    float2 rectSize [[attribute(2)]];
    float4 cornerRadius [[attribute(3)]];
    float4 color [[attribute(4)]];
    float borderWidth [[attribute(5)]];
};

struct RectVertexOutput
{
    float4 position [[position]];
    float2 pixelPosition;
    float2 rectOrigin;
    float2 rectSize;
    float4 cornerRadius;
    float4 color;
    float borderWidth;
    float2 viewportSize;
};

struct ViewportUniforms
{
    float2 viewportSize;
};

vertex RectVertexOutput vertex_rect(RectVertexInput input [[stage_in]],
                                   constant ViewportUniforms& uniforms [[buffer(1)]])
{
    RectVertexOutput out;
    out.position = float4(input.position, 0.0, 1.0);
    
    float2 ndcPos = input.position.xy;
    out.pixelPosition = float2((ndcPos.x + 1.0) * 0.5, (1.0 - ndcPos.y) * 0.5);
    
    out.rectOrigin = input.rectOrigin;
    out.rectSize = input.rectSize;
    out.cornerRadius = input.cornerRadius;
    out.color = input.color;
    out.borderWidth = input.borderWidth;
    out.viewportSize = uniforms.viewportSize;
    
    return out;
}

float sdRoundedBox(float2 p, float2 rectCenter, float2 rectHalfSize, float4 cornerRadius)
{
    float4 r = float4(cornerRadius.y,cornerRadius.w,cornerRadius.x,cornerRadius.z);
    float2 q = p - rectCenter;
    r.xy = (q.x > 0.0) ? r.xy : r.zw;
    r.x  = (q.y > 0.0) ? r.x  : r.y;
    float2 d = abs(q) - rectHalfSize + r.x;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - r.x;
}

fragment float4 fragment_rect(RectVertexOutput in [[stage_in]])
{
    float2 rectCenter = in.rectOrigin + in.rectSize * 0.5;
    float2 rectHalfSize = in.rectSize * 0.5;
    
    float2 pixelPos = in.pixelPosition * in.viewportSize;
    
    float4 adjustedCornerRadius = max(in.cornerRadius + 0.5, float4(0.0));
    
    float outerDistance = sdRoundedBox(pixelPos, rectCenter, rectHalfSize, adjustedCornerRadius);
    
    if (in.borderWidth > 0.0)
    {
        float2 innerHalfSize = rectHalfSize - in.borderWidth;
        innerHalfSize = max(innerHalfSize, float2(0.0));
        
        float4 innerCornerRadius = max(adjustedCornerRadius - in.borderWidth, float4(0.0));
        float innerDistance = sdRoundedBox(pixelPos, rectCenter, innerHalfSize, innerCornerRadius);
        
        float edgeWidth = fwidth(outerDistance) * 0.5;
        float outerEdge = smoothstep(edgeWidth, -edgeWidth, outerDistance);
        float innerEdge = smoothstep(-edgeWidth, edgeWidth, innerDistance);
        
        float alpha = outerEdge * innerEdge;
        
        if (alpha < 0.01) discard_fragment();
        
        float4 color = in.color;
        color.a *= alpha;
        return color;
    }
    else
    {
        float edgeWidth = fwidth(outerDistance) * 0.5;
        float alpha = smoothstep(edgeWidth, -edgeWidth, outerDistance);
        
        if (alpha < 0.01) discard_fragment();
        
        float4 color = in.color;
        color.a *= alpha;
        return color;
    }
}
)";

//==========================================================================================
// Text Shaders - Text rendering with various modes
//==========================================================================================
static constexpr const char* TextShaders = R"(
#include <metal_stdlib>
using namespace metal;

struct ViewportUniforms {
    float2 viewportSize;
};

struct TextVertexInput {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct TextVertexOutput {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

vertex TextVertexOutput vertex_text(TextVertexInput input [[stage_in]],
                                   constant ViewportUniforms& uniforms [[buffer(1)]]) {
    TextVertexOutput out;
    
    float2 ndc = (input.position / uniforms.viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    out.position = float4(ndc, 0.0, 1.0);
    out.texCoord = input.texCoord;
    out.color = input.color;
    
    return out;
}

fragment float4 fragment_text(TextVertexOutput input [[stage_in]],
                             texture2d<float> glyphTexture [[texture(0)]],
                             sampler glyphSampler [[sampler(0)]]) {
    float alpha = glyphTexture.sample(glyphSampler, input.texCoord).r;
    
    float4 color = input.color;
    color.a *= alpha;
    
    if (color.a < 0.01) {
        discard_fragment();
    }
    
    return color;
}

)";

//==========================================================================================
// Image Shaders - Image rendering
//==========================================================================================
static constexpr const char* ImageShaders = R"(
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
)";

//==========================================================================================
// Shape Shaders - Lines, triangles, and circles
//==========================================================================================
static constexpr const char* ShapeShaders = R"(
#include <metal_stdlib>
using namespace metal;

struct ViewportUniforms
{
    float2 viewportSize;
};

struct ShapeVertex
{
    float2 position [[attribute(0)]];
    float4 color [[attribute(1)]];
};

struct ShapeVertexOut
{
    float4 position [[position]];
    float4 color;
};

vertex ShapeVertexOut vertex_shape(ShapeVertex in [[stage_in]],
                                   constant ViewportUniforms& uniforms [[buffer(1)]])
{
    ShapeVertexOut out;
    
    float2 ndc = (in.position / uniforms.viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    out.position = float4(ndc, 0.0, 1.0);
    out.color = in.color;
    
    return out;
}

fragment float4 fragment_shape(ShapeVertexOut in [[stage_in]])
{
    return in.color;
}

struct CircleVertex
{
    float2 position [[attribute(0)]];
    float2 center [[attribute(1)]];
    float radius [[attribute(2)]];
    float borderWidth [[attribute(3)]];
    float4 color [[attribute(4)]];
};

struct CircleVertexOut
{
    float4 position [[position]];
    float2 center;
    float2 pixelPos;
    float radius;
    float borderWidth;
    float4 color;
};

vertex CircleVertexOut vertex_circle(CircleVertex in [[stage_in]],
                                     constant ViewportUniforms& uniforms [[buffer(1)]])
{
    CircleVertexOut out;
    
    float2 ndc = (in.position / uniforms.viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    out.position = float4(ndc, 0.0, 1.0);
    out.center = in.center;
    out.pixelPos = in.position;
    out.radius = in.radius;
    out.borderWidth = in.borderWidth;
    out.color = in.color;
    
    return out;
}

fragment float4 fragment_circle(CircleVertexOut in [[stage_in]])
{
    float dist = distance(in.pixelPos, in.center);
    
    if (in.borderWidth > 0.0)
    {
        float outerRadius = in.radius;
        float innerRadius = in.radius - in.borderWidth;
        
        if (dist > outerRadius)
        {
            discard_fragment();
        }
        
        if (dist < innerRadius)
        {
            discard_fragment();
        }
        
        float outerEdge = smoothstep(outerRadius - 1.0, outerRadius, dist);
        float innerEdge = smoothstep(innerRadius, innerRadius + 1.0, dist);
        float alpha = (1.0 - outerEdge) * innerEdge;
        
        return float4(in.color.rgb, in.color.a * alpha);
    }
    else
    {
        if (dist > in.radius)
        {
            discard_fragment();
        }
        
        float edge = smoothstep(in.radius - 1.0, in.radius, dist);
        float alpha = 1.0 - edge;
        
        return float4(in.color.rgb, in.color.a * alpha);
    }
}
)";

} // namespace ShaderSources
} // namespace YuchenUI
