#include "Application.h"
#include "MixerPanel/MixerWindow.h"
#include "YuchenUI/text/TextRenderer.h"

#include <iostream>
#include <sstream>
#include <iomanip>

namespace YuchenUI {
    class FontManager;
    class TextRenderer;
}

class MainWindowContent;
class ConfirmationDialogContent;
class LevelMeterWindowContent;

using namespace YuchenUI;

DemoApplication* DemoApplication::s_instance = nullptr;

DemoApplication::DemoApplication()
    : m_frameworkApp()
    , m_mainWindow(nullptr)
    , m_levelMeterWindow(nullptr)
    , m_mixerWindow(nullptr)      // æ·»åŠ æ··éŸ³å™¨çª—å£åˆå§‹åŒ–
    , m_isDarkTheme(true)
{
}

DemoApplication::~DemoApplication() {
}

bool DemoApplication::initialize() {
    
    m_frameworkApp.initialize();

    ThemeManager& themeManager = m_frameworkApp.getThemeManager();
    
    if (m_isDarkTheme)
    {
        themeManager.setStyle(std::make_unique<ProtoolsDarkStyle>());
    }
    else
    {
        themeManager.setStyle(std::make_unique<ProtoolsClassicStyle>());
    }

    m_frameworkApp.getFontManager();

    createMainWindow();
    return true;
}

bool DemoApplication::createMainWindow() {
    m_mainWindow = m_frameworkApp.createWindow<MainWindowContent>(WINDOW_WIDTH,WINDOW_HEIGHT,"UI Component Test",this);
    m_mainWindow->setAffectsAppLifetime(true);
    m_mainWindow->show();

    return true;
}

int DemoApplication::run() {
    int exitCode = m_frameworkApp.run();
    return exitCode;
}

void DemoApplication::onShowLevelMeterClick() {
    WindowManager& windowManager = m_frameworkApp.getWindowManager();

    if (m_levelMeterWindow && m_levelMeterWindow->isVisible())
    {
        m_levelMeterWindow->hide();
    }
    else
    {
        m_levelMeterWindow = windowManager.createToolWindow<LevelMeterWindowContent>(
            89, 554, "Level Meter Test", m_mainWindow);
        m_levelMeterWindow->show();
    }
}

// ============================================================================
// æ··éŸ³å™¨æŒ‰é’®ç‚¹å‡»å›žè°ƒ - å®Œæ•´å®žçŽ°
// ============================================================================
void DemoApplication::onShowMixerClick() {
    WindowManager& windowManager = m_frameworkApp.getWindowManager();

    if (m_mixerWindow && m_mixerWindow->isVisible())
    {
        m_mixerWindow->hide();
    }
    else
    {
        // Create as Main window type (independent top-level window)
        m_mixerWindow = windowManager.createMainWindow<MixerWindowContent>(
            500, 400, "Mixer (æ··éŸ³å™¨)");
        
        // Don't affect app lifetime - closing mixer won't quit app
        m_mixerWindow->setAffectsAppLifetime(false);
        
        m_mixerWindow->show();
    }
}

void DemoApplication::onShowDialogClick() {
    WindowManager& windowManager = m_frameworkApp.getWindowManager();

    BaseWindow* dialog = windowManager.createDialog<ConfirmationDialogContent>(420, 135, "Confirm", m_mainWindow,"Are you sure you want to perform this operation?");
    if (dialog) dialog->showModal();
}

void DemoApplication::onToggleThemeClick() {
    ThemeManager& themeManager = m_frameworkApp.getThemeManager();

    if (m_isDarkTheme) {
        themeManager.setStyle(std::make_unique<ProtoolsClassicStyle>());
        m_isDarkTheme = false;
    }
    else {
        themeManager.setStyle(std::make_unique<ProtoolsDarkStyle>());
        m_isDarkTheme = true;
    }
}

void DemoApplication::shutdown() {
    m_frameworkApp.quit();
}

//==========================================================================================
// MainWindowContent Implementation
//==========================================================================================

MainWindowContent::MainWindowContent(DemoApplication* app)
    : m_app(app)
    , m_deviceComboBox(nullptr)
    , m_sampleRateComboBox(nullptr)
    , m_scrollArea(nullptr)
    , m_volumeKnob(nullptr)
    , m_panKnob(nullptr)
    , m_filterKnob(nullptr)
{
}

MainWindowContent::~MainWindowContent() {
}

void MainWindowContent::onCreate(YuchenUI::UIContext* context, const YuchenUI::Rect& contentArea)
{
    m_context = context;
    m_contentArea = contentArea;

    createTitleLabel();
    createActionButtons();
    createComboBoxTestArea();
    createTextInputTestArea();
    createSpinBoxTestArea();
    createCheckBoxTestArea();
    createRadioButtonTestArea();
    createScrollAreaTestArea();
    createKnobTestArea();
}

void MainWindowContent::createTitleLabel() {
    IFontProvider* fontProvider = m_context->getFontProvider();

    YuchenUI::Rect titleBounds(0.0f, 0.0f, m_contentArea.width, 60.0f);
    m_titleLabel = std::make_unique<TextLabel>(titleBounds);
    m_titleLabel->setText("YuchenUI - Component Test");
    m_titleLabel->setFont(fontProvider->getDefaultBoldFont());
    m_titleLabel->setFontSize(24.0f);
    m_titleLabel->setAlignment(TextAlignment::Center, VerticalAlignment::Middle);
    addComponent(m_titleLabel.get());
}

// ============================================================================
// åˆ›å»ºåŠ¨ä½œæŒ‰é’® - å®Œæ•´å®žçŽ°ï¼ˆåŒ…å«æ··éŸ³å™¨æŒ‰é’®ï¼‰
// ============================================================================
void MainWindowContent::createActionButtons() {
    // Level Meter æŒ‰é’®
    YuchenUI::Rect levelMeterBounds(20.0f, 60.0f, 98.0f, 17.0f);
    m_levelMeterButton = std::make_unique<Button>(levelMeterBounds);
    m_levelMeterButton->setText("Level Meter");
    m_levelMeterButton->setRole(ButtonRole::Normal);
    m_levelMeterButton->setClickCallback([this]() {
        m_app->onShowLevelMeterClick();
    });
    addComponent(m_levelMeterButton.get());

    // æ··éŸ³å™¨æŒ‰é’®ï¼ˆæ–°å¢žï¼‰
    YuchenUI::Rect mixerBounds(130.0f, 60.0f, 98.0f, 17.0f);
    m_mixerButton = std::make_unique<Button>(mixerBounds);
    m_mixerButton->setText("Mixer");
    m_mixerButton->setRole(ButtonRole::Normal);
    m_mixerButton->setClickCallback([this]() {
        m_app->onShowMixerClick();
    });
    addComponent(m_mixerButton.get());

    // Dialog æŒ‰é’®ï¼ˆä½ç½®è°ƒæ•´ï¼‰
    YuchenUI::Rect dialogBounds(240.0f, 60.0f, 98.0f, 17.0f);
    m_dialogButton = std::make_unique<Button>(dialogBounds);
    m_dialogButton->setText("Dialog");
    m_dialogButton->setRole(ButtonRole::Primary);
    m_dialogButton->setClickCallback([this]() {
        m_app->onShowDialogClick();
    });
    addComponent(m_dialogButton.get());

    // Toggle Theme æŒ‰é’®ï¼ˆä½ç½®è°ƒæ•´ï¼‰
    YuchenUI::Rect themeBounds(350.0f, 60.0f, 108.0f, 17.0f);
    m_themeButton = std::make_unique<Button>(themeBounds);
    m_themeButton->setText("Toggle Theme");
    m_themeButton->setRole(ButtonRole::Normal);
    m_themeButton->setClickCallback([this]() {
        m_app->onToggleThemeClick();
    });
    addComponent(m_themeButton.get());
}

void MainWindowContent::createComboBoxTestArea()
{
    IFontProvider* fontProvider = m_context->getFontProvider();

    YuchenUI::Rect groupBounds(20.0f, 85.0f, 330.0f, 80.0f);
    m_comboBoxGroupBox = std::make_unique<GroupBox>(groupBounds);
    m_comboBoxGroupBox->setTitle("ComboBox Test");
    addComponent(m_comboBoxGroupBox.get());

    YuchenUI::Rect resultBounds(10.0f, 10.0f, groupBounds.width - 20, 17.0f);
    m_comboResultLabel = std::make_unique<TextLabel>(resultBounds);
    m_comboResultLabel->setText("Please select from the dropdowns above...");
    m_comboResultLabel->setFont(fontProvider->getDefaultBoldFont());
    m_comboResultLabel->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    m_comboBoxGroupBox->addChild<TextLabel>(*m_comboResultLabel);

    YuchenUI::Rect deviceBounds(10.0f, 30.0f, 150.0f, 17.0f);
    m_deviceComboBox = m_comboBoxGroupBox->addChild<ComboBox>(deviceBounds);
    m_deviceComboBox->setTheme(ComboBoxTheme::Grey);
    m_deviceComboBox->setPlaceholder("Select audio device...");
    m_deviceComboBox->addGroup("Input Devices");
    m_deviceComboBox->addItem("Built-in Microphone", 1);
    m_deviceComboBox->addItem("USB Microphone", 2);
    m_deviceComboBox->addItem("Bluetooth Headset", 3, false);
    m_deviceComboBox->addSeparator();
    m_deviceComboBox->addGroup("Output Devices");
    m_deviceComboBox->addItem("Built-in Speakers", 101);
    m_deviceComboBox->addItem("HDMI Audio", 102);
    m_deviceComboBox->addItem("Headphones", 103);
    m_deviceComboBox->setCallback([this](int index, int value) {
        std::ostringstream oss;
        oss << "Device: " << m_deviceComboBox->getSelectedText()
            << " (Value: " << value << ")";
        if (m_comboResultLabel) {
            m_comboResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect sampleRateBounds(170.0f, 30.0f, 150.0f, 17.0f);
    m_sampleRateComboBox = m_comboBoxGroupBox->addChild<ComboBox>(sampleRateBounds);
    m_sampleRateComboBox->setTheme(ComboBoxTheme::Grey);
    m_sampleRateComboBox->setPlaceholder("Select sample rate...");
    m_sampleRateComboBox->addItem("44.1 kHz", 44100);
    m_sampleRateComboBox->addItem("48 kHz", 48000);
    m_sampleRateComboBox->addItem("88.2 kHz", 88200);
    m_sampleRateComboBox->addItem("96 kHz", 96000);
    m_sampleRateComboBox->addItem("192 kHz", 192000);
    m_sampleRateComboBox->setSelectedIndex(1);
    m_sampleRateComboBox->setCallback([this](int index, int value) {
        std::ostringstream oss;
        oss << "Sample Rate: " << m_sampleRateComboBox->getSelectedText()
            << " (Value: " << value << ")";
        if (m_comboResultLabel) {
            m_comboResultLabel->setText(oss.str());
        }
    });
}

void MainWindowContent::createSpinBoxTestArea()
{
    IFontProvider* fontProvider = m_context->getFontProvider();
    
    YuchenUI::Rect groupBounds(20.0f, 265.0f, 330.0f, 80.0f);
    m_spinBoxGroupBox = std::make_unique<GroupBox>(groupBounds);
    m_spinBoxGroupBox->setTitle("SpinBox Test");
    m_spinBoxGroupBox->setCornerRadius(4.0f);
    addComponent(m_spinBoxGroupBox.get());
    
    YuchenUI::Rect resultBounds(10.0f, 10.0f, groupBounds.width - 20, 17.0f);
    m_spinBoxResultLabel = std::make_unique<TextLabel>(resultBounds);
    m_spinBoxResultLabel->setText("Adjust the values using mouse or keyboard...");
    m_spinBoxResultLabel->setFont(fontProvider->getDefaultBoldFont());
    m_spinBoxResultLabel->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    m_spinBoxGroupBox->addChild<TextLabel>(*m_spinBoxResultLabel);
    
    YuchenUI::Rect volumeBounds(10.0f, 35.0f, 80.0f, 17.0f);
    auto volumeSpinBox = m_spinBoxGroupBox->addChild<SpinBox>(volumeBounds);
    volumeSpinBox->setValue(75.0);
    volumeSpinBox->setMinValue(0.0);
    volumeSpinBox->setMaxValue(100.0);
    volumeSpinBox->setStep(5.0);
    volumeSpinBox->setPrecision(0);
    volumeSpinBox->setSuffix("%");
    volumeSpinBox->setFontSize(11.0f);
    volumeSpinBox->setValueChangedCallback([this](double value) {
        std::ostringstream oss;
        oss << "Volume: " << static_cast<int>(value) << "%";
        if (m_spinBoxResultLabel) {
            m_spinBoxResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect frequencyBounds(100.0f, 35.0f, 100.0f, 17.0f);
    auto frequencySpinBox = m_spinBoxGroupBox->addChild<SpinBox>(frequencyBounds);
    frequencySpinBox->setValue(1000.0);
    frequencySpinBox->setMinValue(20.0);
    frequencySpinBox->setMaxValue(20000.0);
    frequencySpinBox->setStep(10.0);
    frequencySpinBox->setPrecision(1);
    frequencySpinBox->setSuffix(" Hz");
    frequencySpinBox->setFontSize(11.0f);
    frequencySpinBox->setValueChangedCallback([this](double value) {
        std::ostringstream oss;
        oss << "Frequency: " << std::fixed << std::setprecision(1) << value << " Hz";
        if (m_spinBoxResultLabel) {
            m_spinBoxResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect gainBounds(210.0f, 35.0f, 100.0f, 17.0f);
    auto gainSpinBox = m_spinBoxGroupBox->addChild<SpinBox>(gainBounds);
    gainSpinBox->setValue(0.0);
    gainSpinBox->setMinValue(-12.0);
    gainSpinBox->setMaxValue(12.0);
    gainSpinBox->setStep(0.5);
    gainSpinBox->setPrecision(1);
    gainSpinBox->setSuffix(" dB");
    gainSpinBox->setFontSize(11.0f);
    gainSpinBox->setValueChangedCallback([this](double value) {
        std::ostringstream oss;
        oss << "Gain: " << std::fixed << std::setprecision(1)
            << (value >= 0 ? "+" : "") << value << " dB";
        if (m_spinBoxResultLabel) {
            m_spinBoxResultLabel->setText(oss.str());
        }
    });
}

void MainWindowContent::createTextInputTestArea()
{
    IFontProvider* fontProvider = m_context->getFontProvider();

    YuchenUI::Rect groupBounds(20.0f, 175.0f, 330.0f, 80.0f);
    m_textInputGroupBox = std::make_unique<GroupBox>(groupBounds);
    m_textInputGroupBox->setTitle("TextInput Test");
    m_textInputGroupBox->setCornerRadius(4.0f);
    addComponent(m_textInputGroupBox.get());

    YuchenUI::Rect nameBounds(10.0f, 10.0f, 150.0f, 17.0f);
    auto nameInput = m_textInputGroupBox->addChild<TextInput>(nameBounds);
    nameInput->setPlaceholder("Enter your name...");

    YuchenUI::Rect passwordBounds(170.0f, 10.0f, 150.0f, 17.0f);
    auto passwordInput = m_textInputGroupBox->addChild<TextInput>(passwordBounds);
    passwordInput->setPlaceholder("Enter password...");
    passwordInput->setPasswordMode(true);

    YuchenUI::Rect hintBounds(10.0f, 35.0f, groupBounds.width - 20, 17.0f);
    auto hintLabel = m_textInputGroupBox->addChild<TextLabel>(hintBounds);
    hintLabel->setText("Note: Supports input, copy and paste");
    hintLabel->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    hintLabel->setFont(fontProvider->getDefaultBoldFont());
}

void MainWindowContent::createCheckBoxTestArea()
{
    IFontProvider* fontProvider = m_context->getFontProvider();
    
    YuchenUI::Rect groupBounds(20.0f, 355.0f, 330.0f, 110.0f);
    m_checkBoxGroupBox = std::make_unique<GroupBox>(groupBounds);
    m_checkBoxGroupBox->setTitle("CheckBox Test");
    addComponent(m_checkBoxGroupBox.get());
    
    YuchenUI::Rect resultBounds(10.0f, 10.0f, groupBounds.width - 20, 17.0f);
    m_checkBoxResultLabel = std::make_unique<TextLabel>(resultBounds);
    m_checkBoxResultLabel->setText("Select your preferences...");
    m_checkBoxResultLabel->setFont(fontProvider->getDefaultBoldFont());
    m_checkBoxResultLabel->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    m_checkBoxGroupBox->addChild<TextLabel>(*m_checkBoxResultLabel);
    
    YuchenUI::Rect checkbox1Bounds(10.0f, 35.0f, 150.0f, 17.0f);
    auto checkbox1 = m_checkBoxGroupBox->addChild<CheckBox>(checkbox1Bounds);
    checkbox1->setText("Enable Auto-Save");
    checkbox1->setChecked(true);
    checkbox1->setStateChangedCallback([this](CheckBoxState state) {
        std::ostringstream oss;
        oss << "Auto-Save: " << (state == CheckBoxState::Checked ? "ON" : "OFF");
        if (m_checkBoxResultLabel) {
            m_checkBoxResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect checkbox2Bounds(170.0f, 35.0f, 150.0f, 17.0f);
    auto checkbox2 = m_checkBoxGroupBox->addChild<CheckBox>(checkbox2Bounds);
    checkbox2->setText("Show Tooltips");
    checkbox2->setStateChangedCallback([this](CheckBoxState state) {
        std::ostringstream oss;
        oss << "Tooltips: " << (state == CheckBoxState::Checked ? "VISIBLE" : "HIDDEN");
        if (m_checkBoxResultLabel) {
            m_checkBoxResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect checkbox3Bounds(10.0f, 60.0f, 150.0f, 17.0f);
    auto checkbox3 = m_checkBoxGroupBox->addChild<CheckBox>(checkbox3Bounds);
    checkbox3->setText("Enable Animations");
    checkbox3->setChecked(true);
    checkbox3->setStateChangedCallback([this](CheckBoxState state) {
        std::ostringstream oss;
        oss << "Animations: " << (state == CheckBoxState::Checked ? "ENABLED" : "DISABLED");
        if (m_checkBoxResultLabel) {
            m_checkBoxResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect checkbox4Bounds(170.0f, 60.0f, 150.0f, 17.0f);
    auto checkbox4 = m_checkBoxGroupBox->addChild<CheckBox>(checkbox4Bounds);
    checkbox4->setText("Dark Mode");
    checkbox4->setState(CheckBoxState::Indeterminate);
    checkbox4->setStateChangedCallback([this](CheckBoxState state) {
        std::ostringstream oss;
        oss << "Dark Mode: ";
        switch (state) {
            case CheckBoxState::Checked:
                oss << "ON";
                break;
            case CheckBoxState::Unchecked:
                oss << "OFF";
                break;
            case CheckBoxState::Indeterminate:
                oss << "AUTO";
                break;
        }
        if (m_checkBoxResultLabel) {
            m_checkBoxResultLabel->setText(oss.str());
        }
    });
}

void MainWindowContent::createRadioButtonTestArea()
{
    IFontProvider* fontProvider = m_context->getFontProvider();
    
    YuchenUI::Rect groupBounds(20.0f, 475.0f, 330.0f, 110.0f);
    m_radioButtonGroupBox = std::make_unique<GroupBox>(groupBounds);
    m_radioButtonGroupBox->setTitle("RadioButton Test");
    addComponent(m_radioButtonGroupBox.get());
    
    YuchenUI::Rect resultBounds(10.0f, 10.0f, groupBounds.width - 20, 17.0f);
    m_radioButtonResultLabel = std::make_unique<TextLabel>(resultBounds);
    m_radioButtonResultLabel->setText("Choose your quality setting...");
    m_radioButtonResultLabel->setFont(fontProvider->getDefaultBoldFont());
    m_radioButtonResultLabel->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    m_radioButtonGroupBox->addChild<TextLabel>(*m_radioButtonResultLabel);
    
    m_radioGroup = std::make_unique<RadioButtonGroup>();
    
    YuchenUI::Rect radio1Bounds(10.0f, 35.0f, 100.0f, 17.0f);
    auto radio1 = m_radioButtonGroupBox->addChild<RadioButton>(radio1Bounds);
    radio1->setText("Low Quality");
    radio1->setGroup(m_radioGroup.get());
    
    YuchenUI::Rect radio2Bounds(120.0f, 35.0f, 100.0f, 17.0f);
    auto radio2 = m_radioButtonGroupBox->addChild<RadioButton>(radio2Bounds);
    radio2->setText("Medium Quality");
    radio2->setGroup(m_radioGroup.get());
    
    YuchenUI::Rect radio3Bounds(230.0f, 35.0f, 90.0f, 17.0f);
    auto radio3 = m_radioButtonGroupBox->addChild<RadioButton>(radio3Bounds);
    radio3->setText("High Quality");
    radio3->setGroup(m_radioGroup.get());
    
    YuchenUI::Rect radio4Bounds(10.0f, 60.0f, 100.0f, 17.0f);
    auto radio4 = m_radioButtonGroupBox->addChild<RadioButton>(radio4Bounds);
    radio4->setText("Ultra Quality");
    radio4->setGroup(m_radioGroup.get());
    
    m_radioGroup->setSelectionCallback([this](int index, RadioButton* button) {
        std::ostringstream oss;
        oss << "Selected: " << button->getText() << " (Index: " << index << ")";
        if (m_radioButtonResultLabel) {
            m_radioButtonResultLabel->setText(oss.str());
        }
    });
    
    m_radioGroup->setCheckedIndex(1);
}

void MainWindowContent::createScrollAreaTestArea() {
    YuchenUI::Rect groupBounds(360.0f, 60.0f, 330.0f, 195.0f);
    m_scrollGroupBox = std::make_unique<GroupBox>(groupBounds);
    m_scrollGroupBox->setTitle("ScrollArea Test");
    m_scrollGroupBox->setCornerRadius(4.0f);
    addComponent(m_scrollGroupBox.get());

    YuchenUI::Rect scrollBounds(1.0f, 1.0f, groupBounds.width - 2.0f, 173.0f);
    m_scrollArea = m_scrollGroupBox->addChild<ScrollArea>(scrollBounds);
    m_scrollArea->setContentSize(Vec2(scrollBounds.width - 20, 1280.0f));
    m_scrollArea->setShowVerticalScrollbar(true);
    m_scrollArea->setShowHorizontalScrollbar(true);

    YuchenUI::Rect textBlockBounds(5.0f, 5.0f, scrollBounds.width - 10.0f, 1280.0f);
    auto longTextBlock = m_scrollArea->addChild<TextBlock>(textBlockBounds);
    
    longTextBlock->setText(
        "Paragraph 1: This is a test paragraph for the TextBlock component. TextBlock supports automatic line wrapping, which wraps text to the next line when it exceeds the width. This is very useful for displaying large blocks of text, such as articles, documentation, or chat logs.\n"
        "Paragraph 2: In practical applications, TextBlock can be used to display various types of text content. It supports mixed Chinese and English text, and can correctly handle the display and line breaking of Chinese characters. It also supports proper handling of punctuation marks to ensure the beauty and readability of text display.\n"
        "Paragraph 3: An important feature of the TextBlock component is support for paragraph spacing settings. Through the setParagraphSpacing method, you can control the blank distance between paragraphs to make text layout more beautiful. In addition, it also supports line height multiplier settings to adjust the spacing between lines.\n"
        "Paragraph 4: Testing the scroll area is also important. When the content of TextBlock exceeds the visible area, users can view the complete content through the scroll bar. The scroll bar supports mouse dragging, clicking the track to jump, and mouse wheel scrolling and other interactive methods.\n\n"
        "Paragraph 5: Text alignment is also an important typesetting feature. TextBlock supports multiple alignment methods such as left alignment, center alignment, and right alignment. It also supports vertical alignment settings to control the position of text in the vertical direction.\n"
        "Paragraph 6: Font and font size settings make text display more flexible. You can set different fonts for Western and Chinese text separately to ensure that text in different languages can get the best display effect. The font size can also be adjusted as needed.\n"
        "Paragraph 7: The padding setting can keep the text at a certain distance from the border to avoid text clinging to the edge. Through the setPadding method, you can set the padding in four directions: top, bottom, left and right, or set the padding in all directions uniformly.\n"
        "Paragraph 8: The text color setting can make the interface more colorful. You can set the text color through the setTextColor method, which supports complete control of four RGBA channels, and can achieve various color effects and transparency effects.\n\n"
        "Paragraph 9: This test paragraph continues. We need enough text content to test whether the scrolling function works properly. The scroll bar should be able to scroll smoothly and accurately reflect the current scroll position and scrollable range.\n\n"
        "Paragraph 10: Testing user interface components is a very important part of software development. Through adequate testing, we can discover potential problems, improve user experience, and improve software quality and stability.\n\n"
        "Paragraph 11: Continue to add more content to test the display effect of long text. The automatic line wrapping function of the text should be able to correctly handle various situations, including long words, punctuation marks, mixed Chinese and English and other complex scenarios.\n"
        "Paragraph 12: Scrolling performance is also a focus. When there is a lot of content, scrolling should remain smooth and there should be no lag or delay. This requires optimizing the rendering logic to only render the content in the visible area.\n\n"
        "Paragraph 13: Finally, let's summarize the main functions of the TextBlock component: automatic line wrapping, paragraph support, font settings, alignment methods, padding, text color, etc. These functions are combined together to form a fully functional text display component."
    );
    
    longTextBlock->setFontSize(11.0f);
    longTextBlock->setHorizontalAlignment(TextAlignment::Left);
    longTextBlock->setVerticalAlignment(VerticalAlignment::Top);
    longTextBlock->setPadding(10.0f);
    longTextBlock->setLineHeightMultiplier(1.15f);
}

void MainWindowContent::createKnobTestArea() {
    IFontProvider* fontProvider = m_context->getFontProvider();
    
    YuchenUI::Rect groupBounds(360.0f, 265.0f, 330.0f, 215.0f);
    m_knobGroupBox = std::make_unique<GroupBox>(groupBounds);
    m_knobGroupBox->setTitle("Knob Test - Enabled vs Disabled");
    m_knobGroupBox->setCornerRadius(4.0f);
    addComponent(m_knobGroupBox.get());
    
    YuchenUI::Rect resultBounds(10.0f, 10.0f, groupBounds.width - 20, 17.0f);
    m_knobResultLabel = std::make_unique<TextLabel>(resultBounds);
    m_knobResultLabel->setText("Try enabled knobs (Row 1), disabled are frozen (Row 2)...");
    m_knobResultLabel->setFont(fontProvider->getDefaultBoldFont());
    m_knobResultLabel->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    m_knobGroupBox->addChild<TextLabel>(*m_knobResultLabel);
    
    YuchenUI::Rect sectionLabel1Bounds(10.0f, 32.0f, groupBounds.width - 20, 12.0f);
    auto sectionLabel1 = m_knobGroupBox->addChild<TextLabel>(sectionLabel1Bounds);
    sectionLabel1->setText("Enabled Knobs (Interactive)");
    sectionLabel1->setFont(fontProvider->getDefaultBoldFont());
    sectionLabel1->setFontSize(10.0f);
    sectionLabel1->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    
    YuchenUI::Rect volumeKnobBounds(30.0f, 50.0f, 34.0f, 36.0f);
    m_volumeKnob = m_knobGroupBox->addChild<Knob>(volumeKnobBounds);
    m_volumeKnob->setKnobType(KnobType::NoCentered);
    m_volumeKnob->setValueRange(0.0f, 100.0f);
    m_volumeKnob->setValue(75.0f);
    m_volumeKnob->setDefaultValue(75.0f);
    m_volumeKnob->setSensitivity(1.0f);
    m_volumeKnob->setEnabled(true);
    m_volumeKnob->setOnValueChanged([this](float value) {
        std::ostringstream oss;
        oss << "Volume (Enabled): " << static_cast<int>(value) << "%";
        if (m_knobResultLabel) {
            m_knobResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect volumeLabelBounds(15.0f, 92.0f, 64.0f, 17.0f);
    auto volumeLabel = m_knobGroupBox->addChild<TextLabel>(volumeLabelBounds);
    volumeLabel->setText("Volume");
    volumeLabel->setFont(fontProvider->getDefaultFont());
    volumeLabel->setFontSize(10.0f);
    volumeLabel->setAlignment(TextAlignment::Center, VerticalAlignment::Top);
    
    YuchenUI::Rect panKnobBounds(120.0f, 50.0f, 34.0f, 36.0f);
    m_panKnob = m_knobGroupBox->addChild<Knob>(panKnobBounds);
    m_panKnob->setKnobType(KnobType::Centered);
    m_panKnob->setValueRange(-1.0f, 1.0f);
    m_panKnob->setValue(0.0f);
    m_panKnob->setDefaultValue(0.0f);
    m_panKnob->setSensitivity(0.8f);
    m_panKnob->setEnabled(true);
    m_panKnob->setOnValueChanged([this](float value) {
        std::ostringstream oss;
        oss << "Pan (Enabled): ";
        if (value < -0.05f) {
            oss << "L" << static_cast<int>(std::abs(value) * 100);
        } else if (value > 0.05f) {
            oss << "R" << static_cast<int>(value * 100);
        } else {
            oss << "Center";
        }
        if (m_knobResultLabel) {
            m_knobResultLabel->setText(oss.str());
        }
    });
    
    YuchenUI::Rect panLabelBounds(105.0f, 92.0f, 64.0f, 17.0f);
    auto panLabel = m_knobGroupBox->addChild<TextLabel>(panLabelBounds);
    panLabel->setText("Pan");
    panLabel->setFont(fontProvider->getDefaultFont());
    panLabel->setFontSize(10.0f);
    panLabel->setAlignment(TextAlignment::Center, VerticalAlignment::Top);
    
    YuchenUI::Rect sectionLabel2Bounds(10.0f, 115.0f, groupBounds.width - 20, 12.0f);
    auto sectionLabel2 = m_knobGroupBox->addChild<TextLabel>(sectionLabel2Bounds);
    sectionLabel2->setText("Disabled Knobs (Non-Interactive)");
    sectionLabel2->setFont(fontProvider->getDefaultBoldFont());
    sectionLabel2->setFontSize(10.0f);
    sectionLabel2->setAlignment(TextAlignment::Left, VerticalAlignment::Top);
    
    YuchenUI::Rect disabledVolumeKnobBounds(30.0f, 133.0f, 34.0f, 36.0f);
    auto disabledVolumeKnob = m_knobGroupBox->addChild<Knob>(disabledVolumeKnobBounds);
    disabledVolumeKnob->setKnobType(KnobType::NoCentered);
    disabledVolumeKnob->setValueRange(0.0f, 100.0f);
    disabledVolumeKnob->setValue(50.0f);
    disabledVolumeKnob->setDefaultValue(50.0f);
    disabledVolumeKnob->setEnabled(false);
    
    YuchenUI::Rect disabledVolumeLabelBounds(15.0f, 175.0f, 64.0f, 17.0f);
    auto disabledVolumeLabel = m_knobGroupBox->addChild<TextLabel>(disabledVolumeLabelBounds);
    disabledVolumeLabel->setText("Volume\n(Disabled)");
    disabledVolumeLabel->setFont(fontProvider->getDefaultFont());
    disabledVolumeLabel->setFontSize(9.0f);
    disabledVolumeLabel->setAlignment(TextAlignment::Center, VerticalAlignment::Top);
    
    YuchenUI::Rect disabledFilterKnobBounds(120.0f, 133.0f, 34.0f, 36.0f);
    m_filterKnob = m_knobGroupBox->addChild<Knob>(disabledFilterKnobBounds);
    m_filterKnob->setKnobType(KnobType::Centered);
    m_filterKnob->setValueRange(-1.0f, 1.0f);
    m_filterKnob->setValue(-0.5f);
    m_filterKnob->setDefaultValue(0.0f);
    m_filterKnob->setEnabled(false);
    
    YuchenUI::Rect disabledFilterLabelBounds(105.0f, 175.0f, 64.0f, 17.0f);
    auto disabledFilterLabel = m_knobGroupBox->addChild<TextLabel>(disabledFilterLabelBounds);
    disabledFilterLabel->setText("Filter\n(Disabled)");
    disabledFilterLabel->setFont(fontProvider->getDefaultFont());
    disabledFilterLabel->setFontSize(9.0f);
    disabledFilterLabel->setAlignment(TextAlignment::Center, VerticalAlignment::Top);
}

// ============================================================================
// æ¸²æŸ“æ–¹æ³• - å®Œæ•´å®žçŽ°ï¼ˆåŒ…å«æ··éŸ³å™¨æŒ‰é’®ï¼‰
// ============================================================================
void MainWindowContent::render(YuchenUI::RenderList& commandList)
{
    if (m_titleLabel) {
        m_titleLabel->addDrawCommands(commandList);
    }
    if (m_levelMeterButton) {
        m_levelMeterButton->addDrawCommands(commandList);
    }
    if (m_mixerButton) {  // æ¸²æŸ“æ··éŸ³å™¨æŒ‰é’®
        m_mixerButton->addDrawCommands(commandList);
    }
    if (m_dialogButton) {
        m_dialogButton->addDrawCommands(commandList);
    }
    if (m_themeButton) {
        m_themeButton->addDrawCommands(commandList);
    }
    if (m_comboBoxGroupBox) {
        m_comboBoxGroupBox->addDrawCommands(commandList);
    }
    if (m_spinBoxGroupBox) {
        m_spinBoxGroupBox->addDrawCommands(commandList);
    }
    if (m_textInputGroupBox) {
        m_textInputGroupBox->addDrawCommands(commandList);
    }
    if (m_checkBoxGroupBox) {
        m_checkBoxGroupBox->addDrawCommands(commandList);
    }
    if (m_radioButtonGroupBox) {
        m_radioButtonGroupBox->addDrawCommands(commandList);
    }
    if (m_scrollGroupBox) {
        m_scrollGroupBox->addDrawCommands(commandList);
    }
    if (m_knobGroupBox) {
        m_knobGroupBox->addDrawCommands(commandList);
    }
}

void MainWindowContent::onDestroy() {
    IUIContent::onDestroy();
}

//==========================================================================================
// ConfirmationDialogContent Implementation
//==========================================================================================

ConfirmationDialogContent::ConfirmationDialogContent(const std::string& message)
    : m_message(message)
    , m_confirmButton(nullptr)
    , m_cancelButton(nullptr)
{
}

ConfirmationDialogContent::~ConfirmationDialogContent() {}

void ConfirmationDialogContent::onCreate(YuchenUI::UIContext* context, const YuchenUI::Rect& contentArea) {
    m_context = context;
    m_contentArea = contentArea;

    createMessageFrame();
    createButtonFrame();
}

void ConfirmationDialogContent::createMessageFrame() {
    constexpr float BUTTON_HEIGHT = 17.0f;
    constexpr float BUTTON_SPACING = 7.0f;
    constexpr float WINDOW_PADDING = 5.0f;
    constexpr float FRAME_GAP = 3.0f;
    constexpr float CORNER_RADIUS = 2.0f;

    const float buttonFrameHeight = BUTTON_HEIGHT + BUTTON_SPACING * 2;
    const float buttonFrameY = m_contentArea.height - buttonFrameHeight - WINDOW_PADDING;
    
    const float messageFrameX = WINDOW_PADDING;
    const float messageFrameY = WINDOW_PADDING;
    const float messageFrameWidth = m_contentArea.width - WINDOW_PADDING * 2;
    const float messageFrameHeight = buttonFrameY - messageFrameY - FRAME_GAP;

    YuchenUI::Rect messageFrameBounds(messageFrameX, messageFrameY,
        messageFrameWidth, messageFrameHeight);
    m_messageFrame = std::make_unique<Frame>(messageFrameBounds);
    m_messageFrame->setCornerRadius(CORNER_RADIUS);
    m_messageFrame->setVisible(true);
    addComponent(m_messageFrame.get());

    YuchenUI::Rect messageTextBounds(BUTTON_SPACING, BUTTON_SPACING,
        messageFrameWidth - BUTTON_SPACING * 2,
        messageFrameHeight - BUTTON_SPACING * 2);
    m_messageTextBlock = m_messageFrame->addChild<TextBlock>(messageTextBounds);
    m_messageTextBlock->setText(m_message.c_str());
    m_messageTextBlock->setFontSize(13.0f);
    m_messageTextBlock->setAlignment(TextAlignment::Center, VerticalAlignment::Middle);
    m_messageTextBlock->setVisible(true);
}

void ConfirmationDialogContent::createButtonFrame() {
    constexpr float BUTTON_WIDTH = 78.0f;
    constexpr float BUTTON_HEIGHT = 17.0f;
    constexpr float BUTTON_SPACING = 7.0f;
    constexpr float WINDOW_PADDING = 5.0f;
    constexpr float CORNER_RADIUS = 2.0f;

    const float buttonFrameHeight = BUTTON_HEIGHT + BUTTON_SPACING * 2;
    const float buttonFrameX = WINDOW_PADDING;
    const float buttonFrameY = m_contentArea.height - buttonFrameHeight - WINDOW_PADDING;
    const float buttonFrameWidth = m_contentArea.width - WINDOW_PADDING * 2;

    YuchenUI::Rect buttonFrameBounds(buttonFrameX, buttonFrameY,
        buttonFrameWidth, buttonFrameHeight);
    m_buttonFrame = std::make_unique<Frame>(buttonFrameBounds);
    m_buttonFrame->setCornerRadius(CORNER_RADIUS);
    m_buttonFrame->setVisible(true);
    addComponent(m_buttonFrame.get());

    const float confirmButtonX = buttonFrameWidth - BUTTON_SPACING - BUTTON_WIDTH;
    YuchenUI::Rect confirmButtonBounds(confirmButtonX, BUTTON_SPACING,
        BUTTON_WIDTH, BUTTON_HEIGHT);
    m_confirmButton = m_buttonFrame->addChild<Button>(confirmButtonBounds);
    m_confirmButton->setText("Confirm");
    m_confirmButton->setRole(ButtonRole::Primary);
    m_confirmButton->setClickCallback([this]() {
        bool* confirmed = new bool(true);
        setUserData(confirmed);
        requestClose(WindowContentResult::Custom);
    });
    m_confirmButton->setVisible(true);

    const float cancelButtonX = confirmButtonX - BUTTON_SPACING - BUTTON_WIDTH;
    YuchenUI::Rect cancelButtonBounds(cancelButtonX, BUTTON_SPACING,
        BUTTON_WIDTH, BUTTON_HEIGHT);
    m_cancelButton = m_buttonFrame->addChild<Button>(cancelButtonBounds);
    m_cancelButton->setText("Cancel");
    m_cancelButton->setRole(ButtonRole::Normal);
    m_cancelButton->setClickCallback([this]() {
        bool* confirmed = new bool(false);
        setUserData(confirmed);
        requestClose(WindowContentResult::Custom);
    });
    m_cancelButton->setVisible(true);
}

void ConfirmationDialogContent::onDestroy() {
    m_messageTextBlock = nullptr;
    m_confirmButton = nullptr;
    m_cancelButton = nullptr;

    if (m_userData) {
        delete static_cast<bool*>(m_userData);
        m_userData = nullptr;
    }

    IUIContent::onDestroy();
}

void ConfirmationDialogContent::render(YuchenUI::RenderList& commandList)
{
    if (m_messageFrame) {
        m_messageFrame->addDrawCommands(commandList);
    }
    if (m_buttonFrame) {
        m_buttonFrame->addDrawCommands(commandList);
    }
}

//==========================================================================================
// LevelMeterWindowContent Implementation
//==========================================================================================

LevelMeterWindowContent::LevelMeterWindowContent()
    : m_isRunning(false)
    , m_currentChannelCount(2)
    , m_currentScaleType(YuchenUI::ScaleType::SAMPLE_PEAK)
    , m_time(0.0f)
    , m_phase1(0.0f)
    , m_phase2(0.0f)
{
    m_testLevels.resize(m_currentChannelCount, -144.0f);
}

LevelMeterWindowContent::~LevelMeterWindowContent() {
}

void LevelMeterWindowContent::onCreate(YuchenUI::UIContext* context,
                                       const YuchenUI::Rect& contentArea) {
    m_context = context;
    m_contentArea = contentArea;
    
    createUI();
}

void LevelMeterWindowContent::createUI() {
    IFontProvider* fontProvider = m_context->getFontProvider();
    
    YuchenUI::Rect titleBounds(10, 10, m_contentArea.width - 20, 25);
    m_titleLabel = std::make_unique<YuchenUI::TextLabel>(titleBounds);
    m_titleLabel->setText("Level Meter Test");
    m_titleLabel->setFont(fontProvider->getDefaultBoldFont());
    m_titleLabel->setFontSize(16.0f);
    m_titleLabel->setAlignment(YuchenUI::TextAlignment::Center,
                               YuchenUI::VerticalAlignment::Middle);
    addComponent(m_titleLabel.get());
    
    YuchenUI::Vec2 meterSize = YuchenUI::Vec2(0, 0);
    float meterX = (m_contentArea.width - 40) * 0.5f;
    YuchenUI::Rect meterBounds(meterX, 45, 0, 0);
    m_levelMeter = std::make_unique<YuchenUI::LevelMeter>(
        m_context,
        meterBounds,
        m_currentChannelCount,
        m_currentScaleType
    );

    m_levelMeter->setShowControlVoltage(true);
    m_levelMeter->setDecayRate(40.0f);
    m_levelMeter->setPeakHoldTime(3000.0f);
    addComponent(m_levelMeter.get());
    
    YuchenUI::Rect controlGroupBounds(10, 295, m_contentArea.width - 20, 100);
    m_controlGroupBox = std::make_unique<YuchenUI::GroupBox>(controlGroupBounds);
    m_controlGroupBox->setTitle("Controls");
    m_controlGroupBox->setCornerRadius(4.0f);
    addComponent(m_controlGroupBox.get());
    
    YuchenUI::Rect startStopBounds(10, 10, 160, 17);
    m_startStopButton = std::make_unique<YuchenUI::Button>(startStopBounds);
    m_startStopButton->setText("Start");
    m_startStopButton->setRole(YuchenUI::ButtonRole::Primary);
    m_startStopButton->setClickCallback([this]() {
        onStartStopClick();
    });
    m_controlGroupBox->addChild<YuchenUI::Button>(*m_startStopButton);
    
    YuchenUI::Rect resetBounds(180, 10, 160, 17);
    m_resetButton = std::make_unique<YuchenUI::Button>(resetBounds);
    m_resetButton->setText("Reset");
    m_resetButton->setRole(YuchenUI::ButtonRole::Normal);
    m_resetButton->setClickCallback([this]() {
        onResetClick();
    });
    m_controlGroupBox->addChild<YuchenUI::Button>(*m_resetButton);
    
    YuchenUI::Rect channelBounds(10, 35, 160, 17);
    m_channelButton = std::make_unique<YuchenUI::Button>(channelBounds);
    m_channelButton->setText("Channels: 2");
    m_channelButton->setRole(YuchenUI::ButtonRole::Normal);
    m_channelButton->setClickCallback([this]() {
        onChannelCountClick();
    });
    m_controlGroupBox->addChild<YuchenUI::Button>(*m_channelButton);
    
    YuchenUI::Rect scaleBounds(180, 35, 160, 17);
    m_scaleButton = std::make_unique<YuchenUI::Button>(scaleBounds);
    m_scaleButton->setText("Scale: Sample Peak");
    m_scaleButton->setRole(YuchenUI::ButtonRole::Normal);
    m_scaleButton->setClickCallback([this]() {
        onScaleTypeClick();
    });
    m_controlGroupBox->addChild<YuchenUI::Button>(*m_scaleButton);
    
    YuchenUI::Rect statusBounds(10, 60, controlGroupBounds.width - 20, 15);
    m_statusLabel = std::make_unique<YuchenUI::TextLabel>(statusBounds);
    m_statusLabel->setText("Status: Stopped");
    m_statusLabel->setFont(fontProvider->getDefaultFont());
    m_statusLabel->setFontSize(10.0f);
    m_statusLabel->setAlignment(YuchenUI::TextAlignment::Left,
                                YuchenUI::VerticalAlignment::Top);
    m_controlGroupBox->addChild<YuchenUI::TextLabel>(*m_statusLabel);
}

void LevelMeterWindowContent::onStartStopClick() {
    m_isRunning = !m_isRunning;
    
    if (m_startStopButton) {
        m_startStopButton->setText(m_isRunning ? "Stop" : "Start");
    }
    
    if (!m_isRunning) {
        for (auto& level : m_testLevels) {
            level = -144.0f;
        }
        if (m_levelMeter) {
            m_levelMeter->updateLevels(m_testLevels);
        }
    }
    
    updateStatusLabel();
}

void LevelMeterWindowContent::onResetClick() {
    m_time = 0.0f;
    m_phase1 = 0.0f;
    m_phase2 = 0.0f;
    
    if (m_levelMeter) {
        m_levelMeter->reset();
    }
    
    for (auto& level : m_testLevels) {
        level = -144.0f;
    }
    
    updateStatusLabel();
}

void LevelMeterWindowContent::onChannelCountClick() {
    if (m_currentChannelCount == 1) {
        m_currentChannelCount = 2;
    } else if (m_currentChannelCount == 2) {
        m_currentChannelCount = 4;
    } else if (m_currentChannelCount == 4) {
        m_currentChannelCount = 8;
    } else {
        m_currentChannelCount = 1;
    }
    
    m_testLevels.resize(m_currentChannelCount, -144.0f);
    
    if (m_levelMeter) {
        m_levelMeter->setChannelCount(m_currentChannelCount);
        
        YuchenUI::Vec2 recommendedSize = m_levelMeter->getRecommendedSize();
        float meterX = (m_contentArea.width - recommendedSize.x) * 0.5f;
        YuchenUI::Rect newBounds(meterX, 45, recommendedSize.x, recommendedSize.y);
        m_levelMeter->setBounds(newBounds);
    }
    
    if (m_channelButton) {
        std::ostringstream oss;
        oss << "Channels: " << m_currentChannelCount;
        m_channelButton->setText(oss.str());
    }
    
    updateStatusLabel();
}

void LevelMeterWindowContent::onScaleTypeClick() {
    switch (m_currentScaleType) {
        case YuchenUI::ScaleType::SAMPLE_PEAK:
            m_currentScaleType = YuchenUI::ScaleType::K12;
            break;
        case YuchenUI::ScaleType::K12:
            m_currentScaleType = YuchenUI::ScaleType::K14;
            break;
        case YuchenUI::ScaleType::K14:
            m_currentScaleType = YuchenUI::ScaleType::VU;
            break;
        case YuchenUI::ScaleType::VU:
            m_currentScaleType = YuchenUI::ScaleType::LINEAR_DB;
            break;
        case YuchenUI::ScaleType::LINEAR_DB:
            m_currentScaleType = YuchenUI::ScaleType::SAMPLE_PEAK;
            break;
    }
    
    if (m_levelMeter) {
        m_levelMeter->setScaleType(m_currentScaleType);
        
        if (m_scaleButton) {
            std::ostringstream oss;
            oss << "Scale: " << m_levelMeter->getScaleTypeName();
            m_scaleButton->setText(oss.str());
        }
    }
}

void LevelMeterWindowContent::updateStatusLabel() {
    if (!m_statusLabel) return;
    
    std::ostringstream oss;
    oss << "Status: " << (m_isRunning ? "Running" : "Stopped");
    oss << " | Channels: " << m_currentChannelCount;
    oss << " | Time: " << std::fixed << std::setprecision(1) << m_time << "s";
    
    m_statusLabel->setText(oss.str());
}

void LevelMeterWindowContent::generateTestSignal() {
    if (m_testLevels.size() != m_currentChannelCount) {
        m_testLevels.resize(m_currentChannelCount);
    }
    
    for (size_t i = 0; i < m_currentChannelCount; ++i) {
        float frequency1 = 0.5f + i * 0.2f;
        float frequency2 = 1.5f + i * 0.3f;
        
        float signal1 = std::sin(m_phase1 * frequency1) * 0.5f;
        float signal2 = std::sin(m_phase2 * frequency2) * 0.3f;
        float noise = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.1f;
        
        float amplitude = signal1 + signal2 + noise;
        amplitude = std::clamp(amplitude, -1.0f, 1.0f);
        
        float levelDb;
        if (std::abs(amplitude) < 0.00001f) {
            levelDb = -144.0f;
        } else {
            levelDb = 20.0f * std::log10(std::abs(amplitude));
            levelDb = std::clamp(levelDb, -144.0f, 0.0f);
        }
        
        m_testLevels[i] = levelDb;
    }
    
    m_phase1 += 0.1f;
    m_phase2 += 0.15f;
    if (m_phase1 > 2.0f * 3.14159f) m_phase1 -= 2.0f * 3.14159f;
    if (m_phase2 > 2.0f * 3.14159f) m_phase2 -= 2.0f * 3.14159f;
}

void LevelMeterWindowContent::updateLevelMeter() {
    if (!m_levelMeter || !m_isRunning) return;
    
    generateTestSignal();
    m_levelMeter->updateLevels(m_testLevels);
    
    float cvLevel = -35.0f + std::sin(m_time * 2.0f) * 15.0f;
    m_levelMeter->updateControlVoltage(cvLevel);
}

void LevelMeterWindowContent::onUpdate(float deltaTime) {
    if (m_isRunning) {
        m_time += deltaTime;
        updateLevelMeter();
        updateStatusLabel();
    }
}

void LevelMeterWindowContent::onDestroy() {
    IUIContent::onDestroy();
}

void LevelMeterWindowContent::render(YuchenUI::RenderList& commandList)
{
    if (m_titleLabel) {
        m_titleLabel->addDrawCommands(commandList);
    }
    if (m_levelMeter) {
        m_levelMeter->addDrawCommands(commandList);
    }
    if (m_controlGroupBox) {
        m_controlGroupBox->addDrawCommands(commandList);
    }
}
