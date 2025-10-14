#pragma once

#include <YuchenUI/YuchenUI-Desktop.h>
#include <memory>
#include <string>

using namespace YuchenUI;

/**
* 混音器窗口内容类
*
* 显示一个简单的混音器界面，包含 Hello World 文本
*/
class MixerWindowContent : public YuchenUI::IUIContent {
public:
   MixerWindowContent();
   virtual ~MixerWindowContent();
   
   // IUIContent 接口实现
   void onCreate(YuchenUI::UIContext* context, const YuchenUI::Rect& contentArea) override;
   void onDestroy() override;
   void render(YuchenUI::RenderList& commandList) override;

private:
   void createUI();
   
   // UI 组件
   std::unique_ptr<YuchenUI::TextLabel> m_titleLabel;
   std::unique_ptr<YuchenUI::TextLabel> m_helloWorldLabel;
   std::unique_ptr<YuchenUI::GroupBox> m_contentGroupBox;
};
