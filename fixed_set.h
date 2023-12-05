#include <iostream>
#include <vector>
#include <functional>
#include <random>
#include <cassert>

class BadHashFunctionException : public std::exception {
public:
    const char* what() const noexcept override {
        return "Bad hash function";
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

    LinearHashFunction* Generate() override {
        std::uniform_int_distribution<int> distribution(1, kPrime);
        int coefficien = distribution(gen) % (kPrime - 1) + 1;
        int bias = distribution(gen) % kPrime;
        return new LinearHashFunction(coefficien, bias);
    }
};


class FixedSet {
private:
    static LinearHashFunction* GetHashFunction(size_t cnt_buckets,
                                         const std::vector<int>&numbers,
                                         std::function<size_t(int)> fun,
                                         int criteria,
                                         GenerateLinearHashFunction* generator) {
        int cnt_run = 0;
        while (cnt_run < 10) {
            cnt_run++;
            std::vector<int> lens(cnt_buckets, 0);
            auto hash = generator->Generate();
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
        throw BadHashFunctionException();
    }

    static std::vector<std::vector<int>> Split(
        const std::vector<int>&numbers,
        LinearHashFunction* hash,
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

        explicit InnerSet(GenerateLinearHashFunction* generator) : generator(generator) {
        }

        ~InnerSet() {
            delete m_hash;
        }

        static std::vector<std::optional<int>> Split(
            const std::vector<int>&numbers,
            LinearHashFunction* hash,
            size_t cnt_buckets) {
            std::vector<std::optional<int>> buckets(cnt_buckets);
            for (int v: numbers) {
                size_t index = hash->GetHash(v) % cnt_buckets;
                buckets[index].emplace(v);
            }
            return buckets;
        }

        LinearHashFunction* m_hash;
        std::vector<std::optional<int>> m_data;
        size_t m_cnt_buckets;

        void Initialize(const std::vector<int>&numbers) {
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

    std::vector<InnerSet*> InitBuckets(
        const std::vector<std::vector<int>>&buckets) {
        std::vector<InnerSet*> data;
        for (const auto&bucket: buckets) {
            InnerSet* inner_set = new InnerSet(&generator);
            inner_set->Initialize(bucket);
            data.push_back(inner_set);
        }
        return data;
    }

public:
    LinearHashFunction* m_hash;
    std::vector<InnerSet*> m_data;
    size_t m_cnt_buckets;
    GenerateLinearHashFunction generator;

    FixedSet(): generator(GenerateLinearHashFunction()) {
    }

    ~FixedSet() {
        delete m_hash;

        std::for_each(m_data.begin(), m_data.end(), [](InnerSet* inner_set_ptr) {
            delete inner_set_ptr;
        });
        m_data.clear();
    }

    void Initialize(const std::vector<int>&numbers) {
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
        return m_data[first]->Contains(number);
    }
};
