#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "CommonConnectionPool.h"
#include "Connection.h"

using namespace std;
using namespace std::chrono;

struct BenchResult {
    atomic<int64_t> success{0};
    atomic<int64_t> failure{0};
    atomic<int64_t> total_wait_ms{0};
    atomic<int64_t> min_wait_ms{INT64_MAX};
    atomic<int64_t> max_wait_ms{0};
};

void worker(int thread_id, int req_per_thread, BenchResult& result) {
    auto pool = ConnectionPool::getConnectionPool();

    for (int i = 0; i < req_per_thread; ++i) {
        auto start = steady_clock::now();
        auto conn = pool->getConnection();
        auto end = steady_clock::now();

        auto wait_ms = duration_cast<milliseconds>(end - start).count();

        if (conn) {
            result.success++;
            result.total_wait_ms += wait_ms;

            // Update min atomically
            int64_t prev_min = result.min_wait_ms.load();
            while (wait_ms < prev_min &&
                   !result.min_wait_ms.compare_exchange_weak(prev_min, wait_ms))
                ;

            // Update max atomically
            int64_t prev_max = result.max_wait_ms.load();
            while (wait_ms > prev_max &&
                   !result.max_wait_ms.compare_exchange_weak(prev_max, wait_ms))
                ;

            // Simulate minimal work: a cheap query
            conn->query("SELECT 1");

        } else {
            result.failure++;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <threads> <requests_per_thread>" << endl;
        cerr << "Example: " << argv[0] << " 8 1000" << endl;
        return 1;
    }

    int num_threads = atoi(argv[1]);
    int req_per_thread = atoi(argv[2]);

    cout << "=== Connection Pool Benchmark ===" << endl;
    cout << "Threads: " << num_threads << endl;
    cout << "Requests per thread: " << req_per_thread << endl;
    cout << "Total requests: " << num_threads * req_per_thread << endl;
    cout << endl;

    // Warm up: wait for pool initial connections
    this_thread::sleep_for(seconds(1));

    BenchResult result;
    vector<thread> threads;

    auto bench_start = steady_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, i, req_per_thread, ref(result));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto bench_end = steady_clock::now();
    auto total_ms = duration_cast<milliseconds>(bench_end - bench_start).count();

    // --- Report ---
    cout << fixed << setprecision(2);
    cout << "=== Results ===" << endl;
    cout << "Total time:           " << total_ms << " ms" << endl;
    cout << "Success:              " << result.success.load() << endl;
    cout << "Failures (timeout):   " << result.failure.load() << endl;

    int64_t succ = result.success.load();
    if (succ > 0) {
        double throughput = (double)succ / total_ms * 1000.0;
        double avg_wait = (double)result.total_wait_ms.load() / succ;
        cout << "Throughput:           " << throughput << " req/s" << endl;
        cout << "Avg wait (acquire):   " << avg_wait << " ms" << endl;
        cout << "Min wait:             " << result.min_wait_ms.load() << " ms" << endl;
        cout << "Max wait:             " << result.max_wait_ms.load() << " ms" << endl;
    }

    if (result.failure > 0) {
        cout << endl;
        cout << "提示: 出现超时失败，可能原因：" << endl;
        cout << "  1. 并发量超过 mysql.ini 中 maxSize=" << endl;
        cout << "  2. 生产者线程来不及创建新连接" << endl;
        cout << "  3. connectionTimeout 设置太短" << endl;
        cout << "  → 建议增大 maxSize 或 connectionTimeout" << endl;
    }

    return 0;
}
