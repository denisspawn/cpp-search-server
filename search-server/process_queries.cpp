#include "process_queries.h"

using std::literals::string_literals::operator""s;

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) 
{
    std::vector<std::vector<Document>> documents(queries.size());
    
    std::transform(std::execution::par,
                   queries.cbegin(), queries.cend(),
                   documents.begin(),
                   [&search_server](const std::string& query) {
                       return search_server.FindTopDocuments(query);
                   });
    
    return documents;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> array_documents =  ProcessQueries(
        search_server, queries);
    
    std::vector<Document> joined_documents;
    
    for (auto& documents : array_documents) {
        joined_documents.insert(joined_documents.end(), documents.begin(), documents.end());
    }
    
    return joined_documents; 
}
