#pragma once

#include "YuchenUI/widgets/UIComponent.h"
#include "YuchenUI/core/Types.h"
#include "YuchenUI/menu/Menu.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace YuchenUI {

class RenderList;

enum class ComboBoxTheme {
    Grey
};

struct ComboBoxItem {
    std::string text;
    int value;
    bool enabled;
    bool isGroup;
    bool isSeparator;
    
    ComboBoxItem()
        : text(), value(-1), enabled(true), isGroup(false), isSeparator(false) {}
    
    ComboBoxItem(const std::string& t, int v = -1, bool e = true)
        : text(t), value(v), enabled(e), isGroup(false), isSeparator(false) {}
    
    static ComboBoxItem Group(const std::string& groupTitle) {
        ComboBoxItem item;
        item.text = groupTitle;
        item.isGroup = true;
        item.enabled = false;
        return item;
    }
    
    static ComboBoxItem Separator() {
        ComboBoxItem item;
        item.isSeparator = true;
        item.enabled = false;
        return item;
    }
};

using ComboBoxCallback = std::function<void(int selectedIndex, int value)>;

class ComboBox : public UIComponent {
public:
    explicit ComboBox(const Rect& bounds);
    virtual ~ComboBox();
    
    void addDrawCommands(RenderList& commandList, const Vec2& offset = Vec2()) const override;
    bool handleMouseMove(const Vec2& position, const Vec2& offset = Vec2()) override;
    bool handleMouseClick(const Vec2& position, bool pressed, const Vec2& offset = Vec2()) override;
    bool handleKeyPress(const Event& event) override;
    const Rect& getBounds() const override { return m_bounds; }
    
    void addItem(const std::string& text, int value = -1, bool enabled = true);
    void addGroup(const std::string& groupTitle);
    void addSeparator();
    void setItems(const std::vector<ComboBoxItem>& items);
    void clearItems();
    
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return m_selectedIndex; }
    int getSelectedValue() const;
    std::string getSelectedText() const;
    
    void setCallback(ComboBoxCallback callback);
    
    void setTheme(ComboBoxTheme theme);
    ComboBoxTheme getTheme() const { return m_theme; }
    
    void setBounds(const Rect& bounds);
    
    void setPlaceholder(const std::string& placeholder);
    const std::string& getPlaceholder() const { return m_placeholder; }
    
    bool isValid() const;
    
    bool canReceiveFocus() const override { return m_isEnabled && m_isVisible; }

protected:
    CornerRadius getFocusIndicatorCornerRadius() const override;

private:
    void buildMenu();
    void onMenuItemSelected(int index);
    bool isValidSelectableIndex(int index) const;
    void openMenu();
    void selectNextItem();
    void selectPreviousItem();
    int findNextValidIndex(int startIndex) const;
    int findPreviousValidIndex(int startIndex) const;

    Rect m_bounds;
    std::vector<ComboBoxItem> m_items;
    int m_selectedIndex;
    ComboBoxTheme m_theme;
    ComboBoxCallback m_callback;
    std::string m_placeholder;
    
    std::unique_ptr<Menu> m_menu;
    bool m_isHovered;
    bool m_menuNeedsRebuild;
};

}
