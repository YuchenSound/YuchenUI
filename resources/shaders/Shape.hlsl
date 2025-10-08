// Shape.hlsl - Shape rendering (lines and circles)
cbuffer ViewportUniforms : register(b0)
{
    float2 viewportSize;
    float2 _padding;
};

// ========== Shape (Lines/Polygons) ==========
struct ShapeVertex
{
    float2 position : POSITION;
    float4 color : COLOR;
};

struct ShapeVertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

ShapeVertexOut VSShape(ShapeVertex input)
{
    ShapeVertexOut output;
    
    float2 ndc = (input.position / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    output.position = float4(ndc, 0.0, 1.0);
    output.color = input.color;
    
    return output;
}

float4 PSShape(ShapeVertexOut input) : SV_TARGET
{
    return input.color;
}

// ========== Circle ==========
struct CircleVertex
{
    float2 position : POSITION0;
    float2 center : POSITION1;
    float radius : TEXCOORD0;
    float borderWidth : TEXCOORD1;
    float4 color : COLOR;
};

struct CircleVertexOut
{
    float4 position : SV_POSITION;
    float2 center : TEXCOORD0;
    float2 pixelPos : TEXCOORD1;
    float radius : TEXCOORD2;
    float borderWidth : TEXCOORD3;
    float4 color : COLOR;
};

CircleVertexOut VSCircle(CircleVertex input)
{
    CircleVertexOut output;
    
    float2 ndc = (input.position / viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    
    output.position = float4(ndc, 0.0, 1.0);
    output.center = input.center;
    output.pixelPos = input.position;
    output.radius = input.radius;
    output.borderWidth = input.borderWidth;
    output.color = input.color;
    
    return output;
}

float4 PSCircle(CircleVertexOut input) : SV_TARGET
{
    float dist = distance(input.pixelPos, input.center);
    
    if (input.borderWidth > 0.0)
    {
        float outerRadius = input.radius;
        float innerRadius = input.radius - input.borderWidth;
        
        if (dist > outerRadius)
            discard;
        
        if (dist < innerRadius)
            discard;
        
        float outerEdge = smoothstep(outerRadius - 1.0, outerRadius, dist);
        float innerEdge = smoothstep(innerRadius, innerRadius + 1.0, dist);
        float alpha = (1.0 - outerEdge) * innerEdge;
        
        return float4(input.color.rgb, input.color.a * alpha);
    }
    else
    {
        if (dist > input.radius)
            discard;
        
        float edge = smoothstep(input.radius - 1.0, input.radius, dist);
        float alpha = 1.0 - edge;
        
        return float4(input.color.rgb, input.color.a * alpha);
    }
}