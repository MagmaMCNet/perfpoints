// File: perfpoints.cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <openssl/sha.h>
#include "BLib.h"

#ifdef _WIN32
#include <windows.h>
unsigned int getPhysicalCoreCount() {
    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &length);
    std::vector<uint8_t> buffer(length);
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.data());

    if (!GetLogicalProcessorInformationEx(RelationProcessorCore, info, &length)) {
        return std::thread::hardware_concurrency(); // Fallback
    }

    unsigned int physicalCoreCount = 0;
    while (reinterpret_cast<uint8_t*>(info) < buffer.data() + length) {
        physicalCoreCount++;
        info = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
            reinterpret_cast<uint8_t*>(info) + info->Size);
    }
    return physicalCoreCount;
}
#elif __linux__
#include <fstream>
#include <set>

unsigned int getPhysicalCoreCount() {
    std::ifstream cpuinfo("/proc/cpuinfo");
    std::set<int> physicalCores;
    std::string line;

    while (std::getline(cpuinfo, line)) {
        if (line.find("core id") != std::string::npos) {
            int coreId = std::stoi(line.substr(line.find(":") + 1));
            physicalCores.insert(coreId);
        }
    }

    return physicalCores.size();
}
#else
unsigned int getPhysicalCoreCount() {
    return std::thread::hardware_concurrency(); // Fallback
}
#endif

using namespace std;
using namespace std::chrono;

// ANSI escape codes for colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
const int HASHTIME = 5;
string formatWithCommas(unsigned long number) {
    stringstream ss;
    ss.imbue(locale(""));  // Use the default locale to add commas
    ss << fixed << number;
    return ss.str();
}
string formatHashRate(double hashRate) {
    const char* units[] = { "H/s", "KH/s", "MH/s", "GH/s", "TH/s", "PH/s" };
    int unitIndex = 0;
    while (hashRate >= 1000.0 && unitIndex < 5) {
        hashRate /= 1000.0;
        unitIndex++;
    }
    stringstream stream;
    stream << fixed << setprecision(2) << hashRate << " " << units[unitIndex];
    return stream.str();
}

// Function to generate hashes
void generateHashesThreaded(unsigned int thread_id, unsigned int& totalHashCount, seconds duration) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    string input = "test_string_" + to_string(thread_id);
    auto start = high_resolution_clock::now();

    unsigned int localHashCount = 0;  // Thread-local counter

    while (duration_cast<seconds>(high_resolution_clock::now() - start).count() < duration.count()) {
        SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
        localHashCount++;  // Increment the thread-local counter
    }

    totalHashCount += localHashCount;  // Aggregating results after completion
}

void singleThreadedHashing() {
    cout << YELLOW << "[Single-threaded Hashing] Starting..." << RESET << endl;

    unsigned int hashCount = 0;

    generateHashesThreaded(1, hashCount, seconds(5));

    string hashesPerSecond = formatHashRate(static_cast<double>(hashCount) / HASHTIME);

    cout << GREEN << "[Single-threaded Hashing] Completed. Hashes generated: " << YELLOW << formatWithCommas(hashCount) << GREEN << ", Hash rate: " << YELLOW << hashesPerSecond << RESET << endl;
}

void multiThreadedHashing(unsigned int numThreads) {
    cout << YELLOW << "[Multi-threaded Hashing] Starting with " << numThreads << " threads..." << endl;

    vector<unsigned int> localHashCounts(numThreads, 0);  // Separate counter for each thread
    vector<thread> threads;

    mutex mtx;
    condition_variable cv;
    unsigned int completedThreads = 0;

    for (unsigned int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&, i] {
            generateHashesThreaded(i, localHashCounts[i], seconds(5));

            // Notify the main thread that this thread has finished
            {
                lock_guard<mutex> lock(mtx);
                completedThreads++;
            }
            cv.notify_one();
            });
    }

    // Wait for all threads to complete
    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&] { return completedThreads == numThreads; });
    }

    // Aggregate results
    unsigned long totalHashCount = 0;
    for (unsigned long count : localHashCounts)
        totalHashCount += count;

    string hashesPerSecond = formatHashRate(static_cast<double>(totalHashCount) / HASHTIME);

    cout << GREEN << 
        "[Multi-threaded Hashing] Completed. Total hashes generated: " << YELLOW << formatWithCommas(totalHashCount) << GREEN << ", Overall Hash rate: " << YELLOW << hashesPerSecond << RESET << endl;

    // Clean up threads
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

int main() {
    Sleep(100);
    EnableANSIColors(true);
    cout << CYAN << "[Perf Points] Starting..." << RESET << endl;
    Sleep(1000);

    // Single-threaded hashing test
    singleThreadedHashing();

    // Multi-threaded hashing test
    multiThreadedHashing(getPhysicalCoreCount());
    string _;
    cin >> _;
    return 0;
}
