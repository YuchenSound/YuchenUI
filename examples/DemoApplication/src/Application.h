#pragma once

#include <YuchenUI/YuchenUI-Desktop.h>
#include <YuchenUI-Desktop/Application.h>
#include <memory>
#include <vector>

class MainWindowContent;
class ConfirmationDialogContent;
class InspectorWindowContent;
class MixerWindowContent;  // 新增：混音器窗口前向声明

// Demo应用程序类，管理业务逻辑
class DemoApplication {
public:
   DemoApplication();
   ~DemoApplication();
   
   bool initialize();
   int run();
   void shutdown();
   
   static DemoApplication* getInstance() { return s_instance; }
   static void setInstance(DemoApplication* instance) { s_instance = instance; }
   
   // 获取框架Application实例
   YuchenUI::Application& getFrameworkApp() { return m_frameworkApp; }

private:
   bool createMainWindow();

   void onShowLevelMeterClick();
   void onShowMixerClick();        // 新增：混音器按钮回调
   void onShowDialogClick();
   void onToggleThemeClick();
   
   YuchenUI::Application m_frameworkApp;
   YuchenUI::BaseWindow* m_mainWindow;
   YuchenUI::BaseWindow* m_levelMeterWindow;
   YuchenUI::BaseWindow* m_mixerWindow;    // 新增：混音器窗口指针
   
   bool m_isDarkTheme;

   static DemoApplication* s_instance;
   
   static constexpr int WINDOW_WIDTH = 800;
   static constexpr int WINDOW_HEIGHT = 750;
   
   friend class MainWindowContent;
};

class MainWindowContent : public YuchenUI::IUIContent {
public:
   MainWindowContent(DemoApplication* app);
   virtual ~MainWindowContent();
   
   void onCreate(YuchenUI::UIContext* context, const YuchenUI::Rect& contentArea) override;
   void onDestroy() override;
   void render(YuchenUI::RenderList& commandList) override;

private:
   void createTitleLabel();
   void createActionButtons();
   void createComboBoxTestArea();
   void createSpinBoxTestArea();
   void createTextInputTestArea();
   void createCheckBoxTestArea();
   void createRadioButtonTestArea();
   void createScrollAreaTestArea();
   void createKnobTestArea();

   DemoApplication* m_app;
   
   std::unique_ptr<YuchenUI::TextLabel> m_titleLabel;
   std::unique_ptr<YuchenUI::Button> m_levelMeterButton;
   std::unique_ptr<YuchenUI::Button> m_mixerButton;        // 新增：混音器按钮
   std::unique_ptr<YuchenUI::Button> m_dialogButton;
   std::unique_ptr<YuchenUI::Button> m_themeButton;
   
   std::unique_ptr<YuchenUI::GroupBox> m_comboBoxGroupBox;
   std::unique_ptr<YuchenUI::TextLabel> m_comboResultLabel;
   YuchenUI::ComboBox* m_deviceComboBox;
   YuchenUI::ComboBox* m_sampleRateComboBox;
   
   std::unique_ptr<YuchenUI::GroupBox> m_spinBoxGroupBox;
   std::unique_ptr<YuchenUI::TextLabel> m_spinBoxResultLabel;
   
   std::unique_ptr<YuchenUI::GroupBox> m_textInputGroupBox;
   
   std::unique_ptr<YuchenUI::GroupBox> m_checkBoxGroupBox;
   std::unique_ptr<YuchenUI::TextLabel> m_checkBoxResultLabel;
   
   std::unique_ptr<YuchenUI::GroupBox> m_radioButtonGroupBox;
   std::unique_ptr<YuchenUI::TextLabel> m_radioButtonResultLabel;
   std::unique_ptr<YuchenUI::RadioButtonGroup> m_radioGroup;
   
   std::unique_ptr<YuchenUI::GroupBox> m_scrollGroupBox;
   YuchenUI::ScrollArea* m_scrollArea;
   
   std::unique_ptr<YuchenUI::GroupBox> m_knobGroupBox;
   std::unique_ptr<YuchenUI::TextLabel> m_knobResultLabel;
   YuchenUI::Knob* m_volumeKnob;
   YuchenUI::Knob* m_panKnob;
   YuchenUI::Knob* m_filterKnob;
};

class ConfirmationDialogContent : public YuchenUI::IUIContent {
public:
   ConfirmationDialogContent(const std::string& message);
   virtual ~ConfirmationDialogContent();
   
   void onCreate(YuchenUI::UIContext* context, const YuchenUI::Rect& contentArea) override;
   void onDestroy() override;
   void render(YuchenUI::RenderList& commandList) override;

private:
   void createMessageFrame();
   void createButtonFrame();

   std::string m_message;
   std::unique_ptr<YuchenUI::Frame> m_messageFrame;
   YuchenUI::TextBlock* m_messageTextBlock;
   std::unique_ptr<YuchenUI::Frame> m_buttonFrame;
   YuchenUI::Button* m_confirmButton;
   YuchenUI::Button* m_cancelButton;
};

class LevelMeterWindowContent : public YuchenUI::IUIContent {
public:
   LevelMeterWindowContent();
   virtual ~LevelMeterWindowContent();
   
   void onCreate(YuchenUI::UIContext* context, const YuchenUI::Rect& contentArea) override;
   void onDestroy() override;
   void onUpdate(float deltaTime) override;
   void render(YuchenUI::RenderList& commandList) override;

private:
   void createUI();
   void updateLevelMeter();
   void generateTestSignal();
   void onStartStopClick();
   void onResetClick();
   void onChannelCountClick();
   void onScaleTypeClick();
   void updateStatusLabel();
   
   std::unique_ptr<YuchenUI::TextLabel> m_titleLabel;
   std::unique_ptr<YuchenUI::LevelMeter> m_levelMeter;
   std::unique_ptr<YuchenUI::GroupBox> m_controlGroupBox;
   std::unique_ptr<YuchenUI::Button> m_startStopButton;
   std::unique_ptr<YuchenUI::Button> m_resetButton;
   std::unique_ptr<YuchenUI::Button> m_channelButton;
   std::unique_ptr<YuchenUI::Button> m_scaleButton;
   std::unique_ptr<YuchenUI::TextLabel> m_statusLabel;
   
   // 状态变量
   bool m_isRunning;
   size_t m_currentChannelCount;
   YuchenUI::ScaleType m_currentScaleType;
   float m_time;
   std::vector<float> m_testLevels;
   float m_phase1;
   float m_phase2;
};
