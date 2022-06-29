#include "test_example_functions.h"

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint) {
    if (!value) {
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {1, 2, 3};
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
    const std::string first_content = "big black dog"s;
    const std::vector<int> first_ratings = {3, 5, -2};
    const int second_doc_id = 51;
    const std::string second_content = "tiny black kitty"s;
    const std::vector<int> second_ratings = {4, 9, -8};
    const std::string raw_query = "black dog -big"s;
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
    const std::string content = "huge gray owl"s;
    const std::vector<int> ratings = {2, 4, -1};
    {
        SearchServer server("a an in"s);
        ASSERT_HINT(server.GetDocumentCount() == 0, "Count of documents by default must be zero"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.GetDocumentCount() == 1, "Count of documents must be not zero before adding of document"s);

    }
}

void TestMatchingDocumentBySearchRequest() {
    const int doc_id = 33;
    const std::string minus_word = " -small"s;
    const std::string stop_words = "in on and"s;
    const std::string content = "small white cat"s;
    const std::vector<int> ratings = {4, 7, -8};
    const std::string raw_query = "white cat"s;
    const DocumentStatus added_status = DocumentStatus::ACTUAL;
    
    {
        SearchServer server("a an in"s);
        server.AddDocument(doc_id, content, added_status, ratings);
        const auto [words, status] = server.MatchDocument(raw_query, doc_id);
        ASSERT_HINT(words.size() > 0, "Vector of matched words must be not empty"s);
        ASSERT_HINT(status == added_status, "Status of matched document must be equal to added document"s);
        std::vector<std::string> raw_query_words = SplitIntoWords(raw_query);
        sort(raw_query_words.begin(), raw_query_words.end());
        ASSERT_HINT(words == raw_query_words, "Vectors of matching and querying should be equal"s);
    }

    {
        SearchServer server(stop_words);
        server.AddDocument(doc_id, content, added_status, ratings);
        const auto [words, status] = server.MatchDocument(raw_query + minus_word, doc_id);
        ASSERT_HINT(words.empty(), "Vector of matching word should not contain any words if document contains minus words"s);

        std::vector<std::string> splitted_stop_words = SplitIntoWords(stop_words);
        for (const std::string& stop_word : splitted_stop_words) {
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
    const std::string content = "cat in the city"s;
    const std::vector<int> ratings = {2, 4, -4};
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
    const std::string raw_query = "fluffy kind cat"s;
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
    const std::string first_doc_content = "a black cat and a fashionable collar"s;
    const std::vector<int> first_doc_ratings = {8, -3};
    const int second_doc_id = 3;
    const std::string second_doc_content = "white dog expressive eyes"s;
    const std::vector<int> second_doc_ratings = {9, -7};
    const std::string raw_query = "white cat"s;

    std::map<std::string, std::map<int, double>> word_to_document_freqs;

    std::map<int, std::vector<std::string>> doc_id_to_split_words;

    
    {
        SearchServer server(""s);

        server.AddDocument(first_doc_id, first_doc_content, DocumentStatus::ACTUAL, first_doc_ratings);
        server.AddDocument(second_doc_id, second_doc_content, DocumentStatus::ACTUAL, second_doc_ratings);

        const auto documents = server.FindTopDocuments(raw_query);
        
        const std::vector<std::string> first_doc_words = SplitIntoWords(first_doc_content);
        doc_id_to_split_words[first_doc_id] = first_doc_words;
        
        const std::vector<std::string> second_doc_words = SplitIntoWords(second_doc_content);
        doc_id_to_split_words[second_doc_id] = second_doc_words;

        for (const auto& [doc_id, split_words] : doc_id_to_split_words) {
            const double inv_word_count = 1.0 / split_words.size();
            for (const std::string& word : split_words) {
                word_to_document_freqs[word][doc_id] += inv_word_count;
            }
        }

        const std::vector<std::string> query_words = SplitIntoWords(raw_query);
        std::map<int, double> document_to_relevance;
        for (const std::string& q_word : query_words) {
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

        ASSERT_HINT(std::abs(document_to_relevance.at(first_doc_id) - server_document.relevance) < PRECISION, "Relevance of documents calculated incorrect"s);
    }
}

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
