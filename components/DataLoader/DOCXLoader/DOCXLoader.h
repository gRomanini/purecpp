#ifndef DOCX_LOADER_H
#define DOCX_LAODER_H

#include <filesystem>
#include <string_view>
#include <utility>

#include "BaseLoader.h"
namespace DOCXLoader
{
    constexpr std::string_view xmlDefaultPath = "word/document.xml";

    class DOCXLoader : public DataLoader::BaseDataLoader
    {
    public:
        DOCXLoader() = default;
        DOCXLoader(const std::string filePath, const unsigned int &numThreads = 1);
        ~DOCXLoader() = default;

    private:
        std::optional<std::pair<std::string, int>> ExtractZIPFile(const RAGLibrary::DataExtractRequestStruct &path);
        void ExtractTextFromXML(std::filesystem::path filePath, const std::pair<std::string, int> &data);

        mutable std::mutex m_mutex;
    };
    using DOCXLoaderPtr = std::shared_ptr<DOCXLoader>;
}
#endif
