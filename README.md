# Поисковый сервер с использованием многопоточности 
Предоставляет возможность поиска документов по ключевым словам и их ранжирования по принципу TF-IDF. Поисковую выдачу также можно фильтровать с применением дополнительной функциональности, которую обеспечивает возможность задания таких параметров как: минус-слова, стоп-слова.

#### Ядро посковой системы
- Основу поисковой системы составляет:
1. [search_server.h](https://github.com/denisspawn/cpp-search-server/tree/main/search-server/search_server.h)
2. [search_server.cpp](https://github.com/denisspawn/cpp-search-server/tree/main/search-server/search_server.cpp)  

- Создание экземпляра основного класса **SearchServer**:
    В конструктор класса передается строка и доплнительные параметры (стоп-слова), раздененые пробелами. Конструктор также поддерживает передачу контейнера с возможностью использования for-range цикла.
  
- Добавление документов для осуществления поиска по ним. Метод **AddDocument**:
    В метод класса передаётся *id* документа, *статус*, *рейтинг* и *документ* в формате строки.

- Поиск документов. Метод **FindTopDocuments**:
    Принимает ключевые слова для поиска и возвращает вектор документов, соответсвующих запросу и отсортированных по **TF-IDF**.

#### Дополнительный функционал
1. Разбиение результатов поиска на страницы: 
- [pagination.h](https://github.com/denisspawn/cpp-search-server/blob/main/search-server/paginator.h)
3. Хранение истории запросов к серверу:
- [request_queue.h](https://github.com/denisspawn/cpp-search-server/blob/main/search-server/request_queue.h)
- [request_queue.cpp](https://github.com/denisspawn/cpp-search-server/blob/main/search-server/request_queue.cpp)
- кол-во хранимых запросов ограничевается заданным значением и смещается порядком очереди
