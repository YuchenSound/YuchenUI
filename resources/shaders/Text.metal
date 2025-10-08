#include <metal_stdlib>
using namespace metal;

// 视口统一变量
struct ViewportUniforms {
    float2 viewportSize;
};

// 文本渲染顶点输入
struct TextVertexInput {
    float2 position [[attribute(0)]];       // 屏幕位置
    float2 texCoord [[attribute(1)]];       // 纹理坐标
    float4 color [[attribute(2)]];          // 文本颜色
};

// 文本渲染顶点输出
struct TextVertexOutput {
    float4 position [[position]];           // 裁剪空间位置
    float2 texCoord;                        // 纹理坐标
    float4 color;                           // 文本颜色
};

// 文本渲染顶点着色器
vertex TextVertexOutput vertex_text(TextVertexInput input [[stage_in]],
                                   constant ViewportUniforms& uniforms [[buffer(1)]]) {
    TextVertexOutput out;
    
    // 屏幕坐标转换为标准化设备坐标 (NDC)
    float2 ndc = (input.position / uniforms.viewportSize) * 2.0 - 1.0;
    ndc.y = -ndc.y;  // 翻转 Y 轴（Metal 坐标系）
    
    out.position = float4(ndc, 0.0, 1.0);
    out.texCoord = input.texCoord;
    out.color = input.color;
    
    return out;
}

// 文本渲染片段着色器 - 基础版本
fragment float4 fragment_text(TextVertexOutput input [[stage_in]],
                             texture2d<float> glyphTexture [[texture(0)]],
                             sampler glyphSampler [[sampler(0)]]) {
    // 从字形图集纹理中采样
    float alpha = glyphTexture.sample(glyphSampler, input.texCoord).r;
    
    // 应用文本颜色
    float4 color = input.color;
    color.a *= alpha;
    
    // 如果透明度太低，丢弃像素
    if (color.a < 0.01) {
        discard_fragment();
    }
    
    return color;
}

// 文本渲染片段着色器 - 子像素渲染版本
fragment float4 fragment_text_subpixel(TextVertexOutput input [[stage_in]],
                                      texture2d<float> glyphTexture [[texture(0)]],
                                      sampler glyphSampler [[sampler(0)]]) {
    // 子像素渲染需要特殊的采样策略
    float2 texSize = float2(glyphTexture.get_width(), glyphTexture.get_height());
    float2 texelSize = 1.0 / texSize;
    
    // 使用更精细的采样来获取子像素信息
    float3 subpixelWeights = float3(0.3, 0.59, 0.11);  // RGB 权重
    
    float alpha_r = glyphTexture.sample(glyphSampler, input.texCoord + float2(-texelSize.x * 0.33, 0)).r;
    float alpha_g = glyphTexture.sample(glyphSampler, input.texCoord).r;
    float alpha_b = glyphTexture.sample(glyphSampler, input.texCoord + float2(texelSize.x * 0.33, 0)).r;
    
    // 计算加权透明度
    float alpha = dot(float3(alpha_r, alpha_g, alpha_b), subpixelWeights);
    
    float4 color = input.color;
    color.a *= alpha;
    
    if (color.a < 0.01) {
        discard_fragment();
    }
    
    return color;
}

// 文本渲染片段着色器 - SDF（Signed Distance Field）版本
fragment float4 fragment_text_sdf(TextVertexOutput input [[stage_in]],
                                 texture2d<float> sdfTexture [[texture(0)]],
                                 sampler sdfSampler [[sampler(0)]]) {
    // 从 SDF 纹理中采样距离值
    float distance = sdfTexture.sample(sdfSampler, input.texCoord).r;
    
    // SDF 渲染参数
    const float smoothness = 1.0 / 16.0;  // 平滑参数
    const float threshold = 0.5;          // 阈值
    
    // 计算透明度
    float alpha = smoothstep(threshold - smoothness, threshold + smoothness, distance);
    
    float4 color = input.color;
    color.a *= alpha;
    
    if (color.a < 0.01) {
        discard_fragment();
    }
    
    return color;
}

// 文本渲染片段着色器 - 带描边效果的 SDF 版本
fragment float4 fragment_text_sdf_outline(TextVertexOutput input [[stage_in]],
                                         texture2d<float> sdfTexture [[texture(0)]],
                                         sampler sdfSampler [[sampler(0)]],
                                         constant float4& outlineColor [[buffer(2)]],
                                         constant float& outlineWidth [[buffer(3)]]) {
    float distance = sdfTexture.sample(sdfSampler, input.texCoord).r;
    
    const float smoothness = 1.0 / 16.0;
    const float threshold = 0.5;
    
    // 文本主体
    float textAlpha = smoothstep(threshold - smoothness, threshold + smoothness, distance);
    
    // 描边
    float outlineAlpha = smoothstep((threshold - outlineWidth) - smoothness, 
                                   (threshold - outlineWidth) + smoothness, distance);
    outlineAlpha -= textAlpha;  // 减去文本部分，只保留描边
    
    // 混合文本和描边
    float4 finalColor = mix(outlineColor, input.color, textAlpha);
    finalColor.a = max(textAlpha, outlineAlpha);
    
    if (finalColor.a < 0.01) {
        discard_fragment();
    }
    
    return finalColor;
}

// 文本渲染片段着色器 - 带阴影效果版本
fragment float4 fragment_text_shadow(TextVertexOutput input [[stage_in]],
                                    texture2d<float> glyphTexture [[texture(0)]],
                                    sampler glyphSampler [[sampler(0)]],
                                    constant float2& shadowOffset [[buffer(2)]],
                                    constant float4& shadowColor [[buffer(3)]]) {
    // 主文本采样
    float mainAlpha = glyphTexture.sample(glyphSampler, input.texCoord).r;
    
    // 阴影采样（偏移采样）
    float2 shadowTexCoord = input.texCoord + shadowOffset;
    float shadowAlpha = 0.0;
    
    // 检查阴影采样坐标是否在有效范围内
    if (shadowTexCoord.x >= 0.0 && shadowTexCoord.x <= 1.0 && 
        shadowTexCoord.y >= 0.0 && shadowTexCoord.y <= 1.0) {
        shadowAlpha = glyphTexture.sample(glyphSampler, shadowTexCoord).r;
    }
    
    // 混合主文本和阴影
    float4 mainColor = input.color;
    mainColor.a *= mainAlpha;
    
    float4 shadow = shadowColor;
    shadow.a *= shadowAlpha;
    
    // 阴影在后，主文本在前
    float4 finalColor = mix(shadow, mainColor, mainColor.a);
    finalColor.a = max(mainColor.a, shadow.a * (1.0 - mainColor.a));
    
    if (finalColor.a < 0.01) {
        discard_fragment();
    }
    
    return finalColor;
}

// 调试用：显示纹理坐标的着色器
fragment float4 fragment_text_debug_uv(TextVertexOutput input [[stage_in]]) {
    return float4(input.texCoord.x, input.texCoord.y, 0.0, 1.0);
}

// 调试用：显示字形图集的着色器
fragment float4 fragment_text_debug_atlas(TextVertexOutput input [[stage_in]],
                                         texture2d<float> glyphTexture [[texture(0)]],
                                         sampler glyphSampler [[sampler(0)]]) {
    float alpha = glyphTexture.sample(glyphSampler, input.texCoord).r;
    return float4(alpha, alpha, alpha, 1.0);  // 显示为灰度
}