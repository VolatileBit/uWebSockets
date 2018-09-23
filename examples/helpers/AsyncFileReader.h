#include <map>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <future>

/* This is just a very simple and inefficient demo of async responses,
 * please do roll your own variant or use a database or Node.js's async
 * features instead of this really bad demo */
struct AsyncFileReader {
private:
    /* The cache we have in memory for this file */
    std::string cache;
    int cacheOffset;
    bool hasCache;

    /* The pending async file read (yes we only support one pending read) */
    std::function<void(std::string_view)> pendingReadCb;

    int fileSize;
    std::ifstream fin;
    uWS::Loop *loop;

public:
    /* Construct a demo async. file reader for fileName */
    AsyncFileReader(std::string fileName) {
        fin.open(fileName, std::ios::binary);

        // get fileSize
        fin.seekg(0, fin.end);
        fileSize = fin.tellg();

        //std::cout << "File size is: " << fileSize << std::endl;

        // cache up 1 mb!
        cache.resize(1024 * 1024);

        //std::cout << "Caching 1 MB at offset = " << 0 << std::endl;
        fin.seekg(0, fin.beg);
        fin.read(cache.data(), cache.length());
        cacheOffset = 0;
        hasCache = true;

        // get loop for thread

        loop = uWS::Loop::defaultLoop();
    }

    /* Returns any data already cached for this offset */
    std::string_view peek(int offset) {
        /* Did we hit the cache? */
        if (hasCache && offset >= cacheOffset && ((offset - cacheOffset) < cache.length())) {
            /* Cache hit */
            //std::cout << "Cache hit!" << std::endl;
            return std::string_view(cache.data() + offset - cacheOffset, cache.length() - offset + cacheOffset);
        } else {
            /* Cache miss */
            //std::cout << "Cache miss!" << std::endl;
            return std::string_view(nullptr, 0);
        }
    }

    /* Asynchronously request more data at offset */
    void request(int offset, std::function<void(std::string_view)> cb) {

        // in this case, what do we do?
        // we need to queue up this chunk request and callback!
        // if queue is full, either block or close the connection via abort!
        if (!hasCache) {
            // already requesting a chunk!
            std::cout << "ERROR: already requesting a chunk!" << std::endl;
            return;
        }

        // disable cache
        hasCache = false;

        std::async(std::launch::async, [this, cb, offset]() {
            std::cout << "ASYNC Caching 1 MB at offset = " << offset << std::endl;



            fin.seekg(offset, fin.beg);
            fin.read(cache.data(), cache.length());
            cacheOffset = offset;

            loop->defer([this, cb]() {

                int chunkSize = std::min(cache.length(), fileSize - offset);

                if (chunkSize != cache.length()) {
                    std::cout << "LESS THAN A CACHE 1 MB!" << std::endl;
                }

                hasCache = true;
                cb(std::string_view(cache.data(), chunkSize));
            });
        });
    }

    /* Abort any pending async. request */
    void abort() {

    }

    int getFileSize() {
        return fileSize;
    }
};
