#include <vector>
#include <functional>
#include <random>
#include <cassert>
#include <optional>
#include <string>


class BadHashFunctionException : public std::exception {
public:
    std::string errorMessage;
    explicit BadHashFunctionException(const std::string& message) : errorMessage(message) {}

    const char* what() const noexcept override {
        return errorMessage.c_str();
    }
};

class HashFunction {
public:
    virtual size_t GetHash(int value) const = 0;
    virtual ~HashFunction() {}
};

class LinearHashFunction : public HashFunction {
public:
    int m_coefficien;
    int m_bias;
    static const size_t kPrime = 1000000021;
    LinearHashFunction(int coefficien, int bias) : m_coefficien(coefficien), m_bias(bias) {}

    ~LinearHashFunction() override = default;

    size_t GetHash(int value) const override {
        return static_cast<size_t>((1LL * value * m_coefficien + m_bias) % kPrime);
    }
};

class GenerateHashFunction {
public:
    virtual HashFunction* Generate() = 0;
};

class GenerateLinearHashFunction : public GenerateHashFunction {
public:
    std::mt19937 gen;
    static const size_t kPrime = 1000000021;

    GenerateLinearHashFunction() : gen(std::random_device()()) {}

    HashFunction* Generate() override {
        std::uniform_int_distribution<int> distribution(1, kPrime);
        int coefficien = distribution(gen) % (kPrime - 1) + 1;
        int bias = distribution(gen) % kPrime;
        return new LinearHashFunction(coefficien, bias);
    }
};


class FixedSet {
private:
    static HashFunction* GetHashFunction(size_t cnt_buckets,
                                         const std::vector<int>&numbers,
                                         std::function<size_t(int)> fun,
                                         int criteria,
                                         GenerateLinearHashFunction* generator) {
        int cnt_run = 0;
        while (cnt_run < 10) {
            cnt_run++;
            std::vector<int> lens(cnt_buckets, 0);
            auto* hash = generator->Generate();
            for (int v: numbers) {
                lens[hash->GetHash(v) % cnt_buckets] += 1;
            }
            int hu = 0;
            for (int x: lens) {
                hu += fun(x);
            }
            if (hu <= criteria) {
                return hash;
            }
        }
        throw BadHashFunctionException("Bad hash function");
    }

    static std::vector<std::vector<int>> Split(
        const std::vector<int>&numbers,
        HashFunction* hash,
        size_t cnt_buckets) {
        std::vector<std::vector<int>> buckets(cnt_buckets, std::vector<int>());
        for (int v: numbers) {
            size_t index = hash->GetHash(v) % cnt_buckets;
            buckets[index].push_back(v);
        }
        return buckets;
    }

    class InnerSet {
    public:
        GenerateLinearHashFunction* generator;
        HashFunction* m_hash;
        std::vector<std::optional<int>> m_data;
        size_t m_cnt_buckets;

        explicit InnerSet(GenerateLinearHashFunction* generator) :
            generator(generator),  m_hash(nullptr) {}

        ~InnerSet() {
            delete m_hash;
        }

        InnerSet(InnerSet&& other) noexcept
            : generator(other.generator), m_hash(other.m_hash),
              m_data(std::move(other.m_data)), m_cnt_buckets(other.m_cnt_buckets) {
            other.generator = nullptr;
            other.m_hash = nullptr;
            other.m_cnt_buckets = 0;
        }

        static std::vector<std::optional<int>> Split(
            const std::vector<int>&numbers,
            HashFunction* hash,
            size_t cnt_buckets) {
            std::vector<std::optional<int>> buckets(cnt_buckets);
            for (int v: numbers) {
                size_t index = hash->GetHash(v) % cnt_buckets;
                buckets[index].emplace(v);
            }
            return buckets;
        }

        void Initialize(const std::vector<int>&numbers) {
            delete m_hash;
            size_t cnt_number = numbers.size();
            m_cnt_buckets = cnt_number * cnt_number + 1;
            assert(m_cnt_buckets != 0);
            // X % 0 - is UB
            m_hash = GetHashFunction(
                m_cnt_buckets, numbers,
                [](int value) {
                    return (value <= 1) ? 0 : value;
                }, 0, generator);
            m_data = Split(
                numbers,
                m_hash,
                m_cnt_buckets);
        }

        bool Contains(int number) const {
            assert(m_cnt_buckets != 0);
            size_t index = m_hash->GetHash(number) % m_cnt_buckets;
            if (m_data[index].has_value()) {
                return m_data[index].value() == number;
            } else {
                return false;
            }
        }
    };

    std::vector<InnerSet> InitBuckets(
        const std::vector<std::vector<int>>&buckets) {
        std::vector<InnerSet> data;
        for (const auto&bucket: buckets) {
            data.emplace_back(&generator);
            data.back().Initialize(bucket);
        }
        return data;
    }

public:
    HashFunction* m_hash;
    std::vector<InnerSet> m_data;
    size_t m_cnt_buckets;
    GenerateLinearHashFunction generator;

    FixedSet(): m_hash(nullptr), m_cnt_buckets(0), generator(GenerateLinearHashFunction())  {
    }

    ~FixedSet() {
        delete m_hash;
    }

    void Initialize(const std::vector<int>&numbers) {
        delete m_hash;
        size_t cnt_number = numbers.size();
        m_cnt_buckets = cnt_number + 1;
        assert(m_cnt_buckets != 0);
        // X % 0 - is UB
        m_hash = GetHashFunction(
            m_cnt_buckets,
            numbers,
            [](int value) {
                return value * value;
            }, 2 * cnt_number,
            &generator);
        auto buckets = Split(
            numbers,
            m_hash,
            m_cnt_buckets);
        m_data = InitBuckets(buckets);
    }

    bool Contains(int number) const {
        assert(m_cnt_buckets != 0);
        size_t first = m_hash->GetHash(number) % m_cnt_buckets;
        return m_data[first].Contains(number);
    }
};
