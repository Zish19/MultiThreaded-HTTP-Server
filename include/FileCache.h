#pragma once

#include <string>
#include <filesystem>
#include <optional>
#include <mutex>
#include <unordered_map>
#include <list>
#include <cstdint>

struct CachedFile {
    std::string content;
    std::string mimeType;
    std::filesystem::file_time_type lastWriteTime;
    std::size_t size;
};

class FileCache {
public:
    explicit FileCache(std::size_t maxSizeBytes);

    std::optional<CachedFile> getFile(const std::filesystem::path& path);
    void clear();
    std::size_t getCurrentSizeBytes() const;

private:
    struct CacheEntry {
        CachedFile file;
        std::list<std::string>::iterator lruIt;
    };

    void evictIfNeeded(std::size_t requiredSize);

    std::size_t m_maxSizeBytes;
    std::size_t m_currentSizeBytes{0};
    
    mutable std::mutex m_mutex;
    
    std::list<std::string> m_lruList;
    std::unordered_map<std::string, CacheEntry> m_cache;
};
