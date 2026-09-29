#include "TrackRowComponent.h"

#include <cmath>

namespace
{
constexpr auto rowColour = 0xff222831;
constexpr auto timelineColour = 0xff15191f;
constexpr auto gridColour = 0xff252c34;
constexpr auto clipColour = 0xff244d6e;
constexpr auto clipSelectedColour = 0xff336f9e;
constexpr auto clipDragColour = 0xff3b7fae;
constexpr auto selectionBorderColour = 0xffffc857;
constexpr auto trimHandleColour = 0xffffd66b;
constexpr auto waveformColour = 0xff81c7ff;
constexpr auto textColour = 0xffe8edf3;
constexpr auto accentColour = 0xff5aa9ff;
constexpr auto muteColour = 0xffd65454;
constexpr auto soloColour = 0xffd4a62a;
constexpr auto playheadColour = 0xffffb347;
constexpr int trimHandleWidth = 8;
constexpr double minimumClipDurationSeconds = 0.01;
}

TrackRowComponent::TrackRowComponent(
    AudioTrack& trackToControl,
    juce::AudioFormatManager& formatManager,
    juce::AudioThumbnailCache& thumbnailCache
)
    : track(trackToControl),
      thumbnail(512, formatManager, thumbnailCache)
{
    thumbnail.addChangeListener(this);
    thumbnail.setSource(
        new juce::FileInputSource(track.getSourceFile())
    );

    nameLabel.setText(
        track.getName(),
        juce::dontSendNotification
    );
    nameLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour)
    );
    nameLabel.setFont(
        juce::FontOptions(16.0f, juce::Font::bold)
    );
    addAndMakeVisible(nameLabel);

    volumeLabel.setText("Vol", juce::dontSendNotification);
    volumeLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour).withAlpha(0.75f)
    );
    addAndMakeVisible(volumeLabel);

    panLabel.setText("Pan", juce::dontSendNotification);
    panLabel.setColour(
        juce::Label::textColourId,
        juce::Colour(textColour).withAlpha(0.75f)
    );
    addAndMakeVisible(panLabel);

    muteButton.onClick = [this]
    {
        track.setMuted(! track.isMuted());
        refreshMuteButton();
    };
    addAndMakeVisible(muteButton);

    soloButton.onClick = [this]
    {
        track.setSolo(! track.isSolo());
        refreshSoloButton();

        if (onSoloChanged)
            onSoloChanged();
    };
    addAndMakeVisible(soloButton);

    volumeSlider.setRange(0.0, 1.5, 0.01);
    volumeSlider.setValue(track.getGain());
    volumeSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );
    volumeSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        52,
        22
    );
    volumeSlider.setColour(
        juce::Slider::trackColourId,
        juce::Colour(accentColour)
    );
    volumeSlider.onValueChange = [this]
    {
        track.setGain(
            static_cast<float>(volumeSlider.getValue())
        );
    };
    addAndMakeVisible(volumeSlider);

    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(track.getPan());
    panSlider.setSliderStyle(
        juce::Slider::LinearHorizontal
    );
    panSlider.setTextBoxStyle(
        juce::Slider::TextBoxRight,
        false,
        52,
        22
    );
    panSlider.onValueChange = [this]
    {
        track.setPan(
            static_cast<float>(panSlider.getValue())
        );
    };
    addAndMakeVisible(panSlider);

    refreshMuteButton();
    refreshSoloButton();
}

TrackRowComponent::~TrackRowComponent()
{
    thumbnail.removeChangeListener(this);
}

void TrackRowComponent::paint(juce::Graphics& graphics)
{
    const auto bounds = getLocalBounds().reduced(2);

    graphics.setColour(juce::Colour(rowColour));
    graphics.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    const auto waveformBounds = getWaveformBounds();

    graphics.setColour(juce::Colour(timelineColour));
    graphics.fillRect(waveformBounds);

    graphics.saveState();
    graphics.reduceClipRegion(waveformBounds);

    if (projectLength > 0.0 && viewDuration > 0.0)
    {
        if (snapInterval > 0.0)
        {
            const auto viewEnd = viewStart + viewDuration;

            auto firstGrid =
                std::floor(viewStart / snapInterval) * snapInterval;

            if (firstGrid < viewStart - 0.001)
                firstGrid += snapInterval;

            graphics.setColour(juce::Colour(gridColour));

            for (double gridPosition = firstGrid;
                 gridPosition <= viewEnd + 0.001;
                 gridPosition += snapInterval)
            {
                const auto ratio =
                    (gridPosition - viewStart) / viewDuration;

                const auto x =
                    waveformBounds.getX()
                    + static_cast<int>(
                        std::round(
                            ratio * waveformBounds.getWidth()
                        )
                    );

                graphics.drawVerticalLine(
                    x,
                    static_cast<float>(waveformBounds.getY()),
                    static_cast<float>(waveformBounds.getBottom())
                );
            }
        }

        const auto clipBounds = getClipBounds();
        const auto isDragging = dragMode != DragMode::none;

        graphics.setColour(
            juce::Colour(
                isDragging
                    ? clipDragColour
                    : selected
                        ? clipSelectedColour
                        : clipColour
            )
        );

        graphics.fillRoundedRectangle(
            clipBounds.toFloat(),
            4.0f
        );

        if (selected)
        {
            graphics.setColour(
                juce::Colour(selectionBorderColour)
            );
            graphics.drawRoundedRectangle(
                clipBounds.toFloat().reduced(1.0f),
                4.0f,
                2.0f
            );

            graphics.setColour(
                juce::Colour(trimHandleColour)
            );
            graphics.fillRect(
                clipBounds.getX(),
                clipBounds.getY() + 4,
                3,
                juce::jmax(1, clipBounds.getHeight() - 8)
            );
            graphics.fillRect(
                clipBounds.getRight() - 3,
                clipBounds.getY() + 4,
                3,
                juce::jmax(1, clipBounds.getHeight() - 8)
            );
        }

        if (thumbnail.getTotalLength() > 0.0
            && clipBounds.getWidth() > 1)
        {
            graphics.setColour(
                juce::Colour(waveformColour)
            );

            thumbnail.drawChannels(
                graphics,
                clipBounds.reduced(4, 7),
                track.getSourceStartSeconds(),
                track.getSourceEndSeconds(),
                1.0f
            );
        }

        if (clipBounds.getWidth() > 82)
        {
            graphics.setColour(
                juce::Colour(textColour).withAlpha(0.85f)
            );
            graphics.setFont(10.0f);
            graphics.drawText(
                juce::String(
                    track.getStartOffsetSeconds(),
                    2
                )
                    + " s  |  "
                    + juce::String(
                        track.getClipDurationSeconds(),
                        2
                    )
                    + " s",
                clipBounds.reduced(6, 3).removeFromTop(14),
                juce::Justification::centredLeft
            );
        }

        const auto viewEnd = viewStart + viewDuration;

        if (playheadPosition >= viewStart
            && playheadPosition <= viewEnd)
        {
            const auto ratio =
                (playheadPosition - viewStart) / viewDuration;

            const auto playheadX =
                waveformBounds.getX()
                + static_cast<int>(
                    std::round(
                        ratio * waveformBounds.getWidth()
                    )
                );

            graphics.setColour(
                juce::Colour(playheadColour)
            );
            graphics.fillRect(
                playheadX - 1,
                waveformBounds.getY(),
                2,
                waveformBounds.getHeight()
            );
        }
    }

    graphics.restoreState();

    graphics.setColour(
        juce::Colour(0xff39424e)
    );
    graphics.drawVerticalLine(
        TimelineRulerComponent::controlsWidth,
        0.0f,
        static_cast<float>(getHeight())
    );
}

void TrackRowComponent::resized()
{
    auto controls = getLocalBounds()
        .withWidth(
            TimelineRulerComponent::controlsWidth
        )
        .reduced(10, 8);

    nameLabel.setBounds(
        controls.removeFromTop(26)
    );

    auto buttons = controls.removeFromTop(28);
    muteButton.setBounds(
        buttons.removeFromLeft(46)
    );
    buttons.removeFromLeft(6);
    soloButton.setBounds(
        buttons.removeFromLeft(46)
    );

    controls.removeFromTop(2);

    auto volumeArea = controls.removeFromTop(26);
    volumeLabel.setBounds(
        volumeArea.removeFromLeft(34)
    );
    volumeSlider.setBounds(volumeArea);

    auto panArea = controls.removeFromTop(26);
    panLabel.setBounds(
        panArea.removeFromLeft(34)
    );
    panSlider.setBounds(panArea);
}

void TrackRowComponent::mouseDown(
    const juce::MouseEvent& event
)
{
    const auto clipBounds = getClipBounds();

    if (projectLength > 0.0
        && viewDuration > 0.0
        && clipBounds.contains(event.getPosition()))
    {
        if (onSelectionRequested)
            onSelectionRequested(this);

        editSnapshotSent = false;
        dragStartX = event.position.x;
        dragStartOffset = track.getStartOffsetSeconds();
        dragSourceStart = track.getSourceStartSeconds();
        dragSourceEnd = track.getSourceEndSeconds();
        dragViewDuration = viewDuration;

        if (std::abs(
                event.position.x
                - static_cast<float>(clipBounds.getX())
            ) <= trimHandleWidth)
        {
            dragMode = DragMode::trimLeft;
        }
        else if (std::abs(
                     event.position.x
                     - static_cast<float>(clipBounds.getRight())
                 ) <= trimHandleWidth)
        {
            dragMode = DragMode::trimRight;
        }
        else
        {
            dragMode = DragMode::move;
        }

        repaint();
        return;
    }

    const auto waveformBounds = getWaveformBounds();

    if (projectLength <= 0.0
        || viewDuration <= 0.0
        || ! waveformBounds.contains(event.getPosition()))
    {
        return;
    }

    const auto relativeX =
        event.position.x
        - static_cast<float>(waveformBounds.getX());

    const auto ratio = juce::jlimit(
        0.0,
        1.0,
        static_cast<double>(relativeX)
            / static_cast<double>(
                waveformBounds.getWidth()
            )
    );

    if (onSeek)
    {
        onSeek(
            juce::jlimit(
                0.0,
                projectLength,
                viewStart + ratio * viewDuration
            )
        );
    }
}

void TrackRowComponent::mouseDrag(
    const juce::MouseEvent& event
)
{
    if (dragMode == DragMode::none
        || dragViewDuration <= 0.0)
    {
        return;
    }

    const auto waveformBounds = getWaveformBounds();

    if (waveformBounds.getWidth() <= 0)
        return;

    if (! editSnapshotSent)
    {
        if (onEditBegin)
            onEditBegin();

        editSnapshotSent = true;
    }

    const auto deltaPixels =
        event.position.x - dragStartX;

    const auto deltaSeconds =
        static_cast<double>(deltaPixels)
        / static_cast<double>(waveformBounds.getWidth())
        * dragViewDuration;

    if (dragMode == DragMode::move)
    {
        auto newOffset = juce::jmax(
            0.0,
            dragStartOffset + deltaSeconds
        );

        if (snapEnabled && snapInterval > 0.0)
        {
            newOffset =
                std::round(newOffset / snapInterval)
                * snapInterval;
        }

        track.setStartOffsetSeconds(newOffset);
    }
    else if (dragMode == DragMode::trimLeft)
    {
        const auto fixedProjectEnd =
            dragStartOffset
            + (dragSourceEnd - dragSourceStart);

        const auto earliestProjectStart =
            juce::jmax(
                0.0,
                dragStartOffset - dragSourceStart
            );

        auto newProjectStart =
            dragStartOffset + deltaSeconds;

        if (snapEnabled && snapInterval > 0.0)
        {
            newProjectStart =
                std::round(newProjectStart / snapInterval)
                * snapInterval;
        }

        newProjectStart = juce::jlimit(
            earliestProjectStart,
            fixedProjectEnd - minimumClipDurationSeconds,
            newProjectStart
        );

        const auto newSourceStart =
            dragSourceStart
            + (newProjectStart - dragStartOffset);

        track.setStartOffsetSeconds(newProjectStart);
        track.setSourceRange(
            newSourceStart,
            dragSourceEnd
        );
    }
    else if (dragMode == DragMode::trimRight)
    {
        const auto initialProjectEnd =
            dragStartOffset
            + (dragSourceEnd - dragSourceStart);

        const auto maximumProjectEnd =
            dragStartOffset
            + (track.getLengthSeconds() - dragSourceStart);

        auto newProjectEnd =
            initialProjectEnd + deltaSeconds;

        if (snapEnabled && snapInterval > 0.0)
        {
            newProjectEnd =
                std::round(newProjectEnd / snapInterval)
                * snapInterval;
        }

        newProjectEnd = juce::jlimit(
            dragStartOffset + minimumClipDurationSeconds,
            maximumProjectEnd,
            newProjectEnd
        );

        const auto newSourceEnd =
            dragSourceEnd
            + (newProjectEnd - initialProjectEnd);

        track.setSourceRange(
            dragSourceStart,
            newSourceEnd
        );
    }

    if (onClipMoved)
        onClipMoved();

    repaint();
}

void TrackRowComponent::mouseUp(
    const juce::MouseEvent& event
)
{
    if (dragMode == DragMode::none)
        return;

    dragMode = DragMode::none;
    editSnapshotSent = false;

    if (onClipMoved)
        onClipMoved();

    updateMouseCursor(event.position);
    repaint();
}

void TrackRowComponent::mouseMove(
    const juce::MouseEvent& event
)
{
    updateMouseCursor(event.position);
}

void TrackRowComponent::mouseExit(
    const juce::MouseEvent&
)
{
    if (dragMode == DragMode::none)
    {
        setMouseCursor(
            juce::MouseCursor(
                juce::MouseCursor::NormalCursor
            )
        );
    }
}

void TrackRowComponent::setTimelineState(
    const double projectLengthSeconds,
    const double playheadSeconds,
    const double viewStartSeconds,
    const double viewDurationSeconds
)
{
    projectLength = juce::jmax(
        0.0,
        projectLengthSeconds
    );

    playheadPosition = juce::jlimit(
        0.0,
        projectLength,
        playheadSeconds
    );

    viewDuration = juce::jlimit(
        0.0,
        projectLength,
        viewDurationSeconds
    );

    const auto maxStart = juce::jmax(
        0.0,
        projectLength - viewDuration
    );

    viewStart = juce::jlimit(
        0.0,
        maxStart,
        viewStartSeconds
    );

    repaint();
}

void TrackRowComponent::setSnapSettings(
    const bool enabled,
    const double intervalSeconds
)
{
    snapEnabled = enabled;
    snapInterval = juce::jmax(
        0.001,
        intervalSeconds
    );

    repaint();
}

void TrackRowComponent::syncControlsFromTrack()
{
    volumeSlider.setValue(
        track.getGain(),
        juce::dontSendNotification
    );
    panSlider.setValue(
        track.getPan(),
        juce::dontSendNotification
    );
    refreshMuteButton();
    refreshSoloButton();
}

void TrackRowComponent::setSelected(
    const bool shouldBeSelected
)
{
    selected = shouldBeSelected;
    repaint();
}

bool TrackRowComponent::isSelected() const noexcept
{
    return selected;
}

AudioTrack& TrackRowComponent::getTrack() noexcept
{
    return track;
}

void TrackRowComponent::changeListenerCallback(
    juce::ChangeBroadcaster*
)
{
    repaint();
}

void TrackRowComponent::refreshMuteButton()
{
    muteButton.setColour(
        juce::TextButton::buttonColourId,
        track.isMuted()
            ? juce::Colour(muteColour)
            : juce::Colour(rowColour).brighter(0.15f)
    );
}

void TrackRowComponent::refreshSoloButton()
{
    soloButton.setColour(
        juce::TextButton::buttonColourId,
        track.isSolo()
            ? juce::Colour(soloColour)
            : juce::Colour(rowColour).brighter(0.15f)
    );
}

void TrackRowComponent::updateMouseCursor(
    const juce::Point<float> position
)
{
    if (dragMode != DragMode::none)
        return;

    const auto clipBounds = getClipBounds().toFloat();

    if (clipBounds.contains(position)
        && (std::abs(position.x - clipBounds.getX())
                <= static_cast<float>(trimHandleWidth)
            || std::abs(position.x - clipBounds.getRight())
                <= static_cast<float>(trimHandleWidth)))
    {
        setMouseCursor(
            juce::MouseCursor(
                juce::MouseCursor::LeftRightResizeCursor
            )
        );
    }
    else
    {
        setMouseCursor(
            juce::MouseCursor(
                juce::MouseCursor::NormalCursor
            )
        );
    }
}

juce::Rectangle<int>
TrackRowComponent::getWaveformBounds() const
{
    return getLocalBounds()
        .withTrimmedLeft(
            TimelineRulerComponent::controlsWidth
        )
        .reduced(5, 8);
}

juce::Rectangle<int>
TrackRowComponent::getClipBounds() const
{
    const auto waveformBounds = getWaveformBounds();

    if (projectLength <= 0.0 || viewDuration <= 0.0)
        return {};

    const auto clipStart = track.getStartOffsetSeconds();
    const auto clipEnd = track.getProjectEndSeconds();

    const auto x = waveformBounds.getX()
        + static_cast<int>(
            std::round(
                (clipStart - viewStart)
                / viewDuration
                * waveformBounds.getWidth()
            )
        );

    const auto right = waveformBounds.getX()
        + static_cast<int>(
            std::round(
                (clipEnd - viewStart)
                / viewDuration
                * waveformBounds.getWidth()
            )
        );

    return juce::Rectangle<int>(
        x,
        waveformBounds.getY(),
        juce::jmax(1, right - x),
        waveformBounds.getHeight()
    );
}
