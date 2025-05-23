#pragma once

#include "ChunkCommons/ChunkCommons.h"
#include "CommonStructs.h"

#include <vector>

namespace Chunk
{

    class ChunkQuery
    {
    public:
        ~ChunkQuery() = default;
        ChunkQuery(const int chunk_size = 100,
                   const int overlap = 20,
                   const EmbeddingModel embedding_model = EmbeddingModel::HuggingFace,
                   const std::string &openai_api_key = "");

        std::vector<RAGLibrary::Document> ProcessSingleDocument(const RAGLibrary::Document &item,
                                                                const std::vector<float> &query_embedding,
                                                                const float similarity_threshold);
        std::vector<RAGLibrary::Document> ProcessDocuments(const std::vector<RAGLibrary::Document> &items,
                                                           const std::string &query,
                                                           const float similarity_threshold,
                                                           int max_workers = 4);

    protected:
        void ValidateModel();
        std::vector<std::vector<float>> GenerateEmbeddings(const std::vector<std::string> &chunks);

    private:
        int m_chunk_size;
        int m_overlap;
        EmbeddingModel m_embedding_model;
        std::string m_openai_api_key;
    };

}