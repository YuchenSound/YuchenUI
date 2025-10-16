#include "YuchenUI/widgets/ComboBox.h"
#include "YuchenUI/rendering/RenderList.h"
#include "YuchenUI/text/IFontProvider.h"
#include "YuchenUI/theme/IThemeProvider.h"
#include "YuchenUI/theme/Theme.h"
#include "YuchenUI/menu/MenuManager.h"
#include "YuchenUI/core/UIContext.h"
#include "YuchenUI/core/Validation.h"
#include "YuchenUI/core/Assert.h"
#include "YuchenUI/core/Config.h"

namespace YuchenUI {

ComboBox::ComboBox(const Rect& bounds)
    : UIComponent()
    , m_items()
    , m_selectedIndex(-1)
    , m_theme(ComboBoxTheme::Grey)
    , m_callback(nullptr)
    , m_placeholder("Please select...")
    , m_menu(nullptr)
    , m_isHovered(false)
    , m_menuNeedsRebuild(true)
{
    Validation::AssertRect(bounds);
    setBounds(bounds);
    setFocusPolicy(FocusPolicy::StrongFocus);
}

ComboBox::~ComboBox()
{
}

void ComboBox::addDrawCommands(RenderList& commandList, const Vec2& offset) const
{
    if (!m_isVisible) return;
    
    UIStyle* style = m_ownerContext ? m_ownerContext->getCurrentStyle() : nullptr;
    IFontProvider* fontProvider = m_ownerContext ? m_ownerContext->getFontProvider() : nullptr;
    YUCHEN_ASSERT(style);
    YUCHEN_ASSERT(fontProvider);
    
    ComboBoxDrawInfo info;
    info.bounds = Rect(m_bounds.x + offset.x, m_bounds.y + offset.y,
                      m_bounds.width, m_bounds.height);
    info.text = getSelectedText();
    info.placeholder = m_placeholder;
    info.isEmpty = (m_selectedIndex == -1);
    info.isHovered = m_isHovered;
    info.isEnabled = m_isEnabled;
    info.theme = m_theme;
    
    info.fallbackChain = style->getDefaultLabelFontChain();
    info.fontSize = 11.0f;
    
    style->drawComboBox(info, commandList);
    
    drawFocusIndicator(commandList, offset);
}

bool ComboBox::handleMouseMove(const Vec2& position, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    bool wasHovered = m_isHovered;
    m_isHovered = absRect.contains(position);
    
    return wasHovered != m_isHovered;
}

bool ComboBox::handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    Vec2 absPos(m_bounds.x + offset.x, m_bounds.y + offset.y);
    Rect absRect(absPos.x, absPos.y, m_bounds.width, m_bounds.height);
    
    if (pressed && absRect.contains(position))
    {
        requestFocus();
        openMenu();
        return true;
    }
    
    return false;
}

bool ComboBox::handleKeyPress(const Event& event)
{
    if (!m_isEnabled || !m_isVisible) return false;
    
    YUCHEN_ASSERT(event.type == EventType::KeyPressed || event.type == EventType::KeyReleased);
    if (event.type == EventType::KeyReleased) return false;
    
    switch (event.key.key)
    {
        case KeyCode::Space:
        case KeyCode::Return:
        case KeyCode::Enter:
        case KeyCode::KeypadEnter:
            openMenu();
            return true;
            
        case KeyCode::UpArrow:
            selectPreviousItem();
            return true;
            
        case KeyCode::DownArrow:
            selectNextItem();
            return true;
            
        default:
            break;
    }
    
    return false;
}

void ComboBox::addItem(const std::string& text, int value, bool enabled)
{
    m_items.emplace_back(text, value, enabled);
    m_menuNeedsRebuild = true;
}

void ComboBox::addGroup(const std::string& groupTitle)
{
    m_items.push_back(ComboBoxItem::Group(groupTitle));
    m_menuNeedsRebuild = true;
}

void ComboBox::addSeparator()
{
    m_items.push_back(ComboBoxItem::Separator());
    m_menuNeedsRebuild = true;
}

void ComboBox::setItems(const std::vector<ComboBoxItem>& items)
{
    m_items = items;
    
    if (m_selectedIndex >= static_cast<int>(m_items.size()) ||
        !isValidSelectableIndex(m_selectedIndex))
    {
        m_selectedIndex = -1;
    }
    
    m_menuNeedsRebuild = true;
}

void ComboBox::clearItems()
{
    m_items.clear();
    m_selectedIndex = -1;
    m_menuNeedsRebuild = true;
}

void ComboBox::setSelectedIndex(int index)
{
    if (index >= -1 && index < static_cast<int>(m_items.size()) &&
        (index == -1 || isValidSelectableIndex(index)))
    {
        m_selectedIndex = index;
    }
}

int ComboBox::getSelectedValue() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
    {
        return m_items[m_selectedIndex].value;
    }
    return -1;
}

std::string ComboBox::getSelectedText() const
{
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_items.size()))
    {
        return m_items[m_selectedIndex].text;
    }
    return "";
}

void ComboBox::setCallback(ComboBoxCallback callback)
{
    m_callback = callback;
}

void ComboBox::setTheme(ComboBoxTheme theme)
{
    m_theme = theme;
}

void ComboBox::setPlaceholder(const std::string& placeholder)
{
    m_placeholder = placeholder;
}

bool ComboBox::isValid() const
{
    return m_bounds.isValid();
}

CornerRadius ComboBox::getFocusIndicatorCornerRadius() const
{
    return CornerRadius(2.0f);
}

void ComboBox::buildMenu()
{
    if (!m_items.empty())
    {
        m_menu = MenuManager::getInstance().createMenu();
        
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            const ComboBoxItem& item = m_items[i];
            
            if (item.isSeparator)
            {
                m_menu->addSeparator();
            }
            else if (item.isGroup)
            {
                MenuItem* menuItem = m_menu->addItem(item.text);
                menuItem->setEnabled(false);
            }
            else
            {
                MenuItem* menuItem = m_menu->addItem(item.text, [this, i]() {
                    this->onMenuItemSelected(static_cast<int>(i));
                });
                menuItem->setEnabled(item.enabled);
            }
        }
        
        m_menu->build();
    }
    
    m_menuNeedsRebuild = false;
}

void ComboBox::onMenuItemSelected(int index)
{
    if (isValidSelectableIndex(index))
    {
        m_selectedIndex = index;
        
        if (m_callback)
        {
            m_callback(m_selectedIndex, getSelectedValue());
        }
    }
}

bool ComboBox::isValidSelectableIndex(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_items.size()))
    {
        return false;
    }
    
    const ComboBoxItem& item = m_items[index];
    return item.enabled && !item.isGroup && !item.isSeparator;
}

void ComboBox::openMenu()
{
    if (m_menuNeedsRebuild)
    {
        buildMenu();
    }
    
    if (!m_menu || !m_ownerContext) return;
    
    Rect windowRect = mapToWindow(m_bounds);
    Vec2 windowPos(windowRect.x, windowRect.y + windowRect.height);
    
    Vec2 screenPos = m_ownerContext->mapToScreen(windowPos);
    
    if (m_menuPopupHandler)
    {
        m_menuPopupHandler(screenPos, m_menu.get());
    }
    else
    {
        m_menu->popup(screenPos.x, screenPos.y);
    }
}

void ComboBox::selectNextItem()
{
    if (m_items.empty()) return;
    
    int nextIndex = findNextValidIndex(m_selectedIndex);
    if (nextIndex != -1)
    {
        setSelectedIndex(nextIndex);
        if (m_callback)
        {
            m_callback(m_selectedIndex, getSelectedValue());
        }
    }
}

void ComboBox::selectPreviousItem()
{
    if (m_items.empty()) return;
    
    int prevIndex = findPreviousValidIndex(m_selectedIndex);
    if (prevIndex != -1)
    {
        setSelectedIndex(prevIndex);
        if (m_callback)
        {
            m_callback(m_selectedIndex, getSelectedValue());
        }
    }
}

int ComboBox::findNextValidIndex(int startIndex) const
{
    int searchStart = (startIndex == -1) ? 0 : startIndex + 1;
    
    for (int i = searchStart; i < static_cast<int>(m_items.size()); ++i)
    {
        if (isValidSelectableIndex(i))
        {
            return i;
        }
    }
    
    for (int i = 0; i < searchStart && i <= startIndex; ++i)
    {
        if (isValidSelectableIndex(i))
        {
            return i;
        }
    }
    
    return -1;
}

int ComboBox::findPreviousValidIndex(int startIndex) const
{
    int searchStart = (startIndex == -1) ? static_cast<int>(m_items.size()) - 1 : startIndex - 1;
    
    for (int i = searchStart; i >= 0; --i)
    {
        if (isValidSelectableIndex(i))
        {
            return i;
        }
    }
    
    for (int i = static_cast<int>(m_items.size()) - 1; i > searchStart && i >= startIndex; --i)
    {
        if (isValidSelectableIndex(i))
        {
            return i;
        }
    }
    
    return -1;
}

} // namespace YuchenUI
