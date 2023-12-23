#include <vector>
#include <random>
#include <optional>
#include <string>
#include <algorithm>

class BadHashFunctionException : public std::exception {
    std::string error_message_;

public:
    explicit BadHashFunctionException(const std::string& message) : error_message_(message) {
    }

    const char* what() const noexcept override {
        return error_message_.c_str();
    }
};

class LinearHashFunction {
    int coefficien_;
    int bias_;
    int k_prime_;
    int64_t not_negative_value_;
    static const int64_t kMaxValue = 1000000000;

public:
    LinearHashFunction(int coefficien, int bias, int k_prime) :
    coefficien_(coefficien), bias_(bias), k_prime_(k_prime),
    not_negative_value_(k_prime_ * kMaxValue) {
    }

    int GetHash(int value) const {
        int ans =  (static_cast<int64_t>(value) * coefficien_ +
            bias_ + not_negative_value_) % k_prime_;
        if (ans < 0) {
            throw BadHashFunctionException("ans < 0");
        }
        return ans;
    }
};

class GenerateLinearHashFunction {
    std::mt19937 generator_;

public:
    static const int kPrime = 1000000021;

    GenerateLinearHashFunction() : generator_(std::random_device()()) {
    }

    LinearHashFunction Generate() {
        std::uniform_int_distribution<int> distribution_coefficien(1, kPrime - 1);
        std::uniform_int_distribution<int> distribution_bias(0, kPrime - 1);
        int coefficien = distribution_coefficien(generator_);
        int bias = distribution_bias(generator_);
        return LinearHashFunction(coefficien, bias, kPrime);
    }
};


class FixedSet {
    class InnerSet {
        std::optional<LinearHashFunction> hash_;
        std::vector<std::optional<int>> data_;

        static std::vector<std::optional<int>> Split(
                const std::vector<int>& numbers,
                LinearHashFunction& hash,
                int cnt_buckets) {
                std::vector<std::optional<int>> buckets(cnt_buckets);
            for (int v: numbers) {
                int index = hash.GetHash(v) % cnt_buckets;
                buckets[index].emplace(v);
            }
            return buckets;
        }

    public:
        InnerSet() = default;

        void Initialize(const std::vector<int>& numbers, GenerateLinearHashFunction& generator) {
            int cnt_number = numbers.size();
            int cnt_buckets = cnt_number * cnt_number;
            if (cnt_buckets == 0) {
                return;
            }
            // X % 0 - is UB
            hash_ = GetHashFunction(
                cnt_buckets, numbers,
                [](const std::vector<int>& lens) {
                    return std::all_of(lens.begin(), lens.end(),
                        [](int element) { return element <= 1; });
                }, generator);
            data_ = Split(
                numbers,
                hash_.value(),
                cnt_buckets);
        }

        bool Contains(int number) const {
            int cnt_buckets = data_.size();
            if (cnt_buckets == 0) {
                return false;
            }
            int index = hash_.value().GetHash(number) % cnt_buckets;
            if (data_[index].has_value()) {
                return data_[index].value() == number;
            } else {
                return false;
            }
        }
    };

    std::optional<LinearHashFunction> hash_;
    std::vector<InnerSet> data_;
    static const int kMaxCountRun = 1000;

    template<typename Predicate>
    static LinearHashFunction GetHashFunction(int cnt_buckets,
                                              const std::vector<int>& numbers,
                                              Predicate predicat,
                                              GenerateLinearHashFunction& generator) {
        int count_run = 0;
        while (count_run < kMaxCountRun) {
            count_run++;
            std::vector<int> lens(cnt_buckets, 0);
            auto hash = generator.Generate();
            for (int v: numbers) {
                lens[hash.GetHash(v) % cnt_buckets] += 1;
            }
            if (predicat(lens)) {
                return hash;
            }
        }
        throw BadHashFunctionException("Bad hash function");
    }

    static int SquereSum(const std::vector<int>& lens) {
        int hu = 0;
        for (int x: lens) {
            hu += x * x;
        }
        return hu;
    }

    static std::vector<std::vector<int>> Split(
        const std::vector<int>& numbers,
        LinearHashFunction& hash,
        int cnt_buckets) {
        std::vector<std::vector<int>> buckets(cnt_buckets, std::vector<int>());
        for (int v: numbers) {
            int index = hash.GetHash(v) % cnt_buckets;
            buckets[index].push_back(v);
        }
        return buckets;
    }

    std::vector<InnerSet> InitBuckets(
        const std::vector<std::vector<int>>& buckets, GenerateLinearHashFunction& generator) {
        std::vector<InnerSet> data;
        for (const auto& bucket: buckets) {
            data.emplace_back();
            data.back().Initialize(bucket, generator);
        }
        return data;
    }

public:
    FixedSet() = default;

    void Initialize(const std::vector<int>& numbers) {
        GenerateLinearHashFunction generator = GenerateLinearHashFunction();

        data_.clear();

        int cnt_number = numbers.size();
        int cnt_buckets = cnt_number;
        if (cnt_buckets == 0) {
            return;
        }
        // X % 0 - is UB
        hash_ = GetHashFunction(
            cnt_buckets,
            numbers,
            [](const std::vector<int>& lens) {
                return SquereSum(lens) <= static_cast<int>(2 * lens.size());
            },
            generator);
        auto buckets = Split(
            numbers,
            hash_.value(),
            cnt_buckets);
        data_ = InitBuckets(buckets, generator);
    }

    bool Contains(int number) const {
        if (data_.empty()) {
            return false;
        }
        int cnt_buckets = data_.size();
        int first = hash_.value().GetHash(number) % cnt_buckets;
        return data_[first].Contains(number);
    }
};
