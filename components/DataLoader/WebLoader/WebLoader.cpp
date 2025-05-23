#include "WebLoader.h"

#include "beauty/beauty.hpp"
#include "RagException.h"

namespace WebLoader
{
    WebLoader::WebLoader(const std::string url, const int &numThreads) : DataLoader::BaseDataLoader(numThreads)
    {
        AddThreadsCallback([this](RAGLibrary::DataExtractRequestStruct url)
                           {
            if (auto pageData = ScrapURL(url.targetIdentifier))
                ExtractTextFromHTML(url.targetIdentifier, *pageData); });

        if (!url.empty())
        {
            const auto urlsToScrap = std::vector<RAGLibrary::DataExtractRequestStruct>{{url}};
            InsertWorkIntoThreads(urlsToScrap);
        }
    }

    std::optional<std::string> WebLoader::ScrapURL(const std::string &url)
    {
        beauty::client client;
        beauty::request req;
        req.set("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36");
        req.method(beast::http::verb::get);

        auto [ec, response] = client.send_request(std::move(req), std::chrono::milliseconds(0), std::string(url));
        if (!ec)
        {
            if (response.status() == boost::beast::http::status::ok)
            {
                std::cout << std::format("Scrapped {}", url) << std::endl;
                return response.body();
            }
            else
            {
                std::cerr << std::format("Non OK status {} from response: {}({})", url, std::string(response.reason()), static_cast<int>(response.status())) << std::endl;
            }
        }
        else
        {
            std::cerr << std::format("Error when sending request to: {} error: {}", url, ec.message()) << std::endl;
        }

        return std::nullopt;
    }

    void WebLoader::ExtractTextFromHTML(const std::string &urlPath, const std::string &htmlData)
    {
        lxb_html_document_t *document = lxb_html_document_create();
        if (!document)
        {
            throw RAGLibrary::RagException("Failed to create HTML document");
        }

        try
        {
            auto status = lxb_html_document_parse(document, reinterpret_cast<unsigned char *>(const_cast<char *>(htmlData.c_str())), htmlData.size());

            if (status != LXB_STATUS_OK)
            {
                lxb_html_document_destroy(document);
                throw RAGLibrary::RagException(std::format("Failed to parse HTML document with status: {}", status));
            }

            if (auto *body = lxb_html_document_body_element(document); body != nullptr)
            {
                ExtractBodyText(lxb_dom_interface_node(body));
                RAGLibrary::Metadata metadata = {{"source", urlPath}};
                m_dataVector.emplace_back(metadata, m_extractedText);
            }

            lxb_html_document_destroy(document);
        }
        catch (const std::exception &e)
        {
            lxb_html_document_destroy(document);
            std::cerr << e.what() << '\n';
        }
    }

    void WebLoader::ExtractBodyText(lxb_dom_node_t *node)
    {
        if (!node)
            return;

        switch (node->type)
        {
        case LXB_DOM_NODE_TYPE_ELEMENT:
        {
            lxb_tag_id_t tag_id = lxb_dom_element_tag_id(reinterpret_cast<lxb_dom_element_t *>(node));
            if (tag_id == LXB_TAG_SCRIPT || tag_id == LXB_TAG_STYLE || tag_id == LXB_TAG_META ||
                tag_id == LXB_TAG_HEAD || tag_id == LXB_TAG_NOSCRIPT || tag_id == LXB_TAG_TITLE ||
                tag_id == LXB_TAG_LINK)
            {
                return;
            }
            break;
        }
        case LXB_DOM_NODE_TYPE_COMMENT:
            return;

        case LXB_DOM_NODE_TYPE_TEXT:
        {
            const unsigned char *text = lxb_dom_node_text_content(node, nullptr);
            if (text && *text != '\0')
            {
                std::string textStr(reinterpret_cast<const char *>(text));
                if (!textStr.empty() && !std::all_of(textStr.begin(), textStr.end(), isspace))
                {
                    m_extractedText += textStr + " ";
                }
            }
            return;
        }

        default:
            break;
        }

        for (lxb_dom_node_t *child = lxb_dom_node_first_child(node); child != nullptr; child = lxb_dom_node_next(child))
        {
            ExtractBodyText(child);
        }
    }
}