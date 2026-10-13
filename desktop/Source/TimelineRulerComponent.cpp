#include "TimelineRulerComponent.h"

#include <cmath>

namespace
{
constexpr auto backgroundColour = 0xff171b21;
constexpr auto separatorColour = 0xff363d48;
constexpr auto gridColour = 0xff29313b;
constexpr auto textColour = 0xffaeb8c5;
constexpr auto playheadColour = 0xffffb347;
}

void TimelineRulerComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(backgroundColour));

    const auto bounds = getLocalBounds();
    const auto timeline = getTimelineBounds();

    graphics.setColour(juce::Colour(separatorColour));
    graphics.drawVerticalLine(
        controlsWidth,
        0.0f,
        static_cast<float>(getHeight())
    );

    graphics.setColour(juce::Colour(separatorColour));
    graphics.drawHorizontalLine(
        getHeight() - 1,
        0.0f,
        static_cast<float>(getWidth())
    );

    graphics.setColour(juce::Colour(textColour));
    graphics.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    graphics.drawText(
        "TIMELINE",
        bounds.withWidth(controlsWidth).reduced(10, 0),
        juce::Justification::centredLeft
    );

    if (projectLength <= 0.0
        || viewDuration <= 0.0
        || timeline.getWidth() <= 0)
    {
        return;
    }

    const auto viewEnd = viewStart + viewDuration;

    if (gridInterval > 0.0)
    {
        auto firstGrid =
            std::floor(viewStart / gridInterval) * gridInterval;

        if (firstGrid < viewStart - 0.001)
            firstGrid += gridInterval;

        graphics.setColour(juce::Colour(gridColour));

        for (double beat = firstGrid;
             beat <= viewEnd + 0.001;
             beat += gridInterval)
        {
            const auto ratio =
                (beat - viewStart) / viewDuration;

            const auto x = timeline.getX()
                + static_cast<int>(
                    std::round(
                        ratio * timeline.getWidth()
                    )
                );

            graphics.drawVerticalLine(
                x,
                20.0f,
                static_cast<float>(getHeight())
            );
        }
    }

    const auto interval = chooseTickInterval();
    const auto minorInterval = interval / 5.0;
    const auto height = static_cast<float>(getHeight());

    const auto toX = [&](const double seconds)
    {
        return timeline.getX()
            + static_cast<int>(
                std::round(
                    (seconds - viewStart) / viewDuration
                    * timeline.getWidth()
                )
            );
    };

    graphics.saveState();
    graphics.reduceClipRegion(timeline.withTop(0));

    graphics.setColour(juce::Colour(separatorColour));

    for (double second = std::floor(viewStart / minorInterval) * minorInterval;
         second <= viewEnd + 0.001;
         second += minorInterval)
    {
        if (second < viewStart - 0.001 || second > projectLength + 0.001)
            continue;

        graphics.drawVerticalLine(toX(second), height - 7.0f, height - 1.0f);
    }

    for (double second = std::floor(viewStart / interval) * interval;
         second <= viewEnd + 0.001;
         second += interval)
    {
        if (second < viewStart - 0.001 || second > projectLength + 0.001)
            continue;

        const auto x = toX(second);

        graphics.setColour(juce::Colour(separatorColour));
        graphics.drawVerticalLine(x, 14.0f, height);

        graphics.setColour(juce::Colour(textColour));
        graphics.setFont(12.0f);
        graphics.drawText(
            formatTick(second),
            x + 4,
            1,
            64,
            16,
            juce::Justification::centredLeft
        );
    }

    graphics.restoreState();

    if (position < viewStart || position > viewEnd)
        return;

    const auto playheadRatio =
        (position - viewStart) / viewDuration;

    const auto playheadX = timeline.getX()
        + static_cast<int>(
            std::round(
                playheadRatio * timeline.getWidth()
            )
        );

    graphics.setColour(juce::Colour(playheadColour));
    graphics.fillRect(playheadX - 1, 0, 2, getHeight());

    juce::Path marker;
    marker.addTriangle(
        static_cast<float>(playheadX - 5),
        0.0f,
        static_cast<float>(playheadX + 5),
        0.0f,
        static_cast<float>(playheadX),
        8.0f
    );
    graphics.fillPath(marker);
}

void TimelineRulerComponent::mouseDown(
    const juce::MouseEvent& event
)
{
    const auto timeline = getTimelineBounds();

    if (projectLength <= 0.0
        || viewDuration <= 0.0
        || timeline.getWidth() <= 0
        || event.position.x < static_cast<float>(timeline.getX()))
    {
        return;
    }

    const auto ratio = juce::jlimit(
        0.0,
        1.0,
        static_cast<double>(
            event.position.x - static_cast<float>(timeline.getX())
        )
            / static_cast<double>(timeline.getWidth())
    );

    const auto requestedPosition = juce::jlimit(
        0.0,
        projectLength,
        viewStart + ratio * viewDuration
    );

    if (onSeek)
        onSeek(requestedPosition);
}

void TimelineRulerComponent::mouseDrag(
    const juce::MouseEvent& event
)
{
    mouseDown(event);
}

juce::Rectangle<int> TimelineRulerComponent::getTimelineBounds() const
{
    return getLocalBounds()
        .withTrimmedLeft(controlsWidth)
        .reduced(timelineInset, 0);
}

void TimelineRulerComponent::setView(
    const double projectLengthSeconds,
    const double viewStartSeconds,
    const double viewDurationSeconds,
    const double positionSeconds,
    const double gridIntervalSeconds
)
{
    projectLength = juce::jmax(
        0.0,
        projectLengthSeconds
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

    position = juce::jlimit(
        0.0,
        projectLength,
        positionSeconds
    );

    gridInterval = juce::jmax(
        0.0,
        gridIntervalSeconds
    );

    repaint();
}

juce::String TimelineRulerComponent::formatTick(
    const double seconds
)
{
    const auto totalSeconds =
        static_cast<int>(std::floor(seconds + 0.001));
    const auto minutes = totalSeconds / 60;
    const auto remainingSeconds = totalSeconds % 60;

    const auto hasHalf = std::abs(seconds - std::floor(seconds)) > 0.25;

    return juce::String(minutes).paddedLeft('0', 2)
        + ":"
        + juce::String(remainingSeconds).paddedLeft('0', 2)
        + (hasHalf ? ".5" : "");
}

double TimelineRulerComponent::chooseTickInterval() const
{
    if (viewDuration <= 3.0)
        return 0.5;

    if (viewDuration <= 10.0)
        return 1.0;

    if (viewDuration <= 30.0)
        return 2.0;

    if (viewDuration <= 120.0)
        return 5.0;

    if (viewDuration <= 600.0)
        return 15.0;

    return 30.0;
}
