#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <chrono>
#include <map>
#include <sstream>
#include <iterator>
#include <fstream>

using namespace std;
using namespace std::chrono;

const vector<int> ARRAY_SIZES = [] {
    vector<int> sizes;
    for (int i = 100; i <= 3000; i += 100) sizes.push_back(i);
    return sizes;
}();

const int NUM_TESTS_PER_SIZE = 5;
const string CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#%:;^&*()-.";

enum DataType { RANDOM, REVERSE, NEARLY_SORTED };

// Генератор строк
class StringGenerator {
    static mt19937 gen;
    static uniform_int_distribution<> char_dist;
    static uniform_int_distribution<> len_dist;

public:
    static vector<string> generate(int size, DataType type) {
        vector<string> arr(size);
        for (auto& s : arr) {
            s.resize(10 + len_dist(gen) % 191);
            std::generate(s.begin(), s.end(), [&] { return CHARS[char_dist(gen)]; });
        }

        if (type != RANDOM) {
            sort(arr.begin(), arr.end());
            if (type == REVERSE) reverse(arr.begin(), arr.end());
            else for (int i = 0; i < size / 20; ++i)
                swap(arr[uniform_int_distribution<>(0, size - 1)(gen)],
                    arr[uniform_int_distribution<>(0, size - 1)(gen)]);
        }
        return arr;
    }
};
mt19937 StringGenerator::gen(random_device{}());
uniform_int_distribution<> StringGenerator::char_dist(0, CHARS.size() - 1);
uniform_int_distribution<> StringGenerator::len_dist(10, 200);

class StringSortTester {
    static int comparisons;

    static int compare(const string& a, const string& b) {
        comparisons++;
        int min_len = min(a.size(), b.size());
        for (int i = 0; i < min_len; ++i) {
            if (a[i] != b[i]) return a[i] - b[i];
        }
        return a.size() - b.size();
    }
    
    static void quickSort(vector<string>& arr, int left, int right) {
        if (left >= right) return;
        
        int mid = left + (right - left)/2;
        string pivot = arr[mid];
        int i = left, j = right;
        
        while (i <= j) {
            while (compare(arr[i], pivot) < 0) i++;
            while (compare(arr[j], pivot) > 0) j--;
            if (i <= j) swap(arr[i++], arr[j--]);
        }
        
        quickSort(arr, left, j);
        quickSort(arr, i, right);
    }

    static void merge(vector<string>& arr, int left, int mid, int right) {
        vector<string> temp(right - left + 1);
        int i = left, j = mid + 1, k = 0;
        
        while (i <= mid && j <= right) {
            if (compare(arr[i], arr[j]) <= 0) temp[k++] = arr[i++];
            else temp[k++] = arr[j++];
        }

        while (i <= mid) temp[k++] = arr[i++];
        while (j <= right) temp[k++] = arr[j++];
        
        for (int idx = 0; idx < k; idx++) {
            arr[left + idx] = temp[idx];
        }
    }

    static void mergeSort(vector<string>& arr, int left, int right) {
        if (left >= right) return;
        
        int mid = left + (right - left)/2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }

    // реализации сортировок с контеста
    static void mergeLCP(vector<string>& arr, int l, int m, int r) {
        vector<string> temp(r - l + 1);
        int i = l, j = m + 1, k = 0;
        
        while (i <= m && j <= r) {
            if (compare(arr[i], arr[j]) <= 0) temp[k++] = arr[i++];
            else temp[k++] = arr[j++];
        }
        
        while (i <= m) temp[k++] = arr[i++];
        while (j <= r) temp[k++] = arr[j++];
        
        for (int idx = 0; idx < k; ++idx)
            arr[l + idx] = temp[idx];
    }

    static void mergeSortLCP(vector<string>& arr, int l, int r) {
        if (l >= r) return;
        int m = l + (r - l)/2;
        mergeSortLCP(arr, l, m);
        mergeSortLCP(arr, m+1, r);
        mergeLCP(arr, l, m, r);
    }

    static void ternaryStringQuickSort(vector<string>& arr, int left, int right, int d = 0) {
        if (left >= right) return;

        char pivot = (d < arr[left + (right-left)/2].size()) ? 
                    arr[left + (right-left)/2][d] : '\0';
        int lt = left, gt = right;
        int i = left;

        while (i <= gt) {
            char cur_ch = (d < arr[i].size()) ? arr[i][d] : '\0';
            
            if (compare(string(1, cur_ch), string(1, pivot)) < 0)
                swap(arr[lt++], arr[i++]);
            else if (compare(string(1, cur_ch), string(1, pivot)) > 0)
                swap(arr[i], arr[gt--]);
            else 
                i++;
        }

        ternaryStringQuickSort(arr, left, lt-1, d);
        if (pivot != '\0')
            ternaryStringQuickSort(arr, lt, gt, d+1);
        ternaryStringQuickSort(arr, gt+1, right, d);
    }

    static void msdRadixSort(vector<string>& arr, int low, int high, int d = 0) {
        if (low >= high) return;

        const int num = 257;
        vector<vector<string>> buckets(num);

        for (int i = low; i <= high; i++) {
            if (d >= arr[i].size()) buckets[0].push_back(arr[i]);
            else {
                unsigned char c = arr[i][d];
                buckets[c+1].push_back(arr[i]);
            }
        }

        int ind = low;
        for (auto& bucket : buckets)
            for (string& s : bucket)
                arr[ind++] = s;

        int start = low;
        for (int i = 1; i < num; i++) {
            int bucket_size = buckets[i].size();
            if (bucket_size > 0) {
                msdRadixSort(arr, start, start + bucket_size - 1, d + 1);
                start += bucket_size;
            }
        }
    }

    static void msdRadixWithQuickSort(vector<string>& arr, int low, int high, int d = 0) {
        if (high - low + 1 < 74) {
            ternaryStringQuickSort(arr, low, high, d);
            return;
        }
        msdRadixSort(arr, low, high, d);
    }

public:
    static pair<string, int> test_algorithm(vector<string> arr, const string& algo) {
        comparisons = 0;
        auto start = high_resolution_clock::now();

        if (algo == "quicksort") {
        quickSort(arr, 0, arr.size()-1);
        }
        else if (algo == "mergesort") {
            mergeSort(arr, 0, arr.size()-1);
        }
        else if (algo == "ternary_quicksort") {
            ternaryStringQuickSort(arr, 0, arr.size()-1);
        }
        else if (algo == "msd_radix") {
            msdRadixSort(arr, 0, arr.size()-1);
        }
        else if (algo == "msd_hybrid") {
            msdRadixWithQuickSort(arr, 0, arr.size()-1);
        }
        else if (algo == "lcp_mergesort") {
            mergeSortLCP(arr, 0, arr.size()-1);
        }

        auto end = high_resolution_clock::now();
        double time = duration_cast<microseconds>(end - start).count() / 1000.0;
        return {to_string(time), comparisons};
    }
};
int StringSortTester::comparisons = 0;

int main() {
    vector<string> algorithms = {
        "quicksort", "mergesort", "ternary_quicksort",
        "msd_radix", "msd_hybrid", "lcp_mergesort"
    };

    cout << "Алгоритм;Размер;ТипДанных;Время(мс);Сравнения\n";

    for (int size : ARRAY_SIZES) {
        for (int type : {RANDOM, REVERSE, NEARLY_SORTED}) {
            auto data = StringGenerator::generate(size, static_cast<DataType>(type));

            for (auto& algo : algorithms) {
                double total_time = 0;
                int total_comp = 0;

                for (int i = 0; i < NUM_TESTS_PER_SIZE; ++i) {
                    auto result = StringSortTester::test_algorithm(data, algo);
                    total_time += stod(result.first);
                    total_comp += result.second;
                }

                cout << algo << ";"
                     << size << ";"
                     << type << ";"
                     << total_time/NUM_TESTS_PER_SIZE << ";"
                     << total_comp/NUM_TESTS_PER_SIZE << "\n";
            }
        }
    }
}