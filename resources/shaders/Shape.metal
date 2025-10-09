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
