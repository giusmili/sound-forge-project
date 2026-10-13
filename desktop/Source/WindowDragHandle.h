#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The app heading is a second drag handle, in addition to the native title bar.
class WindowDragHandle final : public juce::Label
{
public:
    WindowDragHandle()
    {
        setTooltip("Glisser pour deplacer la fenetre. Double-cliquer pour agrandir ou restaurer.");
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        draggedWindow = nullptr;
        if (!event.mods.isLeftButtonDown())
            return;

        auto* window = findParentComponentOfClass<juce::ResizableWindow>();
        if (window != nullptr && !window->isFullScreen())
        {
            draggedWindow = window;
            dragger.startDraggingComponent(window, event);
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (draggedWindow != nullptr)
            dragger.dragComponent(draggedWindow.getComponent(), event, nullptr);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggedWindow = nullptr;
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        draggedWindow = nullptr;
        if (event.mods.isLeftButtonDown())
            if (auto* window = findParentComponentOfClass<juce::ResizableWindow>())
                window->setFullScreen(!window->isFullScreen());
    }

private:
    juce::ComponentDragger dragger;
    juce::Component::SafePointer<juce::ResizableWindow> draggedWindow;
};
