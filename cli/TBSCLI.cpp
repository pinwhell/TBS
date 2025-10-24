#include <TBS/TBS.hpp>
#include <cxxopts.hpp>
#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#endif

class FileView {
public:
    inline FileView(const char* filePath)
        : fileHandle(nullptr)
        , fileMapping(nullptr)
        , mapView(nullptr)
    {
        Init(filePath);
    }

    inline ~FileView() {

        Release();
    }

    operator const void* () const
    {
        return mapView;
    }

    size_t size() const
    {
        return fileSize;
    }

private:
    size_t fileSize;
    union {
        void* fileHandle;
        int fileHandleI;
    };
    void* fileMapping;
    union {
        void* mapView;
        int mapViewI;
    };

#ifdef __linux__
    inline void Init(const char* filePath)
    {
        fileSize = std::filesystem::file_size(filePath);

        if ((fileSize > 0) == false)
            throw std::runtime_error("Invalid File Size");

        fileHandleI = open(filePath, O_RDONLY);

        if (fileHandleI < 0)
            throw std::runtime_error("File Open Failed");

        mapView = mmap(nullptr, fileSize, PROT_READ, MAP_SHARED, fileHandleI, 0);

        if (mapViewI == -1)
        {
            close(fileHandleI);
            throw std::runtime_error("File Mapping Failed");
        }
    }

    inline void Release()
    {
        if (mapViewI != -1 && mapView != nullptr)
        {
            munmap(mapView, fileSize);
        }

        if (fileHandleI > 0)
            close(fileHandleI);
    }
#endif


#ifdef _WIN32
    inline void Init(const char* filePath)
    {
        fileHandle = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (fileHandle == INVALID_HANDLE_VALUE)
            throw std::runtime_error("Error opening file");

        fileSize = std::filesystem::file_size(filePath);

        fileMapping = CreateFileMappingA(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (fileMapping == nullptr)
        {
            CloseHandle(fileHandle);
            throw std::runtime_error("Error creating file mapping");
        }

        mapView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);

        if (mapView == nullptr) {
            CloseHandle(fileMapping);
            CloseHandle(fileHandle);
            throw std::runtime_error("Error mapping view of file");
        }
    }

    inline void Release()
    {
        if (mapView != nullptr) {
            UnmapViewOfFile(mapView);
        }

        if (fileMapping != nullptr) {
            CloseHandle(fileMapping);
        }

        if (fileHandle != nullptr && fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(fileHandle);
        }
    }
#endif
};

using namespace cxxopts;

int TBSCLI(int argc, const char* argv[])
{
    std::string file;
    std::string pattern;
    bool bSingleRes = false;
    bool bNaked = false;
    bool bJson = false;

    cxxopts::Options options("TBSCLI", "Command Line Interface for TBS");

    options.allow_unrecognised_options();

    options.add_options()
        ("f,file", "File to perform the scan at", cxxopts::value<std::string>()) // a bool parameter
        ("p,pattern", "Pattern to scan for", cxxopts::value<std::string>())
        ("s,single", "show first result", cxxopts::value<bool>()->default_value("false"))
        ("n,naked", "to keep neat output raw naked output", cxxopts::value<bool>()->default_value("false"))
        ("j,json", "to output as JSON", cxxopts::value<bool>()->default_value("false"))
        ;

    auto result = options.parse(argc, argv);

    if (!result.count("file") ||
        !result.count("pattern"))
    {
        std::cout << options.help() << std::endl;
        return 0;
    }

    pattern = result["pattern"].as<std::string>();

    if (!TBS::Pattern::Valid(pattern))
    {
        printf("Pattern '%s' invalid\n", pattern.c_str());
        return 1;
    }

    file = result["file"].as<std::string>();

    if (!std::filesystem::exists(file))
    {
        printf("File '%s' Does not exist\n", file.c_str());
        return 2;
    }

    bSingleRes = result["single"].as<bool>();
    bNaked = result["naked"].as<bool>();
    bJson = result["json"].as<bool>();

    try {
        FileView fileView(file.c_str());
        const char* fileBegin = (char*)((const void*)fileView);
        const char* fileEnd = fileBegin + fileView.size();

        const auto handleSingleResult = [&] {
            TBS::Pattern::Result result;

            if (!TBS::Light::ScanOne(fileBegin, fileEnd, result, pattern.c_str()))
            {
                printf("pattern '%s' not found in '%s'\n", pattern.c_str(), file.c_str());
                return 3;
            }

            if (bJson) 
                
                printf(
                R"({
                    \"%s\" : %d
                })", pattern.c_str(), result - (size_t)fileBegin);
            
            else printf("0x%016Xll", result - (size_t)fileBegin);

            return 0;
            };

        const auto handleMultiResult = [&] {
            TBS::Pattern::Results results;

            if (!TBS::Light::Scan(fileBegin, fileEnd, results, pattern.c_str()))
            {
                printf("pattern '%s' not found in '%s'\n", pattern.c_str(), file.c_str());
                return 3;
            }

            if (bJson)
            {
                printf(
                    R"({
                    \"%s\" : [
                ])", pattern.c_str());

                for (size_t i = 0; i < results.size(); i++)
                {
                    if (i != 0)
                        printf(", ");

                    printf("%ull", results[i] - (size_t)fileBegin);
                }

                printf("]\n}");

                return 0;
            }

            for (auto res : results)
                printf("0x%016Xll\n", res - (size_t)fileBegin);

            return 0;
            };

        return (bSingleRes ? handleSingleResult() : handleMultiResult());
    }
    catch (const std::exception& e)
    {
        printf("%s", e.what());
        return 2;
    }

    return 0;
}