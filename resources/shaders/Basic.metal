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
    
    // FIXME: Temporary Workaround — Increase the corner radius by 0.5 logical pixels to compensate for rendering offset
    // The rendered circle is slightly larger than the specified size, so add 0.5 to match the expected dimensions
    // Root Cause: Further investigation needed into SDF distance calculation or smoothing step centering issues
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
