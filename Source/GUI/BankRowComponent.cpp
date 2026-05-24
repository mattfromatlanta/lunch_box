// SPDX-License-Identifier: AGPL-3.0-or-later
#include "BankRowComponent.h"
#include "UIColours.h"

namespace
{
    const juce::Colour rowBgColour = ChompiColours::DARK_GREY;
}

BankRowComponent::BankRowComponent(char letter)
    : bankLetter(letter)
{
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        auto* slot = slots.add(new BankSlotComponent(letter, i + 1));

        slot->onSampleChanged = [this](BankSlotComponent*)
        {
            if (onAssignmentsChanged) onAssignmentsChanged();
        };

        slot->onSlotClicked = [this](BankSlotComponent* s)
        {
            if (onSlotClicked) onSlotClicked(s->getSample());
        };

        slot->getStartDirectory = [this]() -> juce::File
        {
            return (getStartDirectory) ? getStartDirectory() : juce::File{};
        };

        slot->onFolderBrowsed = [this](juce::File dir)
        {
            if (onFolderBrowsed) onFolderBrowsed(dir);
        };

        addAndMakeVisible(slot);
    }
}

void BankRowComponent::setSlot(int index, const juce::File& file)
{
    if (index >= 0 && index < ChompiNamer::SLOTS_PER_BANK)
        slots[index]->setSample(file);
}

void BankRowComponent::clearSlot(int index)
{
    if (index >= 0 && index < ChompiNamer::SLOTS_PER_BANK)
        slots[index]->clearSample();
}

void BankRowComponent::clearAllSlots()
{
    for (auto* slot : slots)
        slot->clearSample();
}

juce::File BankRowComponent::getSlot(int index) const
{
    if (index >= 0 && index < ChompiNamer::SLOTS_PER_BANK)
        return slots[index]->getSample();
    return juce::File{};
}

BankSlotComponent* BankRowComponent::getSlotComponent(int index)
{
    if (index >= 0 && index < ChompiNamer::SLOTS_PER_BANK) return slots[index];
    return nullptr;
}

int BankRowComponent::getFilledCount() const
{
    int count = 0;
    for (const auto* slot : slots)
        if (slot->hasSample()) ++count;
    return count;
}

void BankRowComponent::sortSlotsAlphabetically()
{
    // Collect filled files
    juce::Array<juce::File> filled;
    for (const auto* slot : slots)
        if (slot->hasSample())
            filled.add(slot->getSample());

    if (filled.isEmpty()) return;

    struct NameComp
    {
        int compareElements(const juce::File& a, const juce::File& b) const
        {
            return a.getFileName().compareNatural(b.getFileName());
        }
    } comp;
    filled.sort(comp);

    // Put sorted files back in order, clear remaining slots
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        if (i < filled.size())
            slots[i]->setSample(filled[i]);
        else
            slots[i]->clearSample();
    }
}

void BankRowComponent::autoFillFromFiles(const juce::Array<juce::File>& files, int startIndex)
{
    int fileIdx = startIndex;
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK && fileIdx < files.size(); ++i)
    {
        if (!slots[i]->hasSample())
        {
            slots[i]->setSample(files[fileIdx]);
            ++fileIdx;
        }
    }
}

juce::Array<BankFolderParser::BankAssignment> BankRowComponent::getAssignments() const
{
    juce::Array<BankFolderParser::BankAssignment> assignments;
    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        if (slots[i]->hasSample())
        {
            BankFolderParser::BankAssignment a;
            a.sourceFile     = slots[i]->getSample();
            a.bankLetter     = bankLetter;
            a.slotNumber     = i + 1;
            a.fromBankFolder = false;
            assignments.add(a);
        }
    }
    return assignments;
}

void BankRowComponent::paint(juce::Graphics& g)
{
    g.fillAll(rowBgColour);
}

void BankRowComponent::resized()
{
    auto area = getLocalBounds();
    constexpr int half = ChompiNamer::SLOTS_PER_BANK / 2;  // 7
    const float cellW = (float)area.getWidth()  / (float)half;
    const float cellH = (float)area.getHeight() / 2.0f;
    const int   midY  = juce::roundToInt((float)area.getY() + cellH);

    float x = (float)area.getX();
    for (int i = 0; i < half; ++i)
    {
        const int l = juce::roundToInt(x), r = juce::roundToInt(x + cellW);
        slots[i]->setBounds(l, area.getY(), r - l, midY - area.getY());
        x += cellW;
    }
    x = (float)area.getX();
    for (int i = half; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        const int l = juce::roundToInt(x), r = juce::roundToInt(x + cellW);
        slots[i]->setBounds(l, midY, r - l, area.getBottom() - midY);
        x += cellW;
    }
}
