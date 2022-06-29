#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <deque>
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "log_duration.h"
#include "remove_duplicates.h"

using std::literals::string_literals::operator""s;

// ==================== for example =========================

void PrintDocument(const Document& document) {
    std::cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::exception& e) {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    LOG_DURATION("Operation time"s, std::cout);
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}

//void MatchDocuments(const SearchServer& search_server, const std::string& query) {
//    LOG_DURATION("Operation time"s, std::cout);
//    try {
//        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
//        const int document_count = search_server.GetDocumentCount();
//        for (int index = 0; index < document_count; ++index) {
//            const int document_id = search_server.GetDocumentId(index);
//            const auto [words, status] = search_server.MatchDocument(query, document_id);
//            PrintMatchDocumentResult(document_id, words, status);
//        }
//    } catch (const std::exception& e) {
//        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
//    }
//}

/*int main() {
    //SearchServer search_server("и в на"s);

    //AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    //AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    //AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    //AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    //AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

    //FindTopDocuments(search_server, "пушистый -пёс"s);
    //FindTopDocuments(search_server, "пушистый --кот"s);
    //FindTopDocuments(search_server, "пушистый -"s);

    //MatchDocuments(search_server, "пушистый пёс"s);
    //MatchDocuments(search_server, "модный -кот"s);
    //MatchDocuments(search_server, "модный --пёс"s);
    //MatchDocuments(search_server, "пушистый - хвост"s);


    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly cat and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "curly cat tail"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});
    
    //// 1439 запросов с нулевым результатом
    //for (int i = 0; i < 1439; ++i) {
    //    request_queue.AddFindRequest("empty request"s);
    //}
    //// все еще 1439 запросов с нулевым результатом
    //request_queue.AddFindRequest("curly dog"s);
    //// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    //request_queue.AddFindRequest("big collar"s);
    //// первый запрос удален, 1437 запросов с нулевым результатом
    //request_queue.AddFindRequest("sparrow"s);
    //std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;

    FindTopDocuments(search_server, "big dog -cat"s);
    //MatchDocuments(search_server, "curly -tail");

    for (const auto [doc_id, words] : search_server) {
        std::cout << doc_id << std::endl;
        for (const auto word : words) {
            std::cout << word <<  std::endl;
        }
    }

    auto word_freq = search_server.GetWordFrequencies(1);
    if (word_freq.empty()) {
        std::cout << "Is nothing to do"s << std::endl;
    }
    for (const auto [word, freq] : word_freq) {
        std::cout << "WORD: " << word << std::endl;
        std::cout << "FREQ: " << freq << std::endl;
    }

    std::cout << "Count of documents before removing: "s << search_server.GetDocumentCount() << std::endl;

    search_server.RemoveDocument(5);

    std::cout << "Count of documents after removing: "s << search_server.GetDocumentCount() << std::endl;

    RemoveDuplicates(search_server);

    TestSearchServer();
}*/ 
int main() {
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server);
    std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    
    //const auto search_results = search_server.FindTopDocuments("curly dog"s);
    //int page_size = 1;
    //const auto pages = Paginate(search_results, page_size);

    //// Выводим найденные документы по страницам
    //for (auto page = pages.begin(); page != pages.end(); ++page) {
    //    std::cout << *page << std::endl;
    //    std::cout << "Page break"s << std::endl;
    //}

    //TestSearchServer();
}
