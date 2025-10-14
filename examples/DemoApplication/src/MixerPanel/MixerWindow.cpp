#include "MixerWindow.h"

MixerWindowContent::MixerWindowContent()
{
}

MixerWindowContent::~MixerWindowContent()
{
}

void MixerWindowContent::onCreate(YuchenUI::UIContext* context,
                                  const YuchenUI::Rect& contentArea)
{
   m_context = context;
   m_contentArea = contentArea;
   
   createUI();
}

void MixerWindowContent::createUI()
{
   IFontProvider* fontProvider = m_context->getFontProvider();
   
   // 标题标签
   YuchenUI::Rect titleBounds(10, 10, m_contentArea.width - 20, 40);
   m_titleLabel = std::make_unique<YuchenUI::TextLabel>(titleBounds);
   m_titleLabel->setText("Mixer (混音器)");
   m_titleLabel->setFont(fontProvider->getDefaultBoldFont());
   m_titleLabel->setFontSize(20.0f);
   m_titleLabel->setAlignment(YuchenUI::TextAlignment::Center,
                              YuchenUI::VerticalAlignment::Middle);
   addComponent(m_titleLabel.get());
   
   // 内容区域分组框
   YuchenUI::Rect groupBounds(10, 60, m_contentArea.width - 20, m_contentArea.height - 70);
   m_contentGroupBox = std::make_unique<YuchenUI::GroupBox>(groupBounds);
   m_contentGroupBox->setTitle("Welcome");
   m_contentGroupBox->setCornerRadius(4.0f);
   addComponent(m_contentGroupBox.get());
   
   // Hello World 标签 - 居中显示
   YuchenUI::Rect helloWorldBounds(
       10,
       (groupBounds.height - 50) / 2,  // 垂直居中
       groupBounds.width - 20,
       50
   );
   m_helloWorldLabel = std::make_unique<YuchenUI::TextLabel>(helloWorldBounds);
   m_helloWorldLabel->setText("Hello World!");
   m_helloWorldLabel->setFont(fontProvider->getDefaultBoldFont());
   m_helloWorldLabel->setFontSize(32.0f);
   m_helloWorldLabel->setAlignment(YuchenUI::TextAlignment::Center,
                                   YuchenUI::VerticalAlignment::Middle);
   m_contentGroupBox->addChild<YuchenUI::TextLabel>(*m_helloWorldLabel);
}

void MixerWindowContent::onDestroy()
{
   IUIContent::onDestroy();
}

void MixerWindowContent::render(YuchenUI::RenderList& commandList)
{
   if (m_titleLabel) {
       m_titleLabel->addDrawCommands(commandList);
   }
   if (m_contentGroupBox) {
       m_contentGroupBox->addDrawCommands(commandList);
   }
}
