#include "TimelineRulerComponent.h"

#include <cmath>

namespace
{
constexpr auto backgroundColour = 0xff171b21;
constexpr auto separatorColour = 0xff363d48;
constexpr auto textColour = 0xffaeb8c5;
constexpr auto playheadColour = 0xffffb347;
}

void TimelineRulerComponent::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour(backgroundColour));

    const auto bounds = getLocalBounds();
    const auto timeline = bounds.withTrimmedLeft(controlsWidth);

    graphics.setColour(juce::Colour(separatorColour));
    graphics.drawVerticalLine(
        controlsWidth,
        0.0f,
        static_cast<float>(getHeight())
    );

    graphics.setColour(juce::Colour(textColour));
    graphics.setFont(11.0f);
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

    const auto interval = chooseTickInterval();
    const auto viewEnd = viewStart + viewDuration;

    auto firstTick =
        std::floor(viewStart / interval) * interval;

    if (firstTick < 0.0)
        firstTick = 0.0;

    for (double second = firstTick;
         second <= viewEnd + 0.001;
         second += interval)
    {
        if (second < viewStart - 0.001)
            continue;

        const auto ratio =
            (second - viewStart) / viewDuration;

        const auto x = timeline.getX()
            + static_cast<int>(
                std::round(
                    ratio * timeline.getWidth()
                )
            );

        graphics.setColour(juce::Colour(separatorColour));
        graphics.drawVerticalLine(
            x,
            16.0f,
            static_cast<float>(getHeight())
        );

        graphics.setColour(juce::Colour(textColour));
        graphics.drawText(
            formatTick(second),
            x + 4,
            1,
            58,
            15,
            juce::Justification::centredLeft
        );
    }

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
    if (projectLength <= 0.0
        || viewDuration <= 0.0
        || event.position.x < static_cast<float>(controlsWidth))
    {
        return;
    }

    const auto usableWidth = getWidth() - controlsWidth;

    if (usableWidth <= 0)
        return;

    const auto ratio = juce::jlimit(
        0.0,
        1.0,
        static_cast<double>(
            event.position.x - static_cast<float>(controlsWidth)
        )
            / static_cast<double>(usableWidth)
    );

    const auto requestedPosition = juce::jlimit(
        0.0,
        projectLength,
        viewStart + ratio * viewDuration
    );

    if (onSeek)
        onSeek(requestedPosition);
}

void TimelineRulerComponent::setView(
    const double projectLengthSeconds,
    const double viewStartSeconds,
    const double viewDurationSeconds,
    const double positionSeconds
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

    return juce::String(minutes).paddedLeft('0', 2)
        + ":"
        + juce::String(remainingSeconds).paddedLeft('0', 2);
}

double TimelineRulerComponent::chooseTickInterval() const
{
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
