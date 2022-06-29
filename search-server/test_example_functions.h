#pragma once
#include <iostream>
#include <string>
#include "document.h"
#include "search_server.h"

using std::literals::string_literals::operator""s;

// -------- Start of macros ----------

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cout << std::boolalpha;
        std::cout << file << "("s << line << "): "s << func << ": "s;
        std::cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cout << " Hint: "s << hint;
        }
        std::cout << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
                const std::string& hint); 

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename F>
void RunTestImpl(const F& func, const std::string& func_name) {
    func();
    std::cerr << func_name << " OK"s << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), (#func))

// -------- End of macros ----------

// -------- Start of search engine unit tests ----------

void TestExcludeStopWordsFromAddedDocumentContent();
void TestExcludeMinusWordsFromSearchResults(); 
void TestDocumentsCount();
void TestMatchingDocumentBySearchRequest(); 
void TestRelevanceSortingDescOrder(); 
void TestAverageRatingResult(); 
void TestPredicateFunction(); 
void TestFilterByStatus(); 
void TestCorrectRelevanceDocument(); 

// Entry point to unit tests
void TestSearchServer(); 
// --------- End of searche engine unit tests  -----------
