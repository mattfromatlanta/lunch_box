#include "BankRowComponent.h"

namespace
{
    const juce::Colour rowBgColour    { 0xff1a1f2e };
    const juce::Colour labelColour    { 0xff8899aa };
    const juce::Colour countColour    { 0xff4a5a6a };
}

BankRowComponent::BankRowComponent(char letter)
    : bankLetter(letter)
{
    // Bank label (e.g. "A")
    bankLabel.setText(juce::String::charToString(letter).toUpperCase(),
                      juce::dontSendNotification);
    bankLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    bankLabel.setColour(juce::Label::textColourId, labelColour);
    bankLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bankLabel);

    for (int i = 0; i < ChompiNamer::SLOTS_PER_BANK; ++i)
    {
        auto* slot = slots.add(new BankSlotComponent(i + 1));

        slot->onSampleChanged = [this](BankSlotComponent*)
        {
            // Update the bank label to show count
            int filled = getFilledCount();
            bankLabel.setText(juce::String::charToString(bankLetter).toUpperCase()
                              + "\n" + juce::String(filled) + "/" +
                              juce::String(ChompiNamer::SLOTS_PER_BANK),
                              juce::dontSendNotification);
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

    bankLabel.setBounds(area.removeFromLeft(LABEL_WIDTH));

    int slotW = area.getWidth() / ChompiNamer::SLOTS_PER_BANK;
    for (auto* slot : slots)
        slot->setBounds(area.removeFromLeft(slotW));
}
