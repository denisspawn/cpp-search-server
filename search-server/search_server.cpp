#include "search_server.h"

void SearchServer::AddDocument(int document_id,
                 const std::string_view document,
                 DocumentStatus status, 
                 const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("The document ID must not be less than zero and the document ID must not match the one already added"s);
    }
    std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        std::string str_word(word);
        word_to_document_freqs_[str_word][document_id] += inv_word_count;
        auto it = word_to_document_freqs_.find(str_word);
        document_id_to_word_freqs_[document_id][it->first] += inv_word_count;
    }
    documents_.emplace(document_id, 
                       DocumentData{ComputeAverageRating(ratings), 
                       status});
    document_ids_.insert(document_id);
}


int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() {
    return document_ids_.end();
}

const std::map<std::string_view, double>& SearchServer
    ::GetWordFrequencies(int document_id) const {
    const auto it = document_id_to_word_freqs_.find(document_id);
    if (it != document_id_to_word_freqs_.end()) {
        return it->second;
    }
    static std::map<std::string_view, double> tmp_res_;
    return tmp_res_;
}

void SearchServer::RemoveDocument(int document_id) {
    SearchServer::RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id) {
    if (documents_.count(document_id) == 0)
        return;
    
    documents_.erase(document_id);
        
    const auto it = document_id_to_word_freqs_.find(document_id);
    if (it == document_id_to_word_freqs_.end())
        return;
        
    std::vector<std::string_view> tmp_words(it->second.size());

    std::transform (policy,
                    it->second.begin(), it->second.end(),
                    tmp_words.begin(),
                    [](const std::pair<std::string_view, double>& word_to_freq) { 
                        return word_to_freq.first; 
                    });

    for_each (policy,
              tmp_words.begin(), tmp_words.end(),
              [this, &document_id](const std::string_view word) {
                    word_to_document_freqs_.at(std::string(word)).erase(document_id);
              });

    document_id_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id) {
    if (documents_.count(document_id) == 0)
        return;
    
    documents_.erase(document_id);
        
    const auto it = document_id_to_word_freqs_.find(document_id);
    if (it == document_id_to_word_freqs_.end())
        return;
        
    std::vector<std::string_view> tmp_words(it->second.size());

    std::transform (policy,
                    it->second.begin(), it->second.end(),
                    tmp_words.begin(),
                    [](const std::pair<std::string_view, double>& word_to_freq) { 
                        return word_to_freq.first; 
                    });

    for_each (policy,
              tmp_words.begin(), tmp_words.end(),
              [this, &document_id](const std::string_view word) {
                    word_to_document_freqs_.at(std::string(word)).erase(document_id);
              });

    document_id_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    return SearchServer::MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const {
    Query query(ParseQuery(raw_query));
    bool contains_minus = std::any_of(policy,
                   query.minus_words.begin(), query.minus_words.end(),
                   [this, &document_id](const std::string_view word) {
                       return (word_to_document_freqs_.count(word) && 
                               word_to_document_freqs_.at(std::string(word)).count(document_id));
                   });

    if (contains_minus) {
        std::vector<std::string_view> empty;
        return {empty, documents_.at(document_id).status};
    }
    
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto it = std::copy_if (policy, 
                  query.plus_words.begin(), query.plus_words.end(),
                  matched_words.begin(),
                  [this, &document_id](const std::string_view word) {
                      return (word_to_document_freqs_.count(word) &&
                              word_to_document_freqs_.at(std::string(word)).count(document_id));
                  });
    
    matched_words.resize(distance(matched_words.begin(), it));
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const {
    Query query(ParseQuery(raw_query, false));
    bool contains_minus = std::any_of(policy,
                   query.minus_words.begin(), query.minus_words.end(),
                   [this, &document_id](const std::string_view word) {
                       return (word_to_document_freqs_.count(word) && 
                               word_to_document_freqs_.at(std::string(word)).count(document_id));
                   });

    if (contains_minus) {
        std::vector<std::string_view> empty;
        return {empty, documents_.at(document_id).status};
    }
    
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto it = std::copy_if (policy, 
                  query.plus_words.begin(), query.plus_words.end(),
                  matched_words.begin(),
                  [this, &document_id](const std::string_view word) {
                      return (word_to_document_freqs_.count(word) &&
                              word_to_document_freqs_.at(std::string(word)).count(document_id));
                  });
    
    matched_words.resize(distance(matched_words.begin(), it));
    std::sort(matched_words.begin(), matched_words.end());
    matched_words.erase(std::unique(matched_words.begin(), matched_words.end()), matched_words.end());

    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word))
            throw std::invalid_argument("Words should not contain forbidden characters [0, 31]"s);
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}   

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word) const {
    bool is_minus = false;
    if (!word.empty() && word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("A word of query contains forbidden characters [0, 31] or '--' or empty std::string after '-'"s);
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool is_sec_exec) const {
    if (text.empty()) {
        throw std::invalid_argument("String of query is epmty"s);
    }
    
    Query query;
    for (const std::string_view word : SplitIntoWords(text)) {
        QueryWord query_word(ParseQueryWord(word));
        if (query_word.is_stop)
            continue;
        if (query_word.is_minus) {
            query.minus_words.push_back(query_word.data);
        } else {
            query.plus_words.push_back(query_word.data);
        }
    }
   
    if (is_sec_exec) {
        std::sort(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
        std::sort(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
    }
    
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(std::string(word)).size());
}
