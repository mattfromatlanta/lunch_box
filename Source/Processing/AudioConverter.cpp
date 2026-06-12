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

    // Check if bit depth or sample rate differs from target
    bool bitDepthDiffers = (reader->bitsPerSample != TARGET_BIT_DEPTH);
    bool sampleRateDiffers = (reader->sampleRate != TARGET_SAMPLE_RATE);

    return bitDepthDiffers || sampleRateDiffers;
}

AudioConverter::ConversionResult AudioConverter::convertFileWithName(
    const juce::File& sourceFile,
    const juce::File& outputFolder,
    const juce::String& outputFileName,
    juce::AudioFormatManager& formatManager)
{
    ConversionResult result;
    result.success = false;
    result.skipped = false;
    result.message = "";

    // Log conversion attempt with format name
    juce::String formatName = FileSystemHelper::getAudioFormatName(sourceFile);
    logger.logLine("Converting: " + sourceFile.getFileName() + " (" + formatName + ") → " + outputFileName);

    // Read source file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(sourceFile));

    if (reader == nullptr)
    {
        result.message = "Error: Unable to read source file";
        logger.logLine("   " + result.message);
        return result;
    }

    // Extract source properties
    int numChannels = static_cast<int>(reader->numChannels);
    int bitsPerSample = static_cast<int>(reader->bitsPerSample);
    double sampleRate = reader->sampleRate;
    double durationSeconds = reader->lengthInSamples / sampleRate;

    // Log source properties
    logger.logLine("   Original: " +
                   juce::String(numChannels) + " channel(s), " +
                   juce::String(bitsPerSample) + "-bit, " +
                   juce::String((int)sampleRate) + " Hz, " +
                   juce::String(durationSeconds, 1) + "s");

    // Check channel count - skip if more than 2 channels
    if (numChannels > MAX_CHANNELS)
    {
        result.skipped = true;
        result.message = "Skipped: Multi-channel audio (" + juce::String(numChannels) + " channels) - not supported";
        logger.logLine("   " + result.message);
        return result;
    }

    // Check duration limit
    if (durationSeconds > MAX_DURATION_SECONDS)
    {
        result.skipped = true;
        result.message = "Skipped: Duration " + juce::String(durationSeconds, 1) +
                         "s exceeds 2-minute limit";
        logger.logLine("   " + result.message);
        return result;
    }

    // Generate output path with custom filename
    juce::File outputFile = outputFolder.getChildFile(outputFileName);

    // Log target properties
    logger.logLine("   Target:   " +
                   juce::String(numChannels) + " channel(s), " +
                   juce::String(TARGET_BIT_DEPTH) + "-bit, " +
                   juce::String((int)TARGET_SAMPLE_RATE) + " Hz");
    logger.logLine("   Output:   " + outputFile.getRelativePathFrom(juce::File::getCurrentWorkingDirectory()));

    // Perform the conversion
    result = performConversion(outputFile, reader.get());

    return result;
}


AudioConverter::ConversionResult AudioConverter::performConversion(
    const juce::File& outputFile,
    juce::AudioFormatReader* reader)
{
    ConversionResult result;
    result.success = false;
    result.skipped = false;
    result.message = "";

    // Create output directory if needed
    juce::File outputDir = outputFile.getParentDirectory();
    outputDir.createDirectory();

    // Delete existing output file if it exists
    if (outputFile.exists())
    {
        outputFile.deleteFile();
    }

    // Create output file stream
    std::unique_ptr<juce::FileOutputStream> outputStream(outputFile.createOutputStream());

    if (outputStream == nullptr || outputStream->failedToOpen())
    {
        result.message = "Error: Could not create output file";
        logger.logLine("   " + result.message);
        return result;
    }

    // Create WAV format writer with target specifications
    juce::WavAudioFormat wavFormat;

    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outputStream.get(),
                                  TARGET_SAMPLE_RATE,
                                  reader->numChannels,
                                  TARGET_BIT_DEPTH,
                                  {},  // metadata
                                  0)); // quality option

    if (writer == nullptr)
    {
        result.message = "Error: Could not create audio writer";
        logger.logLine("   " + result.message);
        return result;
    }

    // Release ownership of the stream to the writer
    outputStream.release();

    // Check if sample rate conversion is needed
    bool needsSampleRateConversion = (reader->sampleRate != TARGET_SAMPLE_RATE);

    if (needsSampleRateConversion)
    {
        // Sample rate conversion using ResamplingAudioSource
        // We need to process in blocks

        juce::AudioFormatReaderSource readerSource(reader, false);
        juce::ResamplingAudioSource resamplingSource(&readerSource, false, reader->numChannels);

        // Calculate resampling ratio
        double ratio = reader->sampleRate / TARGET_SAMPLE_RATE;
        resamplingSource.setResamplingRatio(ratio);

        // Prepare the resampling source
        resamplingSource.prepareToPlay(512, TARGET_SAMPLE_RATE);

        // Calculate total number of samples in output
        juce::int64 totalOutputSamples = static_cast<juce::int64>(reader->lengthInSamples / ratio);
        juce::int64 samplesWritten = 0;

        const int bufferSize = 4096;
        juce::AudioBuffer<float> buffer(reader->numChannels, bufferSize);

        // Process audio in blocks
        while (samplesWritten < totalOutputSamples)
        {
            int samplesToRead = juce::jmin(bufferSize, (int)(totalOutputSamples - samplesWritten));

            // Get audio block from resampling source
            juce::AudioSourceChannelInfo channelInfo;
            channelInfo.buffer = &buffer;
            channelInfo.startSample = 0;
            channelInfo.numSamples = samplesToRead;

            resamplingSource.getNextAudioBlock(channelInfo);

            // Write to output file
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

    // Flush and close writer
    writer.reset();

    result.success = true;
    result.message = "Status: ✓ Converted successfully";
    logger.logLine("   " + result.message);

    return result;
}

bool AudioConverter::generateOptimizedSample(const juce::File& baseFile,
                                             const juce::File& optimizedFile,
                                             juce::AudioFormatManager& formatManager)
{
    logger.logLine("   Generating optimized: " + optimizedFile.getFileName());

    // Read the base WAV file (already 16-bit 48kHz)
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(baseFile));

    if (reader == nullptr)
    {
        logger.logLine("   Error: Could not read base file for optimization");
        return false;
    }

    int numChannels = static_cast<int>(reader->numChannels);
    int sourceSamples = static_cast<int>(reader->lengthInSamples);
    int optimizedSamples = sourceSamples / 2;

    if (optimizedSamples <= 0)
    {
        logger.logLine("   Error: Base sample too short for optimization");
        return false;
    }

    // Read all source samples into buffer
    juce::AudioBuffer<float> sourceBuffer(numChannels, sourceSamples);
    reader->read(&sourceBuffer, 0, sourceSamples, 0, true, true);
    reader.reset();

    // Decimate by 2: take every other sample (double speed = one octave up)
    juce::AudioBuffer<float> optimizedBuffer(numChannels, optimizedSamples);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int i = 0; i < optimizedSamples; ++i)
        {
            optimizedBuffer.setSample(ch, i, sourceBuffer.getSample(ch, i * 2));
        }
    }

    // Write optimized file
    if (optimizedFile.exists())
        optimizedFile.deleteFile();

    std::unique_ptr<juce::FileOutputStream> outputStream(optimizedFile.createOutputStream());
    if (outputStream == nullptr || outputStream->failedToOpen())
    {
        logger.logLine("   Error: Could not create optimized output file");
        return false;
    }

    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outputStream.get(),
                                  TARGET_SAMPLE_RATE,
                                  numChannels,
                                  TARGET_BIT_DEPTH,
                                  {},
                                  0));

    if (writer == nullptr)
    {
        logger.logLine("   Error: Could not create optimized file writer");
        return false;
    }

    outputStream.release(); // Writer takes ownership

    if (!writer->writeFromAudioSampleBuffer(optimizedBuffer, 0, optimizedSamples))
    {
        logger.logLine("   Error: Failed to write optimized audio data");
        return false;
    }

    writer.reset();
    logger.logLine("   Status: ✓ Optimized version generated");
    return true;
}
