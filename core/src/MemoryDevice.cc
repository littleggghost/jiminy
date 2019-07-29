#include <iostream>

#include "jiminy/core/MemoryDevice.h"


namespace jiminy
{
    MemoryDevice::MemoryDevice(int64_t size) :
    buffer_(static_cast<size_t>(size)),
    currentPos_(0)
    {
        supportedModes_ = OpenMode::READ_ONLY | OpenMode::WRITE_ONLY | OpenMode::READ_WRITE | OpenMode::NON_BLOCKING | OpenMode::APPEND;
    }


    MemoryDevice::MemoryDevice(MemoryDevice const& other) :
    buffer_(other.buffer_),
    currentPos_(other.currentPos_)
    {
        supportedModes_ = other.supportedModes_;
        modes_ = other.modes_;
    }


    MemoryDevice::MemoryDevice(MemoryDevice&& other) :
    buffer_(std::move(other.buffer_)),
    currentPos_(other.currentPos_)
    {
        supportedModes_ = other.supportedModes_;
        modes_ = other.modes_;
        other.close();
    }

    MemoryDevice::MemoryDevice(std::vector<uint8_t>&& initBuffer) :
    buffer_(std::move(initBuffer)),
    currentPos_(0)
    {
        supportedModes_ = OpenMode::READ_ONLY | OpenMode::WRITE_ONLY | OpenMode::READ_WRITE | OpenMode::NON_BLOCKING | OpenMode::APPEND;
    }

    MemoryDevice::~MemoryDevice(void)
    {
        close();
    }

    MemoryDevice& MemoryDevice::operator=(MemoryDevice const& other)
    {
        buffer_ = other.buffer_;
        currentPos_ = other.currentPos_;
        modes_ = other.modes_;

        return *this;
    }

    MemoryDevice& MemoryDevice::operator=(MemoryDevice&& other)
    {
        buffer_ = std::move(other.buffer_);
        currentPos_ = other.currentPos_;
        modes_ = other.modes_;
        other.close();

        return *this;
    }

    result_t MemoryDevice::seek(int64_t pos)
    {
        if ((pos < 0) or (pos >= (int64_t) buffer_.size()))
        {
            std::cout << "Error - MemoryDevice::seek - The requested position '" << pos << "' is out of scope." << std::endl;
            return result_t::ERROR_GENERIC;
        }

        currentPos_ = pos;
        return result_t::SUCCESS;
    }

    int64_t MemoryDevice::readData(void* data, int64_t dataSize)
    {
        // Read no more than availables bytes.
        int64_t toRead = bytesAvailable();
        if (dataSize < toRead)
        {
            toRead = dataSize;
        }

        std::memcpy(data, buffer_.data() + currentPos_, static_cast<std::size_t>(toRead));
        currentPos_ += toRead;
        return toRead;
    }

    int64_t MemoryDevice::writeData(void const* data, int64_t dataSize)
    {
        // Write no more than availables bytes.
        int64_t toWrite = bytesAvailable();
        if (dataSize < toWrite)
        {
            toWrite = dataSize;
        }

        std::memcpy(buffer_.data() + currentPos_, data, static_cast<std::size_t>(toWrite));
        currentPos_ += toWrite;

        return toWrite;
    }

    result_t MemoryDevice::setBlockingMode(bool_t)
    {
        // Since this is a memory device, it can't block when doing its job,
        // thus we don't care about blocking mode and answer 'OK no problem'.
        return result_t::SUCCESS;
    }

    result_t MemoryDevice::doOpen(enum OpenMode modes)
    {
        if (not (modes & OpenMode::APPEND))
        {
            currentPos_ = 0;
        }

        return result_t::SUCCESS;
    }

    void MemoryDevice::doClose(void)
    {
        // Nothing to do.
    }

    void MemoryDevice::resize(int64_t size)
    {
        buffer_.resize(size);
    }
}
