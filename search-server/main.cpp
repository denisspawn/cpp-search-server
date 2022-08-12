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
#include "remove_duplicates.h"
#include "process_queries.h"
#include "log_duration.h"

#include <random>

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
    //LOG_DURATION("Operation time"s, std::cout);
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
/*int main() {
    //SearchServer search_server("and with"s);

    //AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    //AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    //AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    //AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    //AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    //AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    //AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    //AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    //AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    //std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    //RemoveDuplicates(search_server);
    //std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    
    //const auto search_results = search_server.FindTopDocuments("curly dog"s);
    //int page_size = 1;
    //const auto pages = Paginate(search_results, page_size);

    //// Выводим найденные документы по страницам
    //for (auto page = pages.begin(); page != pages.end(); ++page) {
    //    std::cout << *page << std::endl;
    //    std::cout << "Page break"s << std::endl;
    //}

    //TestSearchServer();
}*/

/*int main() {
    
    using namespace std;
    
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for (
        const auto& documents : ProcessQueries(search_server, queries)
    ) {
        cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << endl;
    }

    return 0;
} */

/* int main() {
    using namespace std;
    
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const vector<string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
        cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
    }

    return 0;
} */

/* int main() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(execution::par, 2);
    report();

    return 0;
} */

/*int main() {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const string query = "curly and funny -not"s;

    {
        const auto [words, status] = search_server.MatchDocument(query, 1);
        cout << words.size() << " words for document 1"s << endl;
        for (auto& word : words) {
            cout << word << endl;
        }
        // 1 words for document 1
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::seq, query, 2);
        cout << words.size() << " words for document 2"s << endl;
        for (auto& word : words) {
            cout << word << endl;
        }
        // 2 words for document 2
    }

    {
        const auto [words, status] = search_server.MatchDocument(execution::par, query, 3);
        cout << words.size() << " words for document 3"s << endl;
        for (auto& word : words) {
            cout << word << endl;
        }
        // 0 words for document 3
    }
   
    return 0;
} */

using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}
vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}
string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}
vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}
template <typename ExecutionPolicy>
void Test(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const string_view query : queries) {
        for (const auto& document : search_server.FindTopDocuments(policy, query)) {
            total_relevance += document.relevance;
        }
    }
    cout << total_relevance << endl;
}

#define TEST(policy) Test(#policy, search_server, queries, execution::policy)

int main() {
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    
    cout << "=================================="s << endl;
    
    {
        mt19937 generator;
        const auto dictionary = GenerateDictionary(generator, 1000, 10);
        const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);
        SearchServer search_server(dictionary[0]);
        for (size_t i = 0; i < documents.size(); ++i) {
            search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
        }
        const auto queries = GenerateQueries(generator, dictionary, 100, 70);
        TEST(seq);
        TEST(par);
    }
    
    return 0;
} 
