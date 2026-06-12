// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioConverter.h"

AudioConverter::AudioConverter(Logger& loggerToUse)
    : logger(loggerToUse)
{
}

bool AudioConverter::needsConversion(const juce::AudioFormatReader* reader) const
{
    if (reader == nullptr)
        return false;

    bool bitDepthDiffers = (reader->bitsPerSample != TARGET_BIT_DEPTH);
    bool sampleRateDiffers = ! juce::exactlyEqual(reader->sampleRate, TARGET_SAMPLE_RATE);

    return bitDepthDiffers || sampleRateDiffers;
}

AudioConverter::ConversionResult AudioConverter::convertFileWithName(
    const juce::File& sourceFile,
    const juce::File& outputFolder,
    const juce::String& outputFileName,
    juce::AudioFormatManager& formatManager)
{
    ConversionResult result;

    juce::String formatName = FileSystemHelper::getAudioFormatName(sourceFile);
    logger.logLine("Converting: " + sourceFile.getFileName() + " (" + formatName + ") → " + outputFileName);

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sourceFile));

    if (reader == nullptr)
    {
        result.message = "Error: Unable to read source file";
        logger.logLine("   " + result.message);
        return result;
    }

    int numChannels = static_cast<int>(reader->numChannels);
    int bitsPerSample = static_cast<int>(reader->bitsPerSample);
    double sampleRate = reader->sampleRate;
    double durationSeconds = reader->lengthInSamples / sampleRate;

    logger.logLine("   Original: " +
                   juce::String(numChannels) + " channel(s), " +
                   juce::String(bitsPerSample) + "-bit, " +
                   juce::String(static_cast<int>(sampleRate)) + " Hz, " +
                   juce::String(durationSeconds, 1) + "s");

    if (numChannels > MAX_CHANNELS)
    {
        result.skipped = true;
        result.message = "Skipped: Multi-channel audio (" + juce::String(numChannels) + " channels) - not supported";
        logger.logLine("   " + result.message);
        return result;
    }

    if (durationSeconds > MAX_DURATION_SECONDS)
    {
        result.skipped = true;
        result.message = "Skipped: Duration " + juce::String(durationSeconds, 1) +
                         "s exceeds 2-minute limit";
        logger.logLine("   " + result.message);
        return result;
    }

    juce::File outputFile = outputFolder.getChildFile(outputFileName);

    logger.logLine("   Target:   " +
                   juce::String(numChannels) + " channel(s), " +
                   juce::String(TARGET_BIT_DEPTH) + "-bit, " +
                   juce::String(static_cast<int>(TARGET_SAMPLE_RATE)) + " Hz");
    logger.logLine("   Output:   " + outputFile.getFullPathName());

    result = performConversion(outputFile, reader.get());

    return result;
}


AudioConverter::ConversionResult AudioConverter::performConversion(
    const juce::File& outputFile,
    juce::AudioFormatReader* reader)
{
    ConversionResult result;

    juce::File outputDir = outputFile.getParentDirectory();
    outputDir.createDirectory();

    if (outputFile.exists())
    {
        outputFile.deleteFile();
    }

    std::unique_ptr<juce::FileOutputStream> fileStream(outputFile.createOutputStream());

    if (fileStream == nullptr || fileStream->failedToOpen())
    {
        result.message = "Error: Could not create output file";
        logger.logLine("   " + result.message);
        return result;
    }

    const int numChannels = static_cast<int>(reader->numChannels);

    // On success the writer takes ownership of the stream; on failure the
    // unique_ptr still owns it and cleans up.
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::OutputStream> outputStream = std::move(fileStream);
    auto writer = wavFormat.createWriterFor(outputStream,
                                            juce::AudioFormatWriterOptions{}
                                                .withSampleRate(TARGET_SAMPLE_RATE)
                                                .withNumChannels(numChannels)
                                                .withBitsPerSample(TARGET_BIT_DEPTH));

    if (writer == nullptr)
    {
        result.message = "Error: Could not create audio writer";
        logger.logLine("   " + result.message);
        return result;
    }

    bool needsSampleRateConversion = ! juce::exactlyEqual(reader->sampleRate, TARGET_SAMPLE_RATE);

    if (needsSampleRateConversion)
    {
        juce::AudioFormatReaderSource readerSource(reader, false);
        juce::ResamplingAudioSource resamplingSource(&readerSource, false, numChannels);

        double ratio = reader->sampleRate / TARGET_SAMPLE_RATE;
        resamplingSource.setResamplingRatio(ratio);

        resamplingSource.prepareToPlay(512, TARGET_SAMPLE_RATE);

        juce::int64 totalOutputSamples = static_cast<juce::int64>(reader->lengthInSamples / ratio);
        juce::int64 samplesWritten = 0;

        const int bufferSize = 4096;
        juce::AudioBuffer<float> buffer(numChannels, bufferSize);

        while (samplesWritten < totalOutputSamples)
        {
            int samplesToRead = juce::jmin(bufferSize, static_cast<int>(totalOutputSamples - samplesWritten));

            juce::AudioSourceChannelInfo channelInfo;
            channelInfo.buffer = &buffer;
            channelInfo.startSample = 0;
            channelInfo.numSamples = samplesToRead;

            resamplingSource.getNextAudioBlock(channelInfo);

            if (!writer->writeFromAudioSampleBuffer(buffer, 0, samplesToRead))
            {
                result.message = "Error: Failed to write audio data";
                logger.logLine("   " + result.message);
                resamplingSource.releaseResources();
                return result;
            }

            samplesWritten += samplesToRead;
        }

        resamplingSource.releaseResources();
    }
    else
    {
        // No sample rate conversion needed - direct copy with bit depth conversion
        if (!writer->writeFromAudioReader(*reader, 0, reader->lengthInSamples))
        {
            result.message = "Error: Failed to write audio data";
            logger.logLine("   " + result.message);
            return result;
        }
    }

    writer.reset();

    result.success = true;
    result.message = "Status: ✓ Converted successfully";
    logger.logLine("   " + result.message);

    return result;
}
