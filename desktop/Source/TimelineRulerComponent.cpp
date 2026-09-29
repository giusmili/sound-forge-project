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

    if (projectLength <= 0.0 || timeline.getWidth() <= 0)
        return;

    const auto interval = chooseTickInterval();

    for (double second = 0.0;
         second <= projectLength + 0.001;
         second += interval)
    {
        const auto ratio = second / projectLength;
        const auto x = timeline.getX()
            + static_cast<int>(
                std::round(ratio * timeline.getWidth())
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
            55,
            15,
            juce::Justification::centredLeft
        );
    }

    const auto playheadRatio = juce::jlimit(
        0.0,
        1.0,
        position / projectLength
    );

    const auto playheadX = timeline.getX()
        + static_cast<int>(
            std::round(playheadRatio * timeline.getWidth())
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

    const auto requestedPosition = ratio * projectLength;

    if (onSeek)
        onSeek(requestedPosition);
}

void TimelineRulerComponent::setProjectLength(
    const double seconds
)
{
    projectLength = juce::jmax(0.0, seconds);
    position = juce::jlimit(0.0, projectLength, position);
    repaint();
}

void TimelineRulerComponent::setPosition(
    const double seconds
)
{
    position = juce::jlimit(0.0, projectLength, seconds);
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
    if (projectLength <= 30.0)
        return 2.0;

    if (projectLength <= 120.0)
        return 5.0;

    if (projectLength <= 600.0)
        return 15.0;

    return 30.0;
}
