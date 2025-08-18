#pragma once
#include "webSearch/IWebSearch.h"
#include "webSearch/ICrawler.h"
#include "webSearch/ICleaner.h"
#include "webSearch/IEmitter.h"
#include "webSearch/ThreadPool.h"

#include <future>
#include <memory>
#include <string>
#include <vector>

namespace purecpp::websearch {

/* Resultado completo de um job */
struct SearchJob {
    std::string                id;
    std::string                query;
    int                        k;
    std::vector<UrlResult>     urls;
    std::vector<std::string>   md_pages;   // mesmo tamanho de urls
};

class JobManager {
public:
    JobManager(std::shared_ptr<IWebSearch> search,
               std::shared_ptr<ICrawler>   crawler,
               std::shared_ptr<ICleaner>   cleaner,
               std::size_t                 n_workers);

    /** Envia job e devolve std::future para ele. */
    std::future<SearchJob> submit(const std::string&     query,
                                  int                    k,
                                  ProgressCallback       cb  = nullptr,
                                  const std::string&     lang = "");

private:
    ThreadPool                        pool_;
    std::shared_ptr<IWebSearch>       search_;
    std::shared_ptr<ICrawler>         crawler_;
    std::shared_ptr<ICleaner>         cleaner_;
};

} // namespace purecpp::websearch
