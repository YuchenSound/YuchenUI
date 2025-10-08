// Basic.hlsl - Rectangle rendering with rounded corners
cbuffer ViewportUniforms : register(b0)
{
    float2 viewportSize;
    float2 _padding;
};

struct RectVertexInput
{
    float2 position : POSITION0;
    float2 rectOrigin : POSITION1;
    float2 rectSize : TEXCOORD0;
    float4 cornerRadius : TEXCOORD1;
    float4 color : COLOR0;
    float borderWidth : TEXCOORD2;
};

struct RectVertexOutput
{
    float4 position : SV_POSITION;
    float2 pixelPosition : TEXCOORD0;
    float2 rectOrigin : TEXCOORD1;
    float2 rectSize : TEXCOORD2;
    float4 cornerRadius : TEXCOORD3;
    float4 color : COLOR0;
    float borderWidth : TEXCOORD4;
    float2 viewportSize : TEXCOORD5;
};

RectVertexOutput VSMain(RectVertexInput input)
{
    RectVertexOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    
    float2 ndcPos = input.position.xy;
    output.pixelPosition = float2((ndcPos.x + 1.0) * 0.5, (1.0 - ndcPos.y) * 0.5);
    
    output.rectOrigin = input.rectOrigin;
    output.rectSize = input.rectSize;
    output.cornerRadius = input.cornerRadius;
    output.color = input.color;
    output.borderWidth = input.borderWidth;
    output.viewportSize = viewportSize;
    
    return output;
}

float sdRoundedBox(float2 p, float2 rectCenter, float2 rectHalfSize, float4 cornerRadius)
{
    float4 r = float4(cornerRadius.y, cornerRadius.w, cornerRadius.x, cornerRadius.z);
    float2 q = p - rectCenter;
    r.xy = (q.x > 0.0) ? r.xy : r.zw;
    r.x = (q.y > 0.0) ? r.x : r.y;
    float2 d = abs(q) - rectHalfSize + r.x;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - r.x;
}

float4 PSMain(RectVertexOutput input) : SV_TARGET
{
    float2 rectCenter = input.rectOrigin + input.rectSize * 0.5;
    float2 rectHalfSize = input.rectSize * 0.5;
    
    float2 pixelPos = input.pixelPosition * input.viewportSize;
    
    float4 adjustedCornerRadius = max(input.cornerRadius + 0.5, float4(0.0, 0.0, 0.0, 0.0));
    
    float outerDistance = sdRoundedBox(pixelPos, rectCenter, rectHalfSize, adjustedCornerRadius);
    
    if (input.borderWidth > 0.0)
    {
        float2 innerHalfSize = rectHalfSize - input.borderWidth;
        innerHalfSize = max(innerHalfSize, float2(0.0, 0.0));
        
        float4 innerCornerRadius = max(adjustedCornerRadius - input.borderWidth, float4(0.0, 0.0, 0.0, 0.0));
        float innerDistance = sdRoundedBox(pixelPos, rectCenter, innerHalfSize, innerCornerRadius);
        
        float edgeWidth = fwidth(outerDistance) * 0.5;
        float outerEdge = smoothstep(edgeWidth, -edgeWidth, outerDistance);
        float innerEdge = smoothstep(-edgeWidth, edgeWidth, innerDistance);
        
        float alpha = outerEdge * innerEdge;
        
        if (alpha < 0.01)
            discard;
        
        float4 color = input.color;
        color.a *= alpha;
        return color;
    }
    else
    {
        float edgeWidth = fwidth(outerDistance) * 0.5;
        float alpha = smoothstep(edgeWidth, -edgeWidth, outerDistance);
        
        if (alpha < 0.01)
            discard;
        
        float4 color = input.color;
        color.a *= alpha;
        return color;
    }
}