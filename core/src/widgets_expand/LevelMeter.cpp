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

#include "YuchenUI/widgets_expand/LevelMeter.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/core/UIContext.h"
#include <algorithm>
#include <cmath>
#include <cassert>

namespace YuchenUI {

//==========================================================================================
// MeterDimensions Implementation
float MeterDimensions::getChannelWidth(size_t totalChannelCount)
{
    if (totalChannelCount == 1) return MONO_CHANNEL_WIDTH;
    if (totalChannelCount == 2) return STEREO_CHANNEL_WIDTH;
    return MULTI_CHANNEL_WIDTH;
}
constexpr float MeterDimensions::getTotalHeight()
{
    return PEAK_INDICATOR_SPACING + PEAK_INDICATOR_HEIGHT + PEAK_INDICATOR_SPACING + DEFAULT_HEIGHT;
}
float MeterDimensions::getChannelGroupWidth(size_t channelCount)
{
    if (channelCount == 0) return 0.0f;
    float channelWidth = getChannelWidth(channelCount);
    if (channelCount == 1) return channelWidth;
    return channelWidth + (channelCount - 1) * (channelWidth + CHANNEL_SPACING);
}
float MeterDimensions::getTotalWidth(size_t channelCount) { return getChannelGroupWidth(channelCount) + SCALE_WIDTH; }

//==========================================================================================
// MeterThresholds Implementation
bool MeterThresholds::isValid() const { return normalToWarning < warningToPeak && warningToPeak < peakIndicator; }
MeterThresholds::Region MeterThresholds::getRegion(float db) const
{
    if (db >= warningToPeak) return PEAK;
    if (db >= normalToWarning) return WARNING;
    return NORMAL;
}

//==========================================================================================
// MeterConfig Implementation
void MeterConfig::setThresholds(const MeterThresholds& thresholds) { if (thresholds.isValid()) thresholds_ = thresholds; }
void MeterConfig::setBehavior(const MeterBehavior& behavior) { behavior_ = behavior; }
bool MeterConfig::isValid() const
{
    return thresholds_.isValid() && behavior_.decayRateDbPerSec > 0.0f && behavior_.peakHoldTimeMs >= 0.0f && behavior_.updateEpsilon > 0.0f;
}
MeterConfig MeterConfig::createDefault() { return MeterConfig(); }

//==========================================================================================
// MeterScale Implementation
MeterScale::MeterScale(ScaleType type) : type_(type)
{
    switch (type_)
    {
        case ScaleType::SAMPLE_PEAK: initializeSamplePeak(); break;
        case ScaleType::K12: initializeK12(); break;
        case ScaleType::K14: initializeK14(); break;
        case ScaleType::VU: initializeVU(); break;
        case ScaleType::LINEAR_DB: initializeLinearDb(); break;
    }
}
float MeterScale::mapDbToPosition(float db) const { return dbToPos_ ? dbToPos_(clampDb(db, minDb_, maxDb_)) : 0.0f; }
float MeterScale::mapPositionToDb(float position) const
{
    position = std::clamp(position, 0.0f, 1.0f);
    return posToDb_ ? posToDb_(position) : minDb_;
}
std::string MeterScale::getTypeName() const
{
    switch (type_)
    {
        case ScaleType::SAMPLE_PEAK: return "Sample Peak";
        case ScaleType::K12: return "K-12";
        case ScaleType::K14: return "K-14";
        case ScaleType::VU: return "VU";
        case ScaleType::LINEAR_DB: return "Linear dB";
        default: return "Unknown";
    }
}
MeterScale MeterScale::create(ScaleType type) { return MeterScale(type); }
void MeterScale::initializeSamplePeak()
{
    minDb_ = -144.0f;
    maxDb_ = 0.0f;
    dbToPos_ = [](float db)
    {
        if (db <= -60.0f)
        { return linearMap(db, -144.0f, -60.0f, 0.0f, 0.01118f); }
        else if (db <= -50.0f)
        { return linearMap(db, -60.0f, -50.0f, 0.01118f, 0.06698f); }
        else if (db <= -40.0f)
        { return linearMap(db, -50.0f, -40.0f, 0.06698f, 0.1875f); }
        else
        { return linearMap(db, -40.0f, 0.0f, 0.1875f, 1.0f); }
    };
    posToDb_ = [](float pos)
    {
        if (pos <= 0.01118f)
        { return linearMap(pos, 0.0f, 0.01118f, -144.0f, -60.0f); }
        else if (pos <= 0.06698f)
        { return linearMap(pos, 0.01118f, 0.06698f, -60.0f, -50.0f); }
        else if (pos <= 0.1875f)
        { return linearMap(pos, 0.06698f, 0.1875f, -50.0f, -40.0f); }
        else
        { return linearMap(pos, 0.1875f, 1.0f, -40.0f, 0.0f); }
    };
    ticks_ =
    {
        {0.0f,   dbToPos_(0.0f),    "0", true},
        {-5.0f,  dbToPos_(-5.0f),   "5", true},
        {-10.0f, dbToPos_(-10.0f), "10", true},
        {-15.0f, dbToPos_(-15.0f), "15", true},
        {-20.0f, dbToPos_(-20.0f), "20", true},
        {-25.0f, dbToPos_(-25.0f), "25", true},
        {-30.0f, dbToPos_(-30.0f), "30", true},
        {-35.0f, dbToPos_(-35.0f), "35", true},
        {-40.0f, dbToPos_(-40.0f), "40", true},
        {-50.0f, dbToPos_(-50.0f), "50", true},
        {-60.0f, dbToPos_(-60.0f), "60", true}
    };
}
void MeterScale::initializeK12() {
    minDb_ = -60.0f;
    maxDb_ = 12.0f;
    float minDb = minDb_;
    float maxDb = maxDb_;
    dbToPos_ = [minDb, maxDb](float db)
    {
        if (std::abs(maxDb - minDb) < 1e-6f) return 0.0f;
        db = std::clamp(db, minDb, maxDb);
        return (db - minDb) / (maxDb - minDb);
    };
    posToDb_ = [minDb, maxDb](float pos)
    {
        pos = std::clamp(pos, 0.0f, 1.0f);
        return minDb + pos * (maxDb - minDb);
    };
    ticks_ =
    {
        {12.0f,  dbToPos_(12.0f),  "+12", true},
        {6.0f,   dbToPos_(6.0f),    "+6", true},
        {0.0f,   dbToPos_(0.0f),     "0", true},
        {-6.0f,  dbToPos_(-6.0f),   "6", true},
        {-12.0f, dbToPos_(-12.0f), "12", true},
        {-18.0f, dbToPos_(-18.0f), "18", true},
        {-24.0f, dbToPos_(-24.0f), "24", true},
        {-36.0f, dbToPos_(-36.0f), "36", true},
        {-48.0f, dbToPos_(-48.0f), "48", true},
        {-60.0f, dbToPos_(-60.0f), "60", true}
    };
}
void MeterScale::initializeK14()
{
    minDb_ = -60.0f;
    maxDb_ = 14.0f;
    float minDb = minDb_;
    float maxDb = maxDb_;
    dbToPos_ = [minDb, maxDb](float db)
    {
        if (std::abs(maxDb - minDb) < 1e-6f) return 0.0f;
        db = std::clamp(db, minDb, maxDb);
        return (db - minDb) / (maxDb - minDb);
    };
    posToDb_ = [minDb, maxDb](float pos)
    {
        pos = std::clamp(pos, 0.0f, 1.0f);
        return minDb + pos * (maxDb - minDb);
    };
    ticks_ =
    {
        {14.0f,  dbToPos_(14.0f),  "+14", true},
        {6.0f,   dbToPos_(6.0f),   "+6",  true},
        {-2.0f,  dbToPos_(-2.0f),  "2",  true},
        {-8.0f,  dbToPos_(-8.0f),  "8",  true},
        {-14.0f, dbToPos_(-14.0f), "14", true},
        {-18.0f, dbToPos_(-18.0f), "18", true},
        {-24.0f, dbToPos_(-24.0f), "24", true},
        {-36.0f, dbToPos_(-36.0f), "36", true},
        {-48.0f, dbToPos_(-48.0f), "48", true},
        {-60.0f, dbToPos_(-60.0f), "60", true}
    };
}
void MeterScale::initializeVU()
{
    minDb_ = -20.0f;
    maxDb_ = 3.0f;
    float minDb = minDb_;
    float maxDb = maxDb_;
    dbToPos_ = [minDb, maxDb](float db)
    {
        if (std::abs(maxDb - minDb) < 1e-6f) return 0.0f;
        db = std::clamp(db, minDb, maxDb);
        return (db - minDb) / (maxDb - minDb);
    };
    posToDb_ = [minDb, maxDb](float pos)
    {
        pos = std::clamp(pos, 0.0f, 1.0f);
        return minDb + pos * (maxDb - minDb);
    };
    ticks_ =
    {
        {3.0f,   dbToPos_(3.0f),    "+3", true},
        {2.0f,   dbToPos_(2.0f),    "+2", true},
        {1.0f,   dbToPos_(1.0f),    "+1", true},
        {0.0f,   dbToPos_(0.0f),     "0", true},
        {-1.0f,  dbToPos_(-1.0f),   "1", true},
        {-3.0f,  dbToPos_(-3.0f),   "3", true},
        {-5.0f,  dbToPos_(-5.0f),   "5", true},
        {-7.0f,  dbToPos_(-7.0f),   "7", true},
        {-10.0f, dbToPos_(-10.0f), "10", true},
        {-20.0f, dbToPos_(-20.0f), "20", true}
    };
}
void MeterScale::initializeLinearDb()
{
    minDb_ = -60.0f;
    maxDb_ = 6.0f;
    float minDb = minDb_;
    float maxDb = maxDb_;
    dbToPos_ = [minDb, maxDb](float db)
    {
        if (std::abs(maxDb - minDb) < 1e-6f) return 0.0f;
        db = std::clamp(db, minDb, maxDb);
        return (db - minDb) / (maxDb - minDb);
    };
    posToDb_ = [minDb, maxDb](float pos)
    {
        pos = std::clamp(pos, 0.0f, 1.0f);
        return minDb + pos * (maxDb - minDb);
    };
    ticks_.clear();
    for (int db = static_cast<int>(maxDb_); db >= static_cast<int>(minDb_); db -= 6)
    {
        ticks_.emplace_back(std::abs(static_cast<float>(db)), dbToPos_(static_cast<float>(db)),std::to_string(std::abs(db)), true);
    }
}
float MeterScale::linearMap(float value, float inMin, float inMax, float outMin, float outMax)
{
    if (std::abs(inMax - inMin) < 1e-6f) return outMin;
    float t = (value - inMin) / (inMax - inMin);
    return outMin + t * (outMax - outMin);
}
float MeterScale::clampDb(float db, float minDb, float maxDb) { return std::clamp(db, minDb, maxDb); }

//==========================================================================================
// ChannelLevelData Implementation
ChannelLevelData::ChannelLevelData()
    : currentLevelDb_(MIN_DB), displayLevelDb_(MIN_DB), peakLevelDb_(MIN_DB)
    , peakHoldTimer_(0.0f), peakIndicatorTimer_(0.0f)
    , decayDbPerSec_(40.0f), peakHoldTimeMs_(3000.0f) {}
ChannelLevelData::ChannelLevelData(const ChannelLevelData& other)
    : currentLevelDb_(other.currentLevelDb_), displayLevelDb_(other.displayLevelDb_)
    , peakLevelDb_(other.peakLevelDb_), peakHoldTimer_(other.peakHoldTimer_)
    , peakIndicatorTimer_(other.peakIndicatorTimer_), decayDbPerSec_(other.decayDbPerSec_)
    , peakHoldTimeMs_(other.peakHoldTimeMs_) {}
ChannelLevelData::ChannelLevelData(ChannelLevelData&& other) noexcept
    : currentLevelDb_(other.currentLevelDb_), displayLevelDb_(other.displayLevelDb_)
    , peakLevelDb_(other.peakLevelDb_), peakHoldTimer_(other.peakHoldTimer_)
    , peakIndicatorTimer_(other.peakIndicatorTimer_), decayDbPerSec_(other.decayDbPerSec_)
    , peakHoldTimeMs_(other.peakHoldTimeMs_)
{
    other.currentLevelDb_ = MIN_DB;
    other.displayLevelDb_ = MIN_DB;
    other.peakLevelDb_ = MIN_DB;
    other.peakHoldTimer_ = 0.0f;
    other.peakIndicatorTimer_ = 0.0f;
}
ChannelLevelData& ChannelLevelData::operator=(const ChannelLevelData& other)
{
    if (this != &other)
    {
        currentLevelDb_     = other.currentLevelDb_;
        displayLevelDb_     = other.displayLevelDb_;
        peakLevelDb_        = other.peakLevelDb_;
        peakHoldTimer_      = other.peakHoldTimer_;
        peakIndicatorTimer_ = other.peakIndicatorTimer_;
        decayDbPerSec_      = other.decayDbPerSec_;
        peakHoldTimeMs_     = other.peakHoldTimeMs_;
    }
    return *this;
}
ChannelLevelData& ChannelLevelData::operator=(ChannelLevelData&& other) noexcept
{
    if (this != &other)
    {
        currentLevelDb_       = other.currentLevelDb_;
        displayLevelDb_       = other.displayLevelDb_;
        peakLevelDb_          = other.peakLevelDb_;
        peakHoldTimer_        = other.peakHoldTimer_;
        peakIndicatorTimer_   = other.peakIndicatorTimer_;
        decayDbPerSec_        = other.decayDbPerSec_;
        peakHoldTimeMs_       = other.peakHoldTimeMs_;
        other.currentLevelDb_ = MIN_DB;
        other.displayLevelDb_ = MIN_DB;
        other.peakLevelDb_    = MIN_DB;
        other.peakHoldTimer_  = 0.0f;
        other.peakIndicatorTimer_ = 0.0f;
    }
    return *this;
}

void ChannelLevelData::updateLevel(float levelDb, float deltaTimeMs)
{
    float newLevel = clampDb(levelDb);
    currentLevelDb_ = newLevel;
    if (currentLevelDb_ > peakLevelDb_)
    {
        peakLevelDb_ = currentLevelDb_;
        peakHoldTimer_ = peakHoldTimeMs_;
    }
    if (currentLevelDb_ > displayLevelDb_)
    {
        displayLevelDb_ = currentLevelDb_;
    }
    else
    {
        float decay = decayDbPerSec_ * (deltaTimeMs / 1000.0f);
        displayLevelDb_ = displayLevelDb_ - decay;
        displayLevelDb_ = std::max(currentLevelDb_, displayLevelDb_);
        displayLevelDb_ = std::max(MIN_DB, displayLevelDb_);
    }
    peakHoldTimer_ = std::max(0.0f, peakHoldTimer_ - deltaTimeMs);
    if (peakHoldTimer_ <= 0.0f)
    {
        float decay = decayDbPerSec_ * (deltaTimeMs / 1000.0f);
        peakLevelDb_ = peakLevelDb_ - decay;
        peakLevelDb_ = std::max(displayLevelDb_, peakLevelDb_);
        peakLevelDb_ = std::max(MIN_DB, peakLevelDb_);
    }
    if (peakLevelDb_ >= -0.1f)
    { peakIndicatorTimer_ = PEAK_INDICATOR_HOLD_TIME_MS; }
    else
    { peakIndicatorTimer_ = std::max(0.0f, peakIndicatorTimer_ - deltaTimeMs); }
}
void ChannelLevelData::reset()
{
    currentLevelDb_ = displayLevelDb_ = peakLevelDb_ = MIN_DB;
    peakHoldTimer_ = peakIndicatorTimer_ = 0.0f;
}
float ChannelLevelData::clampDb(float db) const { return std::clamp(db, MIN_DB, MAX_DB); }

//==========================================================================================
// LevelDataManager Implementation
LevelDataManager::LevelDataManager(size_t channelCount)
    : channels_(channelCount)
    , controlVoltageData_(std::make_unique<ChannelLevelData>()){}

float LevelDataManager::getFrameDeltaTime() const
{
    static auto lastTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    float deltaMs = std::chrono::duration<float, std::milli>(currentTime - lastTime).count();
    lastTime = currentTime;
    if (deltaMs < 0.1f) deltaMs = 16.0f;
    if (deltaMs > 100.0f) deltaMs = 100.0f;
    return deltaMs;
}
void LevelDataManager::updateLevels(const std::vector<float>& levels)
{
    float deltaTime = getFrameDeltaTime();
    if (levels.size() != channels_.size()) { setChannelCount(levels.size()); }
    size_t count = std::min(levels.size(), channels_.size());
    for (size_t i = 0; i < count; ++i) { channels_[i].updateLevel(levels[i], deltaTime); }
}
void LevelDataManager::updateLevel(size_t channel, float levelDb)
{
    if (channel >= channels_.size()) return;
    float deltaTime = getFrameDeltaTime();
    channels_[channel].updateLevel(levelDb, deltaTime);
}
void LevelDataManager::reset()
{
    for (auto& channel : channels_) { channel.reset(); }
    clearControlVoltage();
}
const ChannelLevelData& LevelDataManager::getChannel(size_t index) const
{
    static const ChannelLevelData dummy;
    return (index < channels_.size()) ? channels_[index] : dummy;
}
void LevelDataManager::setDecayRate(float dbPerSec)
{
    for (auto& channel : channels_) { channel.setDecayRate(dbPerSec); }
    if (controlVoltageData_) controlVoltageData_->setDecayRate(dbPerSec);
}
void LevelDataManager::setPeakHoldTime(float timeMs)
{
    for (auto& channel : channels_) { channel.setPeakHoldTime(timeMs); }
    if (controlVoltageData_) { controlVoltageData_->setPeakHoldTime(timeMs); }
}
void LevelDataManager::setChannelCount(size_t count)
{
    if (count != channels_.size()) channels_.resize(count);
}
void LevelDataManager::updateControlVoltage(float levelDb)
{
    if (controlVoltageData_)
    {
        float clampedLevel = std::clamp(levelDb, -35.0f, 0.0f);
        float deltaTime = getFrameDeltaTime();
        controlVoltageData_->updateLevel(clampedLevel, deltaTime);
    }
}
float LevelDataManager::getControlVoltageLevel() const
{ return controlVoltageData_ ? controlVoltageData_->getDisplayLevel() : 0.0f; }
void LevelDataManager::clearControlVoltage()
{ if (controlVoltageData_) controlVoltageData_->reset(); }

//==========================================================================================
// BlendedColorCache Implementation
BlendedColorCache::BlendedColorCache()
    : channelWidth_(0.0f)
    , blendTexturePixelCount_(0)
    , initializedForChannelCount_(0)
    , initialized_(false)
{
}

void BlendedColorCache::initialize(size_t totalChannelCount)
{
    if (initialized_ && initializedForChannelCount_ == totalChannelCount) return;
    channelWidth_ = MeterDimensions::getChannelWidth(totalChannelCount);

    // Calculate inner width (excluding 1px borders on each side)
    float innerWidth = channelWidth_ - 2.0f;
    blendTexturePixelCount_ = static_cast<int>(innerWidth * 2.0f);
    blendCache_.clear();
    initializedForChannelCount_ = totalChannelCount;
    initialized_ = true;
    initializeTextures();
}

void BlendedColorCache::initializeTextures()
{
    const float normalEdge = 204.0f;
    const float normalCenter = 255.0f;
    
    const float warningEdgeR = 255.0f;
    const float warningEdgeG = 207.0f;
    const float warningEdgeB = 205.0f;
    const float warningCenterR = 255.0f;
    const float warningCenterG = 255.0f;
    const float warningCenterB = 255.0f;
    
    constexpr float PI = 3.14159265359f;
    
    normalTexture_.pixels.resize(blendTexturePixelCount_);
    warningTexture_.pixels.resize(blendTexturePixelCount_);
    
    float center = (blendTexturePixelCount_ - 1) * 0.5f;
    
    for (int i = 0; i < blendTexturePixelCount_; ++i)
    {
        float distanceFromCenter = std::abs(i - center) / center;
        float cosineWeight = (1.0f + std::cos(distanceFromCenter * PI)) * 0.5f;
        float normalValue = normalEdge + (normalCenter - normalEdge) * cosineWeight;
        normalTexture_.pixels[i] = Vec4::FromRGBA(
            static_cast<uint8_t>(std::round(normalValue)),
            static_cast<uint8_t>(std::round(normalValue)),
            static_cast<uint8_t>(std::round(normalValue)),
            255
        );
        
        // Warning texture
        float warningR = warningEdgeR + (warningCenterR - warningEdgeR) * cosineWeight;
        float warningG = warningEdgeG + (warningCenterG - warningEdgeG) * cosineWeight;
        float warningB = warningEdgeB + (warningCenterB - warningEdgeB) * cosineWeight;
        
        warningTexture_.pixels[i] = Vec4::FromRGBA(
            static_cast<uint8_t>(std::round(warningR)),
            static_cast<uint8_t>(std::round(warningG)),
            static_cast<uint8_t>(std::round(warningB)),
            255
        );
    }
}

Vec4 BlendedColorCache::getBlendedColor(const Vec4& baseColor, float x, bool isWarningRegion) const
{
    uint64_t key = getCacheKey(baseColor, x, isWarningRegion);
    auto it = blendCache_.find(key);
    if (it != blendCache_.end()) return it->second;
    const TextureData& texture = isWarningRegion ? warningTexture_ : normalTexture_;
    Vec4 overlayColor = sampleTexture(texture, x);
    Vec4 result = multiplyBlend(baseColor, overlayColor);
    
    blendCache_[key] = result;
    return result;
}

Vec4 BlendedColorCache::sampleTexture(const TextureData& texture, float logicalX) const
{
    float maxLogicalX = channelWidth_ - 2.0f;
    float normalizedX = logicalX / maxLogicalX;
    normalizedX = std::clamp(normalizedX, 0.0f, 1.0f);
    float physicalX = normalizedX * blendTexturePixelCount_;
    int textureIndex = static_cast<int>(std::floor(physicalX));
    textureIndex = std::clamp(textureIndex, 0, blendTexturePixelCount_ - 1);
    return texture.pixels[textureIndex];
}
Vec4 BlendedColorCache::multiplyBlend(const Vec4& baseColor, const Vec4& overlayColor) const
{
    return Vec4(
        baseColor.x * overlayColor.x,
        baseColor.y * overlayColor.y,
        baseColor.z * overlayColor.z,
        baseColor.w
    );
}
uint64_t BlendedColorCache::getCacheKey(const Vec4& baseColor, float x, bool isWarningRegion) const
{
    uint32_t colorHash = static_cast<uint32_t>(baseColor.x * 255) << 24 |
                         static_cast<uint32_t>(baseColor.y * 255) << 16 |
                         static_cast<uint32_t>(baseColor.z * 255) << 8 |
                         static_cast<uint32_t>(baseColor.w * 255);
    uint32_t xInt = static_cast<uint32_t>(x * 1000);
    return (static_cast<uint64_t>(colorHash) << 32) |
           (static_cast<uint64_t>(xInt) << 1) |
           (isWarningRegion ? 1 : 0);
}

//==========================================================================================
// MeterRenderer Implementation
MeterRenderer::MeterRenderer() : m_context(nullptr), scale_(nullptr), config_(nullptr) {}
void MeterRenderer::renderChannels(RenderList& cmdList, const Vec2& startPos,
                                   const Vec2& totalSize, const LevelDataManager& levelData,
                                   bool showControlVoltage)
{
    if (!isReady()) return;
    size_t totalChannelCount = levelData.getChannelCount();
    updateBlendCache(totalChannelCount);
    auto channelInfos = calculateChannelLayout(startPos, totalSize, levelData);
    for (auto& info : channelInfos)
    {
        info.totalChannelCount = totalChannelCount;
        renderSingleChannel(cmdList, info);
    }
    if (showControlVoltage) renderControlVoltage(cmdList, startPos, totalSize, levelData);
}
void MeterRenderer::renderScale(RenderList& cmdList, const Rect& scaleRect, const Rect& referenceRect)
{
    if (!isReady() || scaleRect.width <= 0 || scaleRect.height <= 0) return;
    if (referenceRect.width <= 0 || referenceRect.height <= 0) return;
    const auto& ticks = scale_->getTickMarks();
    drawScaleTicks(cmdList, scaleRect, referenceRect, ticks);
}
void MeterRenderer::renderSingleChannel(RenderList& cmdList, const ChannelRenderInfo& info)
{
    if (!isReady()) return;
    size_t totalChannelCount = info.totalChannelCount;
    renderChannelBackground(cmdList, info.meterRect, info.displayLevelDb, totalChannelCount);
    renderChannelFill(cmdList, info.meterRect, info.displayLevel01, info.displayLevelDb, totalChannelCount);
    renderChannelPeak(cmdList, info.meterRect, info.peakLevel01, info.displayLevelDb, totalChannelCount);
    renderInternalScale(cmdList, info.meterRect, info.displayLevelDb, totalChannelCount);
    renderChannelFrame(cmdList, info.meterRect);
    renderPeakIndicator(cmdList, info.peakIndicatorRect, info.isPeakIndicatorActive);
    renderPeakIndicatorFrame(cmdList, info.peakIndicatorRect);
}
std::vector<ChannelRenderInfo> MeterRenderer::calculateChannelLayout(const Vec2& startPos, const Vec2& totalSize,
                                                                     const LevelDataManager& levelData) const
{
    std::vector<ChannelRenderInfo> infos;
    size_t totalChannelCount = levelData.getChannelCount();
    if (totalChannelCount == 0) return infos;
    float peakAndSpacing = MeterDimensions::PEAK_INDICATOR_HEIGHT + MeterDimensions::PEAK_INDICATOR_SPACING;
    Vec2 meterStartPos(startPos.x + MeterDimensions::SCALE_WIDTH, startPos.y + peakAndSpacing);
    Vec2 meterSize(totalSize.x - MeterDimensions::SCALE_WIDTH, MeterDimensions::DEFAULT_HEIGHT);
    infos.reserve(totalChannelCount);
    for (size_t i = 0; i < totalChannelCount; ++i)
    {
        ChannelRenderInfo info;
        info.meterRect = calculateChannelRect(meterStartPos, meterSize, i, totalChannelCount);
        info.peakIndicatorRect = Rect(info.meterRect.x, startPos.y,info.meterRect.width,MeterDimensions::PEAK_INDICATOR_HEIGHT);
        const auto& channelData = levelData.getChannel(i);
        info.displayLevel01 = dbToPosition01(channelData.getDisplayLevel());
        info.peakLevel01 = dbToPosition01(channelData.getPeakLevel());
        info.displayLevelDb = channelData.getDisplayLevel();
        info.isPeakIndicatorActive = channelData.isPeakIndicatorActive();
        info.channelIndex = i;
        info.totalChannelCount = totalChannelCount;
        infos.push_back(info);
    }
    return infos;
}

void MeterRenderer::renderChannelBackground(RenderList& cmdList, const Rect& rect,
                                           float displayLevelDb, size_t totalChannelCount)
{
    if (rect.width <= 0 || rect.height <= 0) return;
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return;
    LevelMeterColors colors = style->getLevelMeterColors();
    
    Rect innerRect(rect.x + 1.0f, rect.y + 1.0f, rect.width - 2.0f, rect.height - 2.0f);
    if (innerRect.width <= 0 || innerRect.height <= 0) return;
    
    float peakThreshold01 = getPeakThreshold01();
    float warningThreshold01 = getWarningThreshold01();
    float normalHeight = warningThreshold01 * innerRect.height;
    float warningHeight = (peakThreshold01 - warningThreshold01) * innerRect.height;
    float peakHeight = (1.0f - peakThreshold01) * innerRect.height;
    constexpr float PIXEL_STEP = 1.0f;
    if (normalHeight > 0.0f)
    {
        Rect normalRect(innerRect.x, innerRect.y + innerRect.height - normalHeight,innerRect.width, normalHeight);
        for (float x = normalRect.x; x < normalRect.x + normalRect.width; x += PIXEL_STEP)
        {
            float pixelWidth = std::min(PIXEL_STEP, normalRect.x + normalRect.width - x);
            Rect pixelRect(x, normalRect.y, pixelWidth, normalRect.height);
            Vec4 blendedColor = blendCache_.getBlendedColor(colors.bgNormal, x - innerRect.x, false);
            cmdList.fillRect(pixelRect, blendedColor);
        }
    }
    if (warningHeight > 0.0f)
    {
        Rect warningRect(innerRect.x, innerRect.y + innerRect.height - normalHeight - warningHeight,innerRect.width, warningHeight);
        for (float x = warningRect.x; x < warningRect.x + warningRect.width; x += PIXEL_STEP)
        {
            float pixelWidth = std::min(PIXEL_STEP, warningRect.x + warningRect.width - x);
            Rect pixelRect(x, warningRect.y, pixelWidth, warningRect.height);
            Vec4 blendedColor = blendCache_.getBlendedColor(colors.bgWarning, x - innerRect.x, false);
            cmdList.fillRect(pixelRect, blendedColor);
        }
    }
    if (peakHeight > 0.0f)
    {
        Rect peakRect(innerRect.x, innerRect.y, innerRect.width, peakHeight);
        for (float x = peakRect.x; x < peakRect.x + peakRect.width; x += PIXEL_STEP)
        {
            float pixelWidth = std::min(PIXEL_STEP, peakRect.x + peakRect.width - x);
            Rect pixelRect(x, peakRect.y, pixelWidth, peakRect.height);
            Vec4 blendedColor = blendCache_.getBlendedColor(colors.bgPeak, x - innerRect.x, true);
            cmdList.fillRect(pixelRect, blendedColor);
        }
    }
}
void MeterRenderer::renderChannelFill(RenderList& cmdList, const Rect& rect, float level01,
                                     float displayLevelDb, size_t totalChannelCount)
{
    if (rect.width <= 0 || rect.height <= 0 || level01 <= 0.0f) return;
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return;
    LevelMeterColors colors = style->getLevelMeterColors();
    Rect innerRect(rect.x + 1.0f, rect.y + 1.0f, rect.width - 2.0f, rect.height - 2.0f);
    if (innerRect.width <= 0 || innerRect.height <= 0) return;
    level01 = std::clamp(level01, 0.0f, 1.0f);
    float warningThreshold01 = getWarningThreshold01();
    float peakThreshold01 = getPeakThreshold01();
    float currentBottom = innerRect.y + innerRect.height;
    constexpr float PIXEL_STEP = 1.0f;
    if (level01 > 0.0f && warningThreshold01 > 0.0f)
    {
        float normalFill = std::min(level01, warningThreshold01);
        if (normalFill > 0.0f)
        {
            float normalHeight = normalFill * innerRect.height;
            Rect normalFillRect(innerRect.x, currentBottom - normalHeight, innerRect.width, normalHeight);
            
            for (float x = normalFillRect.x; x < normalFillRect.x + normalFillRect.width; x += PIXEL_STEP)
            {
                float pixelWidth = std::min(PIXEL_STEP, normalFillRect.x + normalFillRect.width - x);
                Rect pixelRect(x, normalFillRect.y, pixelWidth, normalFillRect.height);
                Vec4 blendedColor = blendCache_.getBlendedColor(colors.levelNormal, x - innerRect.x, false);
                cmdList.fillRect(pixelRect, blendedColor);
            }
        }
    }
    if (level01 > warningThreshold01 && peakThreshold01 > warningThreshold01)
    {
        float warningFill = std::min(level01, peakThreshold01) - warningThreshold01;
        if (warningFill > 0.0f)
        {
            float warningHeight = warningFill * innerRect.height;
            Rect warningFillRect(innerRect.x,
                                currentBottom - (warningThreshold01 * innerRect.height) - warningHeight,
                                innerRect.width, warningHeight);
            
            for (float x = warningFillRect.x; x < warningFillRect.x + warningFillRect.width; x += PIXEL_STEP)
            {
                float pixelWidth = std::min(PIXEL_STEP, warningFillRect.x + warningFillRect.width - x);
                Rect pixelRect(x, warningFillRect.y, pixelWidth, warningFillRect.height);
                Vec4 blendedColor = blendCache_.getBlendedColor(colors.levelWarning, x - innerRect.x, false);
                cmdList.fillRect(pixelRect, blendedColor);
            }
        }
    }
    if (level01 > peakThreshold01)
    {
        float peakFill = level01 - peakThreshold01;
        if (peakFill > 0.0f)
        {
            float peakHeight = peakFill * innerRect.height;
            Rect peakFillRect(innerRect.x,
                             currentBottom - (peakThreshold01 * innerRect.height) - peakHeight,
                             innerRect.width, peakHeight);
            
            for (float x = peakFillRect.x; x < peakFillRect.x + peakFillRect.width; x += PIXEL_STEP)
            {
                float pixelWidth = std::min(PIXEL_STEP, peakFillRect.x + peakFillRect.width - x);
                Rect pixelRect(x, peakFillRect.y, pixelWidth, peakFillRect.height);
                Vec4 blendedColor = blendCache_.getBlendedColor(colors.levelPeak, x - innerRect.x, true);
                cmdList.fillRect(pixelRect, blendedColor);
            }
        }
    }
}
void MeterRenderer::renderChannelFrame(RenderList& cmdList, const Rect& rect)
{
    if (rect.width <= 0 || rect.height <= 0) return;
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return;
    LevelMeterColors colors = style->getLevelMeterColors();
    cmdList.drawRect(rect, colors.border, 1.0f);
}
void MeterRenderer::renderChannelPeak(RenderList& cmdList, const Rect& rect, float peakLevel01,
                                     float displayLevelDb, size_t totalChannelCount)
{
    if (rect.width <= 0 || rect.height <= 0 || peakLevel01 <= 0.0f) return;
    float peakDb = scale_->mapPositionToDb(peakLevel01);
    Vec4 peakColor = getLevelColor(peakDb);
    drawPeakLine(cmdList, rect, peakLevel01, peakColor, MeterDimensions::PEAK_LINE_HEIGHT);
}
void MeterRenderer::renderInternalScale(RenderList& cmdList, const Rect& rect,
                                       float displayLevelDb, size_t totalChannelCount)
{
    if (!isReady() || rect.width <= 0 || rect.height <= 0) return;
    const auto& ticks = scale_->getTickMarks();
    drawInternalScaleTicks(cmdList, rect, ticks, displayLevelDb, totalChannelCount);
}
void MeterRenderer::renderPeakIndicator(RenderList& cmdList, const Rect& rect, bool isActive)
{
    if (rect.width <= 0 || rect.height <= 0) return;
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return;
    LevelMeterColors colors = style->getLevelMeterColors();
    Vec4 fillColor = isActive ? colors.peakIndicatorActive : colors.peakIndicatorInactive;
    cmdList.fillRect(rect, fillColor);
}
void MeterRenderer::renderPeakIndicatorFrame(RenderList& cmdList, const Rect& rect)
{
    if (rect.width <= 0 || rect.height <= 0) return;
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return;
    LevelMeterColors colors = style->getLevelMeterColors();
    cmdList.drawRect(rect, colors.border, 1.0f);
}
void MeterRenderer::renderControlVoltage(RenderList& cmdList, const Vec2& startPos,
                                        const Vec2& totalSize, const LevelDataManager& levelData)
{
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return;
    LevelMeterColors colors = style->getLevelMeterColors();
    size_t totalChannelCount = levelData.getChannelCount();
    float channelWidth = MeterDimensions::getChannelWidth(totalChannelCount);
    float channelSpacing = MeterDimensions::CHANNEL_SPACING;
    float peakAndSpacing = MeterDimensions::PEAK_INDICATOR_HEIGHT + MeterDimensions::PEAK_INDICATOR_SPACING;
    float channelAreaStartX = startPos.x + MeterDimensions::SCALE_WIDTH;
    float lastChannelX = channelAreaStartX + (totalChannelCount - 1) * (channelWidth + channelSpacing);
    float controlVoltageX = lastChannelX + channelWidth + 1.0f;
    Vec2 cvPos(controlVoltageX, startPos.y + peakAndSpacing);
    const float CV_TOP_DB = 0.0f;
    const float CV_BOTTOM_DB = -35.0f;
    float scaleMinDb = scale_->getMinDb();
    float scaleMaxDb = scale_->getMaxDb();
    float visibleTopDb = std::min(CV_TOP_DB, scaleMaxDb);
    float visibleBottomDb = std::max(CV_BOTTOM_DB, scaleMinDb);
    if (visibleBottomDb >= visibleTopDb) return;
    float visibleTopPos = scale_->mapDbToPosition(visibleTopDb);
    float visibleBottomPos = scale_->mapDbToPosition(visibleBottomDb);
    float meterHeight = MeterDimensions::DEFAULT_HEIGHT;
    float topY = cvPos.y + (1.0f - visibleTopPos) * meterHeight;
    float bottomY = cvPos.y + (1.0f - visibleBottomPos) * meterHeight;
    float displayHeight = bottomY - topY;
    const float MIN_HEIGHT = 2.5f;
    if (displayHeight < MIN_HEIGHT)
    {
        float center = (topY + bottomY) * 0.5f;
        topY = center - MIN_HEIGHT * 0.5f;
        bottomY = center + MIN_HEIGHT * 0.5f;
        displayHeight = MIN_HEIGHT;
    }
    Rect actualRect(cvPos.x, topY, 5.0f, displayHeight);
    Rect innerRect(actualRect.x + 1.0f, actualRect.y + 1.0f, actualRect.width - 2.0f, actualRect.height - 2.0f);
    if (innerRect.width > 0 && innerRect.height > 0) {
        cmdList.fillRect(innerRect, colors.bgNormal);
        float currentLevel = levelData.getControlVoltageLevel();
        if (currentLevel >= visibleBottomDb && currentLevel <= visibleTopDb) {
            float fillRatio = (visibleTopDb - currentLevel) / (visibleTopDb - visibleBottomDb);
            fillRatio = std::clamp(fillRatio, 0.0f, 1.0f);
            if (fillRatio > 0.0f)
            {
                float fillHeight = fillRatio * innerRect.height;
                Rect fillRect(innerRect.x, innerRect.y, innerRect.width, fillHeight);
                if (fillRect.width > 0 && fillRect.height > 0) cmdList.fillRect(fillRect, colors.levelPeak);
            }
        }
        else if (currentLevel < visibleBottomDb)
        {
            cmdList.fillRect(innerRect, colors.levelPeak);
        }
    }
    cmdList.drawRect(actualRect, colors.border, 1.0f);
}
Vec4 MeterRenderer::getLevelColor(float db) const
{
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return Vec4::FromRGBA(255, 0, 255, 255);
    LevelMeterColors colors = style->getLevelMeterColors();
    const auto& thresholds = config_->getThresholds();
    switch (thresholds.getRegion(db))
    {
        case MeterThresholds::PEAK: return colors.levelPeak;
        case MeterThresholds::WARNING: return colors.levelWarning;
        default: return colors.levelNormal;
    }
}
float MeterRenderer::dbToPosition01(float db) const
{
    return scale_->mapDbToPosition(db);
}
float MeterRenderer::getWarningThreshold01() const
{
    return dbToPosition01(config_->getThresholds().normalToWarning);
}
float MeterRenderer::getPeakThreshold01() const
{
    return dbToPosition01(config_->getThresholds().warningToPeak);
}
void MeterRenderer::updateBlendCache(size_t totalChannelCount)
{
    blendCache_.initialize(totalChannelCount);
}
bool MeterRenderer::isReady() const
{
    return scale_ && config_;
}
void MeterRenderer::drawPeakLine(RenderList& cmdList, const Rect& rect, float peakLevel01, const Vec4& color, float lineHeight) {
    if (rect.width <= 0 || rect.height <= 0 || peakLevel01 <= 0.0f) return;
    float peakY = rect.y + rect.height - (peakLevel01 * rect.height);
    float adjustedPeakY = peakY + 0.5f;
    float halfHeight = lineHeight * 0.5f;
    Rect peakRect(rect.x + 1.0f, adjustedPeakY - halfHeight, rect.width - 2.0f, lineHeight);
    cmdList.fillRect(peakRect, color);
}
void MeterRenderer::drawInternalScaleTicks(RenderList& cmdList, const Rect& rect,
                                          const std::vector<ScaleTick>& ticks,
                                          float currentDisplayLevel, size_t totalChannelCount)
{
    if (rect.width <= 0 || rect.height <= 0) return;
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    if (!style) return;
    LevelMeterColors colors = style->getLevelMeterColors();
    
    constexpr float topBuffer = 2.0f;
    constexpr float bottomBuffer = 2.0f;
    constexpr float lineThickness = 0.5f;
    constexpr float leftOffset = 1.0f;
    constexpr float VERTICAL_OFFSET = 0.5f;
    float channelWidth = MeterDimensions::getChannelWidth(totalChannelCount);
    float tickLength = channelWidth - 2.0f;
    tickLength = std::max(tickLength, 2.0f);
    const auto& thresholds = config_->getThresholds();
    for (const auto& tick : ticks) {
        if (tick.db < scale_->getMinDb() || tick.db > scale_->getMaxDb()) continue;
        float tickY = rect.y + rect.height - (tick.position * rect.height);
        tickY += VERTICAL_OFFSET;
        if (tickY < rect.y + topBuffer || tickY > rect.y + rect.height - bottomBuffer) continue;
        bool isActive = currentDisplayLevel >= tick.db;
        Vec4 scaleColor;
        switch (thresholds.getRegion(tick.db))
        {
            case MeterThresholds::PEAK:     scaleColor = isActive ? colors.internalScalePeakActive    : colors.internalScalePeakInactive;    break;
            case MeterThresholds::WARNING:  scaleColor = isActive ? colors.internalScaleWarningActive : colors.internalScaleWarningInactive; break;
            default:                        scaleColor = isActive ? colors.internalScaleNormalActive  : colors.internalScaleNormalInactive;  break;
        }
        float startX = rect.x + leftOffset;
        float endX = rect.x + leftOffset + tickLength;
        Vec2 lineStart(startX, tickY);
        Vec2 lineEnd(endX, tickY);
        cmdList.drawLine(lineStart, lineEnd, scaleColor, lineThickness);
    }
}
void MeterRenderer::drawScaleTicks(RenderList& cmdList, const Rect& scaleRect,
                                  const Rect& meterRect, const std::vector<ScaleTick>& ticks)
{
    if (scaleRect.width <= 0 || scaleRect.height <= 0 || meterRect.width <= 0 || meterRect.height <= 0) return;
    
    UIStyle* style = m_context ? m_context->getCurrentStyle() : nullptr;
    IFontProvider* fontProvider = m_context ? m_context->getFontProvider() : nullptr;
    if (!style || !fontProvider) return;
    
    LevelMeterColors colors = style->getLevelMeterColors();
    FontFallbackChain fallbackChain(fontProvider->getDefaultNarrowBoldFont());
    constexpr float tickLength = 3.0f, tickThickness = 0.5f, VERTICAL_OFFSET = 0.5f, fontSize = 9.0f, letterSpacing = -50.0f;
    
    for (const auto& tick : ticks)
    {
        float tickY = meterRect.y + meterRect.height - (tick.position * meterRect.height) + VERTICAL_OFFSET;
        if (tickY < meterRect.y || tickY > meterRect.y + meterRect.height) continue;

        float tickStart = meterRect.x - tickLength;
        if (tickStart >= scaleRect.x) cmdList.drawLine({tickStart, tickY}, {meterRect.x, tickY}, colors.scaleColor, tickThickness);
        
        if (!tick.label.empty())
        {
            FontHandle primaryFont = fallbackChain.getPrimary();
            FontMetrics metrics = fontProvider->getFontMetrics(primaryFont, fontSize);
            Vec2 textSize = fontProvider->measureText(tick.label.c_str(), fontSize);
            size_t charCount = std::count_if(tick.label.begin(), tick.label.end(), [](char c) { return (c & 0xC0) != 0x80; });
            float adjustedWidth = textSize.x + ((charCount > 1) ? (letterSpacing / 1000.0f) * fontSize * (charCount - 1) : 0);

            float rightEdge = tickStart - 2.0f;
            float textX = rightEdge - adjustedWidth, textY = tickY - textSize.y * 0.5f + metrics.ascender + VERTICAL_OFFSET;
            cmdList.drawText(tick.label.c_str(), {std::round(textX), std::round(textY)}, fallbackChain, fontSize, colors.scaleColor, letterSpacing);
        }
    }
}
Rect MeterRenderer::calculateChannelRect(const Vec2& startPos, const Vec2& totalSize,
                                        size_t channelIndex, size_t totalChannels) const
{
    assert(totalChannels > 0 && channelIndex < totalChannels);
    float channelWidth = MeterDimensions::getChannelWidth(totalChannels);
    constexpr float channelSpacing = MeterDimensions::CHANNEL_SPACING;
    float alignedWidth = pixelAlign(channelWidth);
    float meterStartX = pixelAlign(startPos.x);
    float channelX = meterStartX + static_cast<float>(channelIndex) * (channelWidth + channelSpacing);
    channelX = pixelAlign(channelX);
    return Rect(channelX, startPos.y, alignedWidth, totalSize.y);
}
float MeterRenderer::pixelAlign(float value) const
{
    return std::round(value);
}

//==========================================================================================
// LevelMeter Implementation
LevelMeter::LevelMeter(UIContext* context, const Rect& bounds, size_t channelCount, ScaleType scaleType)
    : Widget(bounds)
    , m_context(context)
    , levelData_(channelCount)
    , config_(MeterConfig::createDefault())
    , scale_(scaleType)
    , showControlVoltage_(true)
{
    setOwnerContext(context);
    initializeRenderer();
    applyConfigToComponents();
}
LevelMeter::LevelMeter(UIContext* context, const Rect& bounds, const MeterConfig& config,
                      size_t channelCount, ScaleType scaleType)
    : Widget(bounds)
    , m_context(context)
    , levelData_(channelCount)
    , config_(config)
    , scale_(scaleType)
    , showControlVoltage_(true)
{
    setOwnerContext(context);
    initializeRenderer();
    applyConfigToComponents();
}
LevelMeter::~LevelMeter() {}
void LevelMeter::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    if (!m_isVisible) return;
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Vec2 renderSize(m_bounds.width, m_bounds.height);
    if (renderSize.x <= 0.0f || renderSize.y <= 0.0f)
    {
        Vec2 autoSize = calculateAutoSize();
        if (renderSize.x <= 0.0f) renderSize.x = autoSize.x;
        if (renderSize.y <= 0.0f) renderSize.y = autoSize.y;
    }
    MeterRenderer& mutableRenderer = const_cast<MeterRenderer&>(renderer_);
    mutableRenderer.renderChannels(commandList, absPos, renderSize, levelData_, showControlVoltage_);
    float scaleWidth = MeterDimensions::SCALE_WIDTH;
    Rect scaleRect(absPos.x, absPos.y, scaleWidth, renderSize.y);
    
    auto channelInfos = mutableRenderer.calculateChannelLayout(absPos, renderSize, levelData_);
    if (!channelInfos.empty()) mutableRenderer.renderScale(commandList, scaleRect, channelInfos[0].meterRect);
    renderChildren(commandList, absPos);
}

void LevelMeter::updateLevels(const std::vector<float>& levels)
{ levelData_.updateLevels(levels); }

void LevelMeter::updateLevel(size_t channel, float levelDb)
{ levelData_.updateLevel(channel, levelDb); }

void LevelMeter::reset()
{ levelData_.reset(); }

void LevelMeter::setChannelCount(size_t count)
{ levelData_.setChannelCount(count); }

void LevelMeter::setScaleType(ScaleType type)
{
    scale_ = MeterScale::create(type);
    renderer_.setScale(&scale_);
}
void LevelMeter::setConfig(const MeterConfig& config)
{
    if (config.isValid()) {
        config_ = config;
        applyConfigToComponents();
    }
}
void LevelMeter::setThresholds(float warningDb, float peakDb)
{
    MeterThresholds thresholds = config_.getThresholds();
    thresholds.normalToWarning = warningDb;
    thresholds.warningToPeak = peakDb;
    if (thresholds.isValid()) {
        config_.setThresholds(thresholds);
    }
}
void LevelMeter::setDecayRate(float dbPerSec)
{
    config_.setDecayRate(dbPerSec);
    levelData_.setDecayRate(dbPerSec);
}
void LevelMeter::setPeakHoldTime(float timeMs)
{
    config_.setPeakHoldTime(timeMs);
    levelData_.setPeakHoldTime(timeMs);
}
size_t LevelMeter::getChannelCount() const { return levelData_.getChannelCount(); }
Vec2 LevelMeter::getRecommendedSize() const { return calculateAutoSize(); }

void LevelMeter::initializeRenderer()
{
    renderer_.setContext(m_context);
    renderer_.setScale(&scale_);
    renderer_.setConfig(&config_);
}
void LevelMeter::applyConfigToComponents()
{
    const auto& behavior = config_.getBehavior();
    levelData_.setDecayRate(behavior.decayRateDbPerSec);
    levelData_.setPeakHoldTime(behavior.peakHoldTimeMs);
    renderer_.setConfig(&config_);
}
Vec2 LevelMeter::calculateAutoSize() const
{
    size_t channelCount = levelData_.getChannelCount();
    float width = MeterDimensions::getTotalWidth(channelCount);
    if (showControlVoltage_) width += 6.0f;
    float height = MeterDimensions::getTotalHeight();
    return Vec2(width, height);
}
void LevelMeter::updateControlVoltage(float levelDb) { levelData_.updateControlVoltage(levelDb); }
void LevelMeter::setShowControlVoltage(bool show) { showControlVoltage_ = show; }
bool LevelMeter::getShowControlVoltage() const { return showControlVoltage_;}

} // namespace YuchenUI
