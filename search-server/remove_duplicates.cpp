#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<std::set<std::string>> tmp_str;
    std::vector<int> duplicates_id;

    for (auto it = search_server.begin(); it != search_server.end(); ++it) {
        std::set<std::string> tmp_str_item;
        if (!search_server.GetWordFrequencies(*it).empty()) {
            for (const auto [word, freqs] : search_server.GetWordFrequencies(*it)) {
                tmp_str_item.insert(word);
            }     
        }

        if (tmp_str.count(tmp_str_item) > 0) {
            duplicates_id.push_back(*it);
        } else {
            tmp_str.insert(tmp_str_item);
        }

    }

    for (const auto id : duplicates_id) {
        search_server.RemoveDocument(id);
        std::cout << "Found duplicate document id "s << id << std::endl;
    }
}
