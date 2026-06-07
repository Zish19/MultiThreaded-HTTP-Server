#include "FileCache.h"
#include "MimeTypes.h"
#include "Metrics.h"
#include "Logger.h"
#include <fstream>
#include <iostream>

FileCache::FileCache(std::size_t maxSizeBytes) : m_maxSizeBytes(maxSizeBytes) {}

std::optional<CachedFile> FileCache::getFile(const std::filesystem::path& path) {
    std::string pathStr = path.string();

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || !std::filesystem::is_regular_file(path, ec)) {
        return std::nullopt;
    }

    auto currentWriteTime = std::filesystem::last_write_time(path, ec);
    if (ec) return std::nullopt;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_cache.find(pathStr);
        if (it != m_cache.end()) {
            if (it->second.file.lastWriteTime == currentWriteTime) {
                // Cache hit: Move to front of LRU
                m_lruList.splice(m_lruList.begin(), m_lruList, it->second.lruIt);
                Metrics::getInstance().recordCacheHit();
                return it->second.file;
            } else {
                // Stale entry: remove it
                m_currentSizeBytes -= it->second.file.size;
                m_lruList.erase(it->second.lruIt);
                m_cache.erase(it);
            }
        }
    }

    // Cache miss or stale
    Metrics::getInstance().recordCacheMiss();

    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return std::nullopt;

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    CachedFile cf;
    cf.size = static_cast<std::size_t>(size);
    cf.lastWriteTime = currentWriteTime;
    cf.mimeType = MimeTypes::getType(path.extension().string());

    if (size > 0) {
        cf.content.resize(cf.size);
        if (!file.read(cf.content.data(), size)) return std::nullopt;
    }

    if (cf.size > m_maxSizeBytes) {
        // File is larger than the entire cache, serve without caching
        return cf;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Double-check if another thread just inserted it
        if (m_cache.find(pathStr) == m_cache.end()) {
            evictIfNeeded(cf.size);
            m_lruList.push_front(pathStr);
            m_cache[pathStr] = {cf, m_lruList.begin()};
            m_currentSizeBytes += cf.size;
        }
    }

    return cf;
}

void FileCache::evictIfNeeded(std::size_t requiredSize) {
    while (m_currentSizeBytes + requiredSize > m_maxSizeBytes && !m_lruList.empty()) {
        std::string oldest = m_lruList.back();
        m_lruList.pop_back();
        
        auto it = m_cache.find(oldest);
        if (it != m_cache.end()) {
            m_currentSizeBytes -= it->second.file.size;
            m_cache.erase(it);
            Metrics::getInstance().recordCacheEviction();
        }
    }
}

void FileCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
    m_lruList.clear();
    m_currentSizeBytes = 0;
}

std::size_t FileCache::getCurrentSizeBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSizeBytes;
}
