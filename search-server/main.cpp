#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>


using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double PRECISION = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

struct Document {
    Document() = default;

    Document(int id, double relevance, int rating)
        : id(id)
        , relevance(relevance)
        , rating(rating) {
    }

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words) {
        set<string> non_empty_stop_words = MakeUniqueNonEmptyStrings(stop_words);
        for (const string& word : stop_words) {
            if (!IsValidWord(word))
                throw invalid_argument("Words should not contain deprecated characters [0, 31]"s);
            stop_words_ = non_empty_stop_words;
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {}   

    void AddDocument(int document_id,
                     const string& document,
                     DocumentStatus status, 
                     const vector<int>& ratings) {
        if ((document_id < 0) || (documents_.count(document_id) > 0)) {
            throw invalid_argument("The document ID must not be less than zero and the document ID must not match the one already added"s);
        }
        vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, 
                           DocumentData{ComputeAverageRating(ratings), 
                           status});
        document_ids_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, 
                                                DocumentPredicate document_predicate) const {
        Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < PRECISION) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query,
                                                DocumentStatus status) const {
        return FindTopDocuments(raw_query,
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    int GetDocumentId(int index) const {
        if (index >= 0 && index < GetDocumentCount()) {
            return document_ids_[index];
        }
        throw out_of_range("ID of document is out of range"s);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
                                                        int document_id) const {
        Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return tie(matched_words, documents_.at(document_id).status);
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;
    
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        // A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word))
                throw invalid_argument("Words should not contain forbidden characters [0, 31]"s);
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }   

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        return rating_sum / static_cast<int>(ratings.size());
    }
    
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        if (!text.empty() && text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
            throw invalid_argument("A word of query contains forbidden characters [0, 31] or '--' or empty string after '-'"s);
        }

        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
    
    Query ParseQuery(const string& text) const {
        if (text.empty()) {
            throw invalid_argument("String of query is epmty"s);
        }
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query,
                                      DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) { 
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({
                document_id,
                relevance,
                documents_.at(document_id).rating
            });
        }
        return matched_documents;
    }
};

// -------- Start of macros ----------

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename F>
void RunTestImpl(const F& func, const string& func_name) {
    func();
    cerr << func_name << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), (#func))

// -------- End of macros ----------

// -------- Start of search engine unit tests ----------

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server("a an"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto documents = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(documents.size(), 1u);
        const Document& doc0 = documents[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}

void TestExcludeMinusWordsFromSearchResults() {
    const int first_doc_id = 50;
    const string first_content = "big black dog"s;
    const vector<int> first_ratings = {3, 5, -2};
    const int second_doc_id = 51;
    const string second_content = "tiny black kitty"s;
    const vector<int> second_ratings = {4, 9, -8};
    const string raw_query = "black dog -big"s;
    {
        SearchServer server("a an in"s);
        server.AddDocument(first_doc_id, first_content, DocumentStatus::ACTUAL, first_ratings);
        ASSERT_HINT(server.FindTopDocuments(raw_query).empty(), "Documents which contain minus words must be exclude from documents"s);
    }

    {
        SearchServer server("a an in"s);
        server.AddDocument(second_doc_id, second_content, DocumentStatus::ACTUAL, second_ratings);
        ASSERT_HINT(!server.FindTopDocuments(raw_query).empty(), "Document should be found if it doesn't contain minus words"s);

    }
}

void TestDocumentsCount() {
    const int doc_id = 60;
    const string content = "huge gray owl"s;
    const vector<int> ratings = {2, 4, -1};
    {
        SearchServer server("a an in"s);
        ASSERT_HINT(server.GetDocumentCount() == 0, "Count of documents by default must be zero"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.GetDocumentCount() == 1, "Count of documents must be not zero before adding of document"s);

    }
}

void TestMatchingDocumentBySearchRequest() {
    const int doc_id = 33;
    const string minus_word = " -small"s;
    const string stop_words = "in on and"s;
    const string content = "small white cat"s;
    const vector<int> ratings = {4, 7, -8};
    const string raw_query = "white cat"s;
    const DocumentStatus added_status = DocumentStatus::ACTUAL;
    
    {
        SearchServer server("a an in"s);
        server.AddDocument(doc_id, content, added_status, ratings);
        const auto [words, status] = server.MatchDocument(raw_query, doc_id);
        ASSERT_HINT(words.size() > 0, "Vector of matched words must be not empty"s);
        ASSERT_HINT(status == added_status, "Status of matched document must be equal to added document"s);
        vector<string> raw_query_words = SplitIntoWords(raw_query);
        sort(raw_query_words.begin(), raw_query_words.end());
        ASSERT_HINT(words == raw_query_words, "Vectors of matching and querying should be equal"s);
    }

    {
        SearchServer server(stop_words);
        server.AddDocument(doc_id, content, added_status, ratings);
        const auto [words, status] = server.MatchDocument(raw_query + minus_word, doc_id);
        ASSERT_HINT(words.empty(), "Vector of matching word should not contain any words if document contains minus words"s);

        vector<string> splitted_stop_words = SplitIntoWords(stop_words);
        for (const string& stop_word : splitted_stop_words) {
            ASSERT_HINT(!count(words.begin(), words.end(), stop_word), "Matched document should not have any stop word"s);
        }
    }
}

void TestRelevanceSortingDescOrder() {
    SearchServer server("a an in"s);
    server.AddDocument(0, "a white cat and a fashionable collar"s, DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "kind dog expressive eyes"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    const auto documents = server.FindTopDocuments("fluffy kind cat"s);
    for (unsigned i = 0; i < documents.size(); ++i) {
        unsigned j = i + 1;
        if (j < documents.size()) {
            ASSERT_HINT(documents[i].relevance > documents[j].relevance, "All documents must be sorted by descending of rating"s);
        }
    }
}

void TestAverageRatingResult() {
    const int doc_id = 55;
    const string content = "cat in the city"s;
    const vector<int> ratings = {2, 4, -4};
    {
        SearchServer server("a an"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT(found_docs.size() > 0);
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        ASSERT_EQUAL_HINT((rating_sum / static_cast<int>(ratings.size())), found_docs[0].rating, "Average rating calculates wrong"s);
    }   
}

void TestPredicateFunction() {
    SearchServer server("a an"s);
    server.AddDocument(0, "a white cat and a fashionable collar"s, DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "kind dog expressive eyes"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    const auto documents = server.FindTopDocuments("fluffy kind cat"s, [](int document_id, DocumentStatus, int) {
        return document_id % 2 == 0;
    });
    ASSERT(documents.size() == 2);
    for (const auto& document : documents) {
        ASSERT(document.id % 2 == 0);
    }
}

void TestFilterByStatus() {
    const string raw_query = "fluffy kind cat"s;
    {
        SearchServer server("a an in"s);
        server.AddDocument(0, "a white cat and a fashionable collar"s, DocumentStatus::ACTUAL, {8, -3});
        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
        server.AddDocument(2, "kind dog expressive eyes"s, DocumentStatus::BANNED, {5, -12, 2, 1});

        const auto documents = server.FindTopDocuments(raw_query, DocumentStatus::BANNED);
        ASSERT(documents.size() == 1);

        const auto [words, status] = server.MatchDocument(raw_query, documents[0].id);
        ASSERT_HINT(status == DocumentStatus::BANNED, "Documents don't filtered by status"s);

        const auto not_existed_doc = server.FindTopDocuments(raw_query, DocumentStatus::REMOVED);
        ASSERT_HINT(!documents.empty(), "Documents must be empty if given wrong status"s);

    }
}

void TestCorrectRelevanceDocument() {
    const int first_doc_id = 6;
    const string first_doc_content = "a black cat and a fashionable collar"s;
    const vector<int> first_doc_ratings = {8, -3};
    const int second_doc_id = 3;
    const string second_doc_content = "white dog expressive eyes"s;
    const vector<int> second_doc_ratings = {9, -7};
    const string raw_query = "white cat"s;

    map<string, map<int, double>> word_to_document_freqs;

    map<int, vector<string>> doc_id_to_split_words;

    
    {
        SearchServer server(""s);

        server.AddDocument(first_doc_id, first_doc_content, DocumentStatus::ACTUAL, first_doc_ratings);
        server.AddDocument(second_doc_id, second_doc_content, DocumentStatus::ACTUAL, second_doc_ratings);

        const auto documents = server.FindTopDocuments(raw_query);
        
        const vector<string> first_doc_words = SplitIntoWords(first_doc_content);
        doc_id_to_split_words[first_doc_id] = first_doc_words;
        
        const vector<string> second_doc_words = SplitIntoWords(second_doc_content);
        doc_id_to_split_words[second_doc_id] = second_doc_words;

        for (const auto& [doc_id, split_words] : doc_id_to_split_words) {
            const double inv_word_count = 1.0 / split_words.size();
            for (const string& word : split_words) {
                word_to_document_freqs[word][doc_id] += inv_word_count;
            }
        }

        const vector<string> query_words = SplitIntoWords(raw_query);
        map<int, double> document_to_relevance;
        for (const string& q_word : query_words) {
            if (word_to_document_freqs.count(q_word) == 0) {
                continue;
            }
            const double inverse_document_freq = log(doc_id_to_split_words.size() * 1.0 / word_to_document_freqs.at(q_word).size());
            for (const auto [document_id, term_freq] : word_to_document_freqs.at(q_word)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }

        Document server_document = {};
        for (const auto& document : documents) {
            if (document.id == first_doc_id)
                server_document = document;
        }

        ASSERT_HINT(abs(document_to_relevance.at(first_doc_id) - server_document.relevance) < PRECISION, "Relevance of documents calculated incorrect"s);
    }
}

// Entry point to unit tests
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeMinusWordsFromSearchResults);
    RUN_TEST(TestDocumentsCount);
    RUN_TEST(TestMatchingDocumentBySearchRequest);
    RUN_TEST(TestRelevanceSortingDescOrder);
    RUN_TEST(TestAverageRatingResult);
    RUN_TEST(TestPredicateFunction);
    RUN_TEST(TestFilterByStatus);
    RUN_TEST(TestCorrectRelevanceDocument);
}

// --------- End of searche engine unit tests  -----------

// ==================== for example =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status,
                 const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

int main() {
    SearchServer search_server("и в на"s);

    AddDocument(search_server, 1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, 3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server, 4, "большой пёс скворец евгений"s, DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);

    TestSearchServer();
} 
