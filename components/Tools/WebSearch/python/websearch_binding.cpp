#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "webSearch/IWebSearch.h"
#include "webSearch/ICrawler.h"
#include "webSearch/ICleaner.h"
#include "webSearch/JobManager.h"
#include "webSearch/BraveEngine.h"
#include "webSearch/CurlCrawler.h"
#include "webSearch/HtmlCleaner.h"

namespace py = pybind11;
using namespace purecpp::websearch;
using namespace pybind11::literals; 

static py::list job_to_py(const SearchJob& job)
{
    py::list out;
    for (std::size_t i = 0; i < job.urls.size(); ++i) {
        const auto& u  = job.urls[i];
        const auto& md = job.md_pages[i];
        out.append(py::dict("url"_a      = u.url,
                            "rank"_a     = u.rank,
                            "markdown"_a = md));
    }
    return out;
}

class PyWebSearchManager {
public:
    PyWebSearchManager(const std::string& engine,
                       std::size_t workers = std::thread::hardware_concurrency())
    {
        /* tiny factory – extend with new engines when needed */
        if (engine == "brave") {
            search_ = std::make_shared<BraveEngine>();
        } else {
            throw std::invalid_argument("unknown engine: " + engine);
        }
        crawler_ = std::make_shared<CurlCrawler>();
        cleaner_ = std::make_shared<HtmlCleaner>();
        mgr_     = std::make_unique<JobManager>(search_, crawler_, cleaner_, workers);
    }

    /** Blocking search that returns Markdown docs as Python objects. */
    py::list search(const std::string& query,
                    int                k          = 5,
                    const std::string& lang       = "",
                    py::function       progress_cb = py::none())
    {
        ProgressCallback cpp_cb = nullptr;
        if (!progress_cb.is_none()) {
            cpp_cb = [progress_cb](const ProgressEvent& ev) {
                static const char* names[] = {"Started","UrlFetched","UrlCleaned",
                                              "Done","Error"};
                py::gil_scoped_acquire gil;
                progress_cb(names[static_cast<int>(ev.stage)],
                            ev.detail,
                            std::chrono::duration_cast<std::chrono::milliseconds>(
                                ev.timestamp.time_since_epoch()).count());
            };
        }

        auto job = mgr_->submit(query, k, cpp_cb, lang).get();   // block
        return job_to_py(job);
    }

private:
    std::shared_ptr<IWebSearch>  search_;
    std::shared_ptr<ICrawler>    crawler_;
    std::shared_ptr<ICleaner>    cleaner_;
    std::unique_ptr<JobManager>  mgr_;
};

void bind_websearch(py::module_& m)
{
    py::enum_<JobStage>(m, "JobStage")
        .value("Started",    JobStage::Started)
        .value("UrlFetched", JobStage::UrlFetched)
        .value("UrlCleaned", JobStage::UrlCleaned)
        .value("Done",       JobStage::Done)
        .value("Error",      JobStage::Error);

    py::class_<UrlResult>(m, "UrlResult")
        .def_readonly("url",  &UrlResult::url)
        .def_readonly("rank", &UrlResult::rank);

    py::class_<PyWebSearchManager>(m, "WebSearchManager")
        .def(py::init<const std::string&, std::size_t>(),
             py::arg("engine")  = "brave",
             py::arg("workers") = std::thread::hardware_concurrency())
        .def("search", &PyWebSearchManager::search,
             py::arg("query"),
             py::arg("k")            = 5,
             py::arg("lang")         = "",
             py::arg("progress_cb")  = py::none(),
             R"pbdoc(
                 Executes a blocking web-search and returns a list of dicts
                 {url, rank, markdown}.  Optionally supply a progress callback
                 cb(stage:str, detail:str, timestamp_ms:int).
             )pbdoc");
}
