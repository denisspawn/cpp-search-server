#pragma once
#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <execution>
#include <string_view>
#include <functional>

using std::literals::string_literals::operator""s;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double PRECISION = 1e-6;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    
    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {} 

    explicit SearchServer(const std::string_view stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {}   

    void AddDocument(int document_id,
                     const std::string_view document,
                     DocumentStatus status, 
                     const std::vector<int>& ratings);

    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, 
                                           const std::string_view raw_query, 
                                           DocumentPredicate document_predicate) const;
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, 
                                           DocumentPredicate document_predicate) const {
        return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy, 
                                           const std::string_view raw_query,
                                           DocumentStatus status) const {
        return FindTopDocuments(policy, raw_query,
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }

    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                           DocumentStatus status) const {
        return FindTopDocuments(std::execution::seq, raw_query,
            [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
    
    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy policy,
                                           const std::string_view raw_query) const {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const {
        return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const;

    //int GetDocumentId(int index) const;
    std::set<int>::const_iterator begin();
    std::set<int>::const_iterator end();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
                                                        int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query,
                                                        int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query,
                                                        int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;
    std::map<int, std::map<std::string_view, double>> document_id_to_word_freqs_;
     
    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);
    
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);
    
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    
    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    
    Query ParseQuery(const std::string_view text, bool is_sec_exec = true) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(ExecutionPolicy policy,
                                           const Query& query,
                                           DocumentPredicate document_predicate) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
                                           DocumentPredicate document_predicate) const  {
        return FindAllDocuments(std::execution::seq, query, document_predicate);
    }
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    for (const auto word : stop_words) {
        if (!IsValidWord(word))
            throw std::invalid_argument("Words should not contain deprecated characters [0, 31]"s);
    }
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy policy,
                                            const std::string_view raw_query, 
                                            DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (std::abs(lhs.relevance - rhs.relevance) < PRECISION) {
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

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy policy,
                                                     const Query& query,
                                                     DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(50);
    for_each(policy, 
            query.plus_words.begin(), query.plus_words.end(),
            [this, &document_predicate, &document_to_relevance](const std::string_view word) {
                if (word_to_document_freqs_.count(word)) {
                    const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
                    for (const auto [document_id, term_freq] : word_to_document_freqs_.at(std::string(word))) {
                       const auto& document_data = documents_.at(document_id);
                       if (document_predicate(document_id, document_data.status, document_data.rating)) { 
                           document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                       }
                    }
                }
            });
    
    for_each(policy,
            query.minus_words.begin(), query.minus_words.end(),
            [this, &document_to_relevance, &policy](const std::string_view word) {
                if (word_to_document_freqs_.count(word)) {
                    for (const auto [document_id, _] : word_to_document_freqs_.at(std::string(word))) {
                        document_to_relevance.erase(document_id);
                    }
                }
            });

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({
            document_id,
            relevance,
            documents_.at(document_id).rating
        });
    }
    return matched_documents;
}
