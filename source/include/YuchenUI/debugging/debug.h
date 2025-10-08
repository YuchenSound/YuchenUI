#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <iostream>
#include <iomanip>

namespace YuchenUI {
namespace Debug {

class PerformanceMonitor {
public:
    static PerformanceMonitor& getInstance() {
        static PerformanceMonitor instance;
        return instance;
    }
    
    // 开始新的一帧
    void beginFrame() {
        m_currentFrameStats = FrameStats();
        m_frameStartTime = std::chrono::high_resolution_clock::now();
    }
    
    // 结束当前帧
    void endFrame() {
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::microseconds>(
            frameEndTime - m_frameStartTime).count();
        
        m_currentFrameStats.frameTimeUs = frameDuration;
        m_accumulatedStats.add(m_currentFrameStats);
        m_frameCount++;
        
        // 每5秒输出一次统计
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            now - m_lastReportTime).count();
        
        if (elapsed >= 5) {
            printReport();
            resetAccumulated();
            m_lastReportTime = now;
        }
    }
    
    // 记录各种事件
    void recordDrawCall() { m_currentFrameStats.drawCalls++; }
    void recordBufferCreation() { m_currentFrameStats.bufferCreations++; }
    void recordTextureSwitch() { m_currentFrameStats.textureSwitches++; }
    void recordPipelineSwitch() { m_currentFrameStats.pipelineSwitches++; }
    void recordVertexCount(size_t count) { m_currentFrameStats.vertexCount += count; }
    void recordImageDraw() { m_currentFrameStats.imageDraws++; }
    void recordNineSliceDraw() { m_currentFrameStats.nineSliceDraws++; }
    void recordTextDraw() { m_currentFrameStats.textDraws++; }
    
    // 记录具体纹理使用
    void recordTextureUsage(const std::string& textureName) {
        m_currentFrameStats.textureUsageCount[textureName]++;
    }

private:
    struct FrameStats {
        size_t drawCalls = 0;
        size_t bufferCreations = 0;
        size_t textureSwitches = 0;
        size_t pipelineSwitches = 0;
        size_t vertexCount = 0;
        size_t imageDraws = 0;
        size_t nineSliceDraws = 0;
        size_t textDraws = 0;
        int64_t frameTimeUs = 0;
        std::unordered_map<std::string, size_t> textureUsageCount;
    };
    
    struct AccumulatedStats {
        size_t totalFrames = 0;
        size_t totalDrawCalls = 0;
        size_t totalBufferCreations = 0;
        size_t totalTextureSwitches = 0;
        size_t totalPipelineSwitches = 0;
        size_t totalVertexCount = 0;
        size_t totalImageDraws = 0;
        size_t totalNineSliceDraws = 0;
        size_t totalTextDraws = 0;
        int64_t totalFrameTimeUs = 0;
        int64_t minFrameTimeUs = INT64_MAX;
        int64_t maxFrameTimeUs = 0;
        std::unordered_map<std::string, size_t> totalTextureUsage;
        
        void add(const FrameStats& frame) {
            totalFrames++;
            totalDrawCalls += frame.drawCalls;
            totalBufferCreations += frame.bufferCreations;
            totalTextureSwitches += frame.textureSwitches;
            totalPipelineSwitches += frame.pipelineSwitches;
            totalVertexCount += frame.vertexCount;
            totalImageDraws += frame.imageDraws;
            totalNineSliceDraws += frame.nineSliceDraws;
            totalTextDraws += frame.textDraws;
            totalFrameTimeUs += frame.frameTimeUs;
            minFrameTimeUs = std::min(minFrameTimeUs, frame.frameTimeUs);
            maxFrameTimeUs = std::max(maxFrameTimeUs, frame.frameTimeUs);
            
            for (const auto& pair : frame.textureUsageCount) {
                totalTextureUsage[pair.first] += pair.second;
            }
        }
    };
    
    PerformanceMonitor()
        : m_frameCount(0)
        , m_lastReportTime(std::chrono::high_resolution_clock::now())
        , m_frameStartTime(std::chrono::high_resolution_clock::now())
    {}
    
    void printReport() {
        if (m_accumulatedStats.totalFrames == 0) return;
        
        double avgFrameTime = static_cast<double>(m_accumulatedStats.totalFrameTimeUs) / m_accumulatedStats.totalFrames;
        double avgDrawCalls = static_cast<double>(m_accumulatedStats.totalDrawCalls) / m_accumulatedStats.totalFrames;
        double avgBufferCreations = static_cast<double>(m_accumulatedStats.totalBufferCreations) / m_accumulatedStats.totalFrames;
        double avgTextureSwitches = static_cast<double>(m_accumulatedStats.totalTextureSwitches) / m_accumulatedStats.totalFrames;
        double avgVertices = static_cast<double>(m_accumulatedStats.totalVertexCount) / m_accumulatedStats.totalFrames;
        double fps = 1000000.0 / avgFrameTime;
        
        std::cout << "\n╔══════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              YuchenUI Performance Report (5s average)               ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Frame Statistics:                                                    ║\n";
        std::cout << "║   Total Frames:        " << std::setw(10) << m_accumulatedStats.totalFrames << "                                      ║\n";
        std::cout << "║   Avg Frame Time:      " << std::setw(10) << std::fixed << std::setprecision(2) << avgFrameTime/1000.0 << " ms                                ║\n";
        std::cout << "║   Min Frame Time:      " << std::setw(10) << std::fixed << std::setprecision(2) << m_accumulatedStats.minFrameTimeUs/1000.0 << " ms                                ║\n";
        std::cout << "║   Max Frame Time:      " << std::setw(10) << std::fixed << std::setprecision(2) << m_accumulatedStats.maxFrameTimeUs/1000.0 << " ms                                ║\n";
        std::cout << "║   Avg FPS:             " << std::setw(10) << std::fixed << std::setprecision(1) << fps << "                                    ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Rendering Statistics (per frame average):                           ║\n";
        std::cout << "║   Draw Calls:          " << std::setw(10) << std::fixed << std::setprecision(1) << avgDrawCalls << "                                    ║\n";
        std::cout << "║   Buffer Creations:    " << std::setw(10) << std::fixed << std::setprecision(1) << avgBufferCreations << "                                    ║\n";
        std::cout << "║   Texture Switches:    " << std::setw(10) << std::fixed << std::setprecision(1) << avgTextureSwitches << "                                    ║\n";
        std::cout << "║   Pipeline Switches:   " << std::setw(10) << m_accumulatedStats.totalPipelineSwitches / m_accumulatedStats.totalFrames << "                                      ║\n";
        std::cout << "║   Vertices:            " << std::setw(10) << std::fixed << std::setprecision(0) << avgVertices << "                                    ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ Draw Type Breakdown:                                                 ║\n";
        std::cout << "║   Image Draws:         " << std::setw(10) << m_accumulatedStats.totalImageDraws / m_accumulatedStats.totalFrames << "                                      ║\n";
        std::cout << "║   Nine-Slice Draws:    " << std::setw(10) << m_accumulatedStats.totalNineSliceDraws / m_accumulatedStats.totalFrames << "                                      ║\n";
        std::cout << "║   Text Draws:          " << std::setw(10) << m_accumulatedStats.totalTextDraws / m_accumulatedStats.totalFrames << "                                      ║\n";
        
        if (!m_accumulatedStats.totalTextureUsage.empty()) {
            std::cout << "╠══════════════════════════════════════════════════════════════════════╣\n";
            std::cout << "║ Top Textures (per frame):                                            ║\n";
            
            // 按使用频率排序
            std::vector<std::pair<std::string, size_t>> sortedTextures(
                m_accumulatedStats.totalTextureUsage.begin(),
                m_accumulatedStats.totalTextureUsage.end()
            );
            std::sort(sortedTextures.begin(), sortedTextures.end(),
                [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (size_t i = 0; i < std::min(size_t(5), sortedTextures.size()); i++) {
                double avgUsage = static_cast<double>(sortedTextures[i].second) / m_accumulatedStats.totalFrames;
                std::string name = sortedTextures[i].first;
                if (name.length() > 35) {
                    name = "..." + name.substr(name.length() - 32);
                }
                std::cout << "║   " << std::left << std::setw(35) << name
                         << std::right << std::setw(10) << std::fixed << std::setprecision(1) << avgUsage
                         << " times           ║\n";
            }
        }
        
        std::cout << "╚══════════════════════════════════════════════════════════════════════╝\n\n";
    }
    
    void resetAccumulated() {
        m_accumulatedStats = AccumulatedStats();
    }
    
    FrameStats m_currentFrameStats;
    AccumulatedStats m_accumulatedStats;
    size_t m_frameCount;
    std::chrono::high_resolution_clock::time_point m_lastReportTime;
    std::chrono::high_resolution_clock::time_point m_frameStartTime;
};

// 便捷宏
#ifdef YUCHEN_DEBUG
    #define YUCHEN_PERF_BEGIN_FRAME() YuchenUI::Debug::PerformanceMonitor::getInstance().beginFrame()
    #define YUCHEN_PERF_END_FRAME() YuchenUI::Debug::PerformanceMonitor::getInstance().endFrame()
    #define YUCHEN_PERF_DRAW_CALL() YuchenUI::Debug::PerformanceMonitor::getInstance().recordDrawCall()
    #define YUCHEN_PERF_BUFFER_CREATE() YuchenUI::Debug::PerformanceMonitor::getInstance().recordBufferCreation()
    #define YUCHEN_PERF_TEXTURE_SWITCH() YuchenUI::Debug::PerformanceMonitor::getInstance().recordTextureSwitch()
    #define YUCHEN_PERF_PIPELINE_SWITCH() YuchenUI::Debug::PerformanceMonitor::getInstance().recordPipelineSwitch()
    #define YUCHEN_PERF_VERTICES(count) YuchenUI::Debug::PerformanceMonitor::getInstance().recordVertexCount(count)
    #define YUCHEN_PERF_IMAGE_DRAW() YuchenUI::Debug::PerformanceMonitor::getInstance().recordImageDraw()
    #define YUCHEN_PERF_NINE_SLICE() YuchenUI::Debug::PerformanceMonitor::getInstance().recordNineSliceDraw()
    #define YUCHEN_PERF_TEXT_DRAW() YuchenUI::Debug::PerformanceMonitor::getInstance().recordTextDraw()
    #define YUCHEN_PERF_TEXTURE_USAGE(name) YuchenUI::Debug::PerformanceMonitor::getInstance().recordTextureUsage(name)
#else
    #define YUCHEN_PERF_BEGIN_FRAME() ((void)0)
    #define YUCHEN_PERF_END_FRAME() ((void)0)
    #define YUCHEN_PERF_DRAW_CALL() ((void)0)
    #define YUCHEN_PERF_BUFFER_CREATE() ((void)0)
    #define YUCHEN_PERF_TEXTURE_SWITCH() ((void)0)
    #define YUCHEN_PERF_PIPELINE_SWITCH() ((void)0)
    #define YUCHEN_PERF_VERTICES(count) ((void)0)
    #define YUCHEN_PERF_IMAGE_DRAW() ((void)0)
    #define YUCHEN_PERF_NINE_SLICE() ((void)0)
    #define YUCHEN_PERF_TEXT_DRAW() ((void)0)
    #define YUCHEN_PERF_TEXTURE_USAGE(name) ((void)0)
#endif

} // namespace Debug
} // namespace YuchenUI
