#include "AudioConverter.h"

AudioConverter::AudioConverter(Logger& logger)
    : logger(logger)
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

AudioConverter::ConversionResult AudioConverter::convertFile(
    const juce::File& sourceFile,
    const juce::File& outputFolder,
    juce::AudioFormatManager& formatManager)
{
    ConversionResult result;
    result.success = false;
    result.skipped = false;
    result.message = "";

    // Log conversion attempt
    logger.logLine("Converting: " + sourceFile.getFileName());

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

    // Log source properties
    logger.logLine("   Original: " +
                   juce::String(numChannels) + " channel(s), " +
                   juce::String(bitsPerSample) + "-bit, " +
                   juce::String((int)sampleRate) + " Hz");

    // Check channel count - skip if more than 2 channels
    if (numChannels > MAX_CHANNELS)
    {
        result.skipped = true;
        result.message = "Skipped: Multi-channel audio (" + juce::String(numChannels) + " channels) - not supported";
        logger.logLine("   " + result.message);
        return result;
    }

    // Generate output path
    juce::File outputFile = outputFolder.getChildFile(sourceFile.getFileName());

    // Log target properties
    logger.logLine("   Target:   " +
                   juce::String(numChannels) + " channel(s), " +
                   juce::String(TARGET_BIT_DEPTH) + "-bit, " +
                   juce::String((int)TARGET_SAMPLE_RATE) + " Hz");
    logger.logLine("   Output:   " + outputFile.getRelativePathFrom(juce::File::getCurrentWorkingDirectory()));

    // Perform the conversion
    result = performConversion(sourceFile, outputFile, reader.get(), formatManager);

    return result;
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

    // Log conversion attempt
    logger.logLine("Converting: " + sourceFile.getFileName() + " → " + outputFileName);

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

    // Log source properties
    logger.logLine("   Original: " +
                   juce::String(numChannels) + " channel(s), " +
                   juce::String(bitsPerSample) + "-bit, " +
                   juce::String((int)sampleRate) + " Hz");

    // Check channel count - skip if more than 2 channels
    if (numChannels > MAX_CHANNELS)
    {
        result.skipped = true;
        result.message = "Skipped: Multi-channel audio (" + juce::String(numChannels) + " channels) - not supported";
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
    result = performConversion(sourceFile, outputFile, reader.get(), formatManager);

    return result;
}

void AudioConverter::convertFiles(
    const juce::Array<juce::File>& files,
    const juce::File& sourceFolder,
    const juce::File& outputFolder,
    juce::AudioFormatManager& formatManager)
{
    logger.logLine("");
    logger.logLine("=== Starting Batch Conversion ===");
    logger.logLine("Output directory: " + outputFolder.getFullPathName());
    logger.logLine("");

    // Ensure output directory exists
    if (!FileSystemHelper::ensureDirectoryExists(outputFolder, logger))
    {
        logger.logLine("Error: Could not create output directory");
        return;
    }

    // Track conversion statistics
    int totalFiles = files.size();
    int convertedCount = 0;
    int skippedCount = 0;
    int errorCount = 0;

    // Convert each file
    for (int i = 0; i < files.size(); ++i)
    {
        const juce::File& file = files[i];

        ConversionResult result = convertFile(file, outputFolder, formatManager);

        if (result.success)
            convertedCount++;
        else if (result.skipped)
            skippedCount++;
        else
            errorCount++;

        logger.logLine("");
    }

    // Display summary
    logger.logLine("=== Conversion Complete ===");
    logger.logLine("Total files: " + juce::String(totalFiles));
    logger.logLine("Converted: " + juce::String(convertedCount));
    logger.logLine("Skipped: " + juce::String(skippedCount));
    logger.logLine("Errors: " + juce::String(errorCount));
}

AudioConverter::ConversionResult AudioConverter::performConversion(
    const juce::File& sourceFile,
    const juce::File& outputFile,
    juce::AudioFormatReader* reader,
    juce::AudioFormatManager& formatManager)
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
