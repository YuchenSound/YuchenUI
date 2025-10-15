/*******************************************************************************************
**
** YuchenUI - Modern C++ GUI Framework
**
** Copyright (C) 2025 Yuchen Wei
** Contact: https://github.com/YuchenSound/YuchenUI
**
** This file is part of the YuchenUI Widgets module.
**
** $YUCHEN_BEGIN_LICENSE:MIT$
** Licensed under the MIT License
** $YUCHEN_END_LICENSE$
**
********************************************************************************************/

//==========================================================================================
/** @file LevelMeter.h
    
    Broadcast-standard audio level meter with VU/PPM/K-metering support.
    
    Architecture:
    - LevelDataManager: Per-channel level tracking with decay and peak hold
    - MeterScale: dB-to-position mapping with non-linear scales (Sample Peak, K-12, etc.)
    - MeterRenderer: Theme-aware rendering with 3D lighting effects
    - BlendedColorCache: Pre-computed color blending for performance
    
    Scale characteristics:
    - SAMPLE_PEAK: Non-linear (0 to -40dB = 81.25% height, optimized for digital audio)
    - K12/K14: ITU-R BS.1770 loudness standards (linear)
    - VU: Classic analog metering (-20dB to +3dB range)
    - LINEAR_DB: Uniform dB spacing for analysis
    
    Usage:
    @code
    LevelMeter meter(context, Rect(10, 10, 100, 240), 2, ScaleType::SAMPLE_PEAK);
    meter.updateLevels({-12.0f, -18.0f});  // Update per frame
    @endcode
*/

#pragma once

#include "YuchenUI/YuchenUI.h"
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <array>
#include <string>

namespace YuchenUI {

//==========================================================================================

enum class ScaleType
{
    SAMPLE_PEAK,  // Non-linear scale optimized for digital peaks
    K12,          // ITU-R BS.1770 with +12dB headroom
    K14,          // ITU-R BS.1770 with +14dB headroom
    VU,           // Classic VU meter range (-20 to +3dB)
    LINEAR_DB     // Linear dB spacing for measurement
};

//==========================================================================================

struct ScaleTick
{
    float db;          // dB value at this position
    float position;    // Normalized vertical position [0,1]
    std::string label; // Display text (e.g., "0", "-6")
    bool isMajor;      // Draw as major tick (longer line)
    
    ScaleTick(float d, float pos, const std::string& lbl = "", bool major = true)
        : db(d), position(pos), label(lbl), isMajor(major) {}
};

//==========================================================================================
/** Layout constants for meter rendering. */
struct MeterDimensions {
    static constexpr float DEFAULT_HEIGHT = 224.0f;
    static constexpr float MONO_CHANNEL_WIDTH = 8.0f;
    static constexpr float STEREO_CHANNEL_WIDTH = 7.0f;
    static constexpr float MULTI_CHANNEL_WIDTH = 6.0f;
    static constexpr float CHANNEL_SPACING = -1.0f;  // Negative = channels overlap by 1px
    static constexpr float SCALE_WIDTH = 13.0f;
    static constexpr float PEAK_LINE_HEIGHT = 0.5f;
    static constexpr float PEAK_INDICATOR_HEIGHT = 6.0f;
    static constexpr float PEAK_INDICATOR_SPACING = 3.0f;
    
    // Returns channel width based on total channel count (for density optimization)
    static float getChannelWidth(size_t totalChannelCount);
    static constexpr float getTotalHeight();
    static float getChannelGroupWidth(size_t channelCount);
    static float getTotalWidth(size_t channelCount);
};

//==========================================================================================
/** Color zone thresholds in dB. */
struct MeterThresholds
{
    float normalToWarning = -20.0f;  // Green to yellow transition
    float warningToPeak = -6.0f;     // Yellow to red transition
    float peakIndicator = -0.1f;     // Activates peak indicator lamp
    
    bool isValid() const;
    
    enum Region { NORMAL, WARNING, PEAK };
    Region getRegion(float db) const;
};

//==========================================================================================
/** Ballistics and decay behavior. */
struct MeterBehavior
{
    float decayRateDbPerSec = 80.0f;   // Decay speed for display level
    float peakHoldTimeMs = 3000.0f;    // Peak hold duration before decay
    float updateEpsilon = 0.1f;        // Minimum change threshold to trigger update
};

//==========================================================================================

class MeterConfig
{
public:
    MeterConfig() = default;
    
    const MeterThresholds& getThresholds() const { return thresholds_; }
    const MeterBehavior& getBehavior() const { return behavior_; }
    
    void setThresholds(const MeterThresholds& thresholds);
    void setBehavior(const MeterBehavior& behavior);
    void setWarningThreshold(float db) { thresholds_.normalToWarning = db; }
    void setPeakThreshold(float db) { thresholds_.warningToPeak = db; }
    void setDecayRate(float dbPerSec) { behavior_.decayRateDbPerSec = dbPerSec; }
    void setPeakHoldTime(float timeMs) { behavior_.peakHoldTimeMs = timeMs; }
    
    bool isValid() const;
    static MeterConfig createDefault();
    
private:
    MeterThresholds thresholds_;
    MeterBehavior behavior_;
};

//==========================================================================================
/** Maps dB values to normalized vertical positions based on metering standard. */
class MeterScale
{
public:
    explicit MeterScale(ScaleType type = ScaleType::SAMPLE_PEAK);
    
    // Bidirectional mapping between dB and normalized position [0,1]
    float mapDbToPosition(float db) const;
    float mapPositionToDb(float position) const;
    
    const std::vector<ScaleTick>& getTickMarks() const { return ticks_; }
    ScaleType getType() const { return type_; }
    float getMinDb() const { return minDb_; }
    float getMaxDb() const { return maxDb_; }
    std::string getTypeName() const;
    
    static MeterScale create(ScaleType type);
    
private:
    ScaleType type_;
    float minDb_;
    float maxDb_;
    std::vector<ScaleTick> ticks_;
    std::function<float(float)> dbToPos_;  // Custom mapping function per scale type
    std::function<float(float)> posToDb_;
    
    // Scale-specific initialization functions
    void initializeSamplePeak();
    void initializeK12();
    void initializeK14();
    void initializeVU();
    void initializeLinearDb();
    
    static float linearMap(float value, float inMin, float inMax, float outMin, float outMax);
    static float clampDb(float db, float minDb, float maxDb);
};

//==========================================================================================
/** Single channel state with ballistic decay and peak hold. */
class ChannelLevelData
{
public:
    ChannelLevelData();
    ChannelLevelData(const ChannelLevelData& other);
    ChannelLevelData(ChannelLevelData&& other) noexcept;
    ChannelLevelData& operator=(const ChannelLevelData& other);
    ChannelLevelData& operator=(ChannelLevelData&& other) noexcept;
    
    // Updates level with time-based decay (deltaTimeMs for frame-independent decay)
    void updateLevel(float levelDb, float deltaTimeMs);
    void reset();
    
    float getCurrentLevel() const { return currentLevelDb_; }
    float getDisplayLevel() const { return displayLevelDb_; }  // With decay applied
    float getPeakLevel() const { return peakLevelDb_; }        // Held peak value
    bool isPeakIndicatorActive() const { return peakIndicatorTimer_ > 0.0f; }
    
    void setDecayRate(float dbPerSec) { decayDbPerSec_ = dbPerSec; }
    void setPeakHoldTime(float timeMs) { peakHoldTimeMs_ = timeMs; }
    
private:
    static constexpr float MIN_DB = -144.0f;
    static constexpr float MAX_DB = 0.0f;
    static constexpr float PEAK_INDICATOR_HOLD_TIME_MS = 3000.0f;
    
    float currentLevelDb_;      // Instantaneous input level
    float displayLevelDb_;      // Decaying display level
    float peakLevelDb_;         // Held peak with separate decay
    float peakHoldTimer_;       // Countdown before peak decay starts
    float peakIndicatorTimer_;  // Countdown for peak lamp
    float decayDbPerSec_;
    float peakHoldTimeMs_;
    
    float clampDb(float db) const;
};

//==========================================================================================
/** Manages multiple channels with synchronized timing. */
class LevelDataManager
{
public:
    explicit LevelDataManager(size_t channelCount = 2);
    
    void updateLevels(const std::vector<float>& levels);
    void updateLevel(size_t channel, float levelDb);
    void reset();
    
    size_t getChannelCount() const { return channels_.size(); }
    const ChannelLevelData& getChannel(size_t index) const;
    
    void setDecayRate(float dbPerSec);
    void setPeakHoldTime(float timeMs);
    void setChannelCount(size_t count);
    
    // Optional control voltage display for modular synthesis (-35dB to 0dB range)
    void updateControlVoltage(float levelDb);
    float getControlVoltageLevel() const;
    void clearControlVoltage();
    
private:
    std::vector<ChannelLevelData> channels_;
    std::unique_ptr<ChannelLevelData> controlVoltageData_;
    
    // Calculates frame delta with clamping (prevents huge jumps on frame drops)
    float getFrameDeltaTime() const;
};

//==========================================================================================
/** Pre-computed color blending for 3D cylindrical lighting effect. */
class BlendedColorCache
{
public:
    BlendedColorCache();
    
    void initialize(size_t totalChannelCount);
    bool isInitialized() const { return initialized_; }
    Vec4 getBlendedColor(const Vec4& baseColor, float x, bool isWarningRegion) const;
private:
    struct TextureData { std::vector<Vec4> pixels; };
    TextureData normalTexture_;
    TextureData warningTexture_;
    float channelWidth_;
    int blendTexturePixelCount_;         // Number of pixels in blend texture
    size_t initializedForChannelCount_;  // Track which channel count we initialized for
    bool initialized_;
    mutable std::unordered_map<uint64_t, Vec4> blendCache_;  // Cache key: (color, x, region)
    
    void initializeTextures();
    Vec4 sampleTexture(const TextureData& texture, float x) const;
    Vec4 multiplyBlend(const Vec4& baseColor, const Vec4& overlayColor) const;
    uint64_t getCacheKey(const Vec4& baseColor, float x, bool isWarningRegion) const;
};

//==========================================================================================
/** Pre-computed layout information for channel rendering. */
struct ChannelRenderInfo
{
    Rect meterRect;                 // Main meter bar area
    Rect peakIndicatorRect;         // Peak lamp area
    float displayLevel01;           // Display level [0,1]
    float peakLevel01;              // Peak marker position [0,1]
    float displayLevelDb;           // Display level in dB
    bool isPeakIndicatorActive;     // Peak lamp state
    size_t channelIndex;
    size_t totalChannelCount;
};

//==========================================================================================
/** Renders meter components with theme colors and 3D effects. */
class MeterRenderer
{
public:
    MeterRenderer();
    
    void setContext(UIContext* context) { m_context = context; }
    void setScale(const MeterScale* scale) { scale_ = scale; }
    void setConfig(const MeterConfig* config) { config_ = config; }
    
    void renderChannels(RenderList& cmdList, const Vec2& startPos, const Vec2& totalSize,
                       const LevelDataManager& levelData, bool showControlVoltage = true);
    void renderScale(RenderList& cmdList, const Rect& scaleRect, const Rect& referenceRect);
    std::vector<ChannelRenderInfo> calculateChannelLayout(const Vec2& startPos,const Vec2& totalSize,const LevelDataManager& levelData) const;
private:
    UIContext* m_context;
    const MeterScale* scale_;
    const MeterConfig* config_;
    BlendedColorCache blendCache_;
    
    void renderSingleChannel(RenderList& cmdList, const ChannelRenderInfo& info);
    void renderChannelBackground(RenderList& cmdList, const Rect& rect, float displayLevelDb, size_t totalChannelCount);
    void renderChannelFill(RenderList& cmdList, const Rect& rect, float level01, float displayLevelDb, size_t totalChannelCount);
    void renderChannelFrame(RenderList& cmdList, const Rect& rect);
    void renderChannelPeak(RenderList& cmdList, const Rect& rect, float peakLevel01, float displayLevelDb, size_t totalChannelCount);
    void renderInternalScale(RenderList& cmdList, const Rect& rect, float displayLevelDb, size_t totalChannelCount);
    void renderPeakIndicator(RenderList& cmdList, const Rect& rect, bool isActive);
    void renderPeakIndicatorFrame(RenderList& cmdList, const Rect& rect);
    void renderControlVoltage(RenderList& cmdList, const Vec2& startPos, const Vec2& totalSize, const LevelDataManager& levelData);

    Vec4 getLevelColor(float db) const;
    float dbToPosition01(float db) const;
    float getWarningThreshold01() const;
    float getPeakThreshold01() const;
    bool isReady() const;
    void updateBlendCache(size_t totalChannelCount);
    
    void drawPeakLine(RenderList& cmdList, const Rect& rect, float peakLevel01, const Vec4& color, float lineHeight);
    void drawInternalScaleTicks(RenderList& cmdList, const Rect& rect, const std::vector<ScaleTick>& ticks, float currentDisplayLevel, size_t totalChannelCount);
    void drawScaleTicks(RenderList& cmdList, const Rect& scaleRect, const Rect& meterRect, const std::vector<ScaleTick>& ticks);
    Rect calculateChannelRect(const Vec2& startPos, const Vec2& totalSize, size_t channelIndex, size_t totalChannels) const;
    float pixelAlign(float value) const;
};

//==========================================================================================
/**
    Broadcast-standard audio level meter widget.
    
    Thread-safe level updates: updateLevels() can be called from audio thread.
    All other methods must be called from UI thread only.
    
    Performance: Supports 1-35 channels at 60fps. Uses pre-computed color blending
    and texture-based 3D lighting for efficient GPU rendering.
    
    @see MeterConfig, MeterScale, UIStyle::getLevelMeterColors()
*/
class LevelMeter : public Widget
{
public:
    explicit LevelMeter(UIContext* context, const Rect& bounds, size_t channelCount = 2,
                       ScaleType scaleType = ScaleType::SAMPLE_PEAK);
    explicit LevelMeter(UIContext* context, const Rect& bounds, const MeterConfig& config,
                       size_t channelCount = 2, ScaleType scaleType = ScaleType::SAMPLE_PEAK);
    virtual ~LevelMeter();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override
    {
        return Widget::handleMouseMove(position, offset);
    }
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override
    {
        return Widget::handleMouseClick(position, pressed, offset);
    }
    
    // Level input (typically called from audio thread)
    void updateLevels(const std::vector<float>& levels);
    void updateLevel(size_t channel, float levelDb);
    void reset();
    
    // Configuration (UI thread only)
    void setChannelCount(size_t count);
    void setScaleType(ScaleType type);
    void setConfig(const MeterConfig& config);
    void setThresholds(float warningDb, float peakDb);
    void setDecayRate(float dbPerSec);
    void setPeakHoldTime(float timeMs);
    
    size_t getChannelCount() const;
    Vec2 getRecommendedSize() const;
    ScaleType getScaleType() const { return scale_.getType(); }
    std::string getScaleTypeName() const { return scale_.getTypeName(); }
    
    // Optional control voltage display (modular synthesis envelope follower)
    void updateControlVoltage(float levelDb);
    void setShowControlVoltage(bool show);
    bool getShowControlVoltage() const;
    
private:
    UIContext* m_context;
    LevelDataManager levelData_;
    MeterConfig config_;
    MeterScale scale_;
    mutable MeterRenderer renderer_;
    bool showControlVoltage_;
    
    void initializeRenderer();
    void applyConfigToComponents();
    Vec2 calculateAutoSize() const;
};

} // namespace YuchenUI
