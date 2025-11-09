#include "httplib.h"
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <string>
#include <libpq-fe.h>
#include <queue>
#include <condition_variable>

using namespace std;
using namespace httplib;

#define cache_size 2
#define pool_size 32

class PGPool {
private:
    vector<PGconn*> pool;
    mutex mtx;
    condition_variable cv;
    int size;

public:
    PGPool(int n) : size(n) {
        for (int i = 0; i < n; i++) {
            PGconn *c = PQconnectdb("host=localhost port=5432 dbname=kvdb user=nithin password=Nithin@362");
            if (PQstatus(c) != CONNECTION_OK) {
                cerr << "DB Connection failed: " << PQerrorMessage(c) << endl;
                exit(1);
            }
            pool.push_back(c);
        }
    }

    PGconn* acquire() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]{ return !pool.empty(); });
        PGconn *c = pool.back(); pool.pop_back();
        return c;
    }

    void release(PGconn* c) {
        unique_lock<mutex> lock(mtx);
        pool.push_back(c);
        lock.unlock();
        cv.notify_one();
    }

    ~PGPool() {
        for (auto &c : pool) PQfinish(c);
    }
};

PGPool dbpool(pool_size);
PGconn *conn;

void db_put(int key, const string &value) {
    PGconn* c = dbpool.acquire();

    PQsetSingleRowMode(c); 
    string q = "INSERT INTO kvtb (key, value) VALUES (" + to_string(key) + ", '" + value +
               "') ON CONFLICT (key) DO UPDATE SET value='" + value + "';";

    PGresult *res = PQexec(c, q.c_str());
    PQclear(res);

    dbpool.release(c);
}


string db_get(int key) {
    PGconn* c = dbpool.acquire();

    PQsetSingleRowMode(c);
    string q = "SELECT value FROM kvtb WHERE key=" + to_string(key);
    PGresult *res = PQexec(c, q.c_str());

    string value = "";
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        value = PQgetvalue(res, 0, 0);
    }

    PQclear(res);
    dbpool.release(c);
    return value;
}


bool db_del(int key) {
    PGconn* c = dbpool.acquire();

    PQsetSingleRowMode(c); 
    string q = "DELETE FROM kvtb WHERE key=" + to_string(key);
    PGresult *res = PQexec(c, q.c_str());

    bool ok = (string(PQcmdTuples(res)) != "0");
    PQclear(res);
    dbpool.release(c);
    return ok;
}

class LRUCache {
private:
    int capacity;
    unordered_map<int, pair<string, list<int>::iterator>> cache;
    list<int> lru; 
    mutex mtx;

public:
    LRUCache(int cap) : capacity(cap) {}

    void put(int key, const string &value) {
        lock_guard<mutex> lock(mtx);

        if (cache.count(key)) {
            lru.erase(cache[key].second);
            lru.push_front(key);
            cache[key] = {value, lru.begin()};
            return;
        }

        if (cache.size() == capacity) {
            int old = lru.back();  
            lru.pop_back();
            cache.erase(old);
        }

        lru.push_front(key);
        cache[key] = {value, lru.begin()};
    }

    string get(int key) {
        lock_guard<mutex> lock(mtx);

        if (!cache.count(key)) return "";

        lru.erase(cache[key].second);
        lru.push_front(key);
        cache[key].second = lru.begin();

        return cache[key].first;
    }

    bool del(int key) {
        lock_guard<mutex> lock(mtx);

        if (!cache.count(key)) return false;

        lru.erase(cache[key].second);
        cache.erase(key);
        return true;
    }
};



int main() {
    LRUCache cache(cache_size);
    Server server;
 
    server.Post("/Create", [&](const Request &req, Response &res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Key is missing\n", "text/plain");
            return;
        }

        int key = stoi(req.get_param_value("id"));
        string value = req.body;

        cache.put(key, value);
        db_put(key, value);

        res.set_content("Key-value pair created/updated successfully\n", "text/plain");
    });

    server.Get("/Read", [&](const Request &req, Response &res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Key is missing\n", "text/plain");
            return;
        }

        int key = stoi(req.get_param_value("id"));
        string value = cache.get(key);

        if (value == "") {
            value = db_get(key);
            if (value == "") {
                res.status = 404;
                res.set_content("Key not found\n", "text/plain");
                return;
            }
            cache.put(key, value);
        }

        res.set_content(value + "\n", "text/plain");
    });

    server.Delete("/Delete", [&](const Request &req, Response &res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Key is missing\n", "text/plain");
            return;
        }

        int key = stoi(req.get_param_value("id"));

        bool removedCache = cache.del(key);
        bool removedDB = db_del(key);

        if (removedCache || removedDB)
            res.set_content("Key-value pair deleted successfully\n", "text/plain");
        else {
            res.status = 404;
            res.set_content("Key not found\n", "text/plain");
        }
    });
    server.new_task_queue = [] {
        return new httplib::ThreadPool(pool_size);
    };

    cout << "\nServer running at http://localhost:8080\n";
    server.listen("0.0.0.0", 8080);
}