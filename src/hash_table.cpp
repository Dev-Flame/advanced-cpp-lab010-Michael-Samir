#include "project/hash_table.h"

#include <limits>
#include <stdexcept>

HashTable::HashTable(std::size_t capacity)
    : table_(capacity), size_(0) {
    if (capacity == 0) {
        throw std::invalid_argument("Hash table capacity must be greater than zero");
    }
}

HashTable::~HashTable() = default;

std::size_t HashTable::hashFunction(const std::string& key) const {
    std::size_t hash = 2166136261u;
    for (unsigned char ch : key) {
        hash ^= ch;
        hash *= 16777619u;
    }
    return hash % table_.size();
}

std::size_t HashTable::probe(const std::string& key) const {
    std::size_t index = hashFunction(key);
    std::size_t firstDeleted = capacity();

    for (std::size_t step = 0; step < capacity(); ++step) {
        const HashEntry& entry = table_[index];
        if (entry.occupied) {
            if (entry.key == key) {
                return index;
            }
        } else if (!entry.deleted) {
            return firstDeleted == capacity() ? index : firstDeleted;
        } else if (firstDeleted == capacity()) {
            // Keep searching past a tombstone in case this key is farther along.
            firstDeleted = index;
        }
        // Consecutive square offsets differ by 2 * step + 1.
        index = (index + 2 * step + 1) % capacity();
    }
    return firstDeleted;
}

bool HashTable::insert(const std::string& key, int value) {
    std::size_t index = probe(key);
    if (index != capacity() && table_[index].occupied) {
        return false;
    }

    // A prime-sized table below half full has room along each probe sequence.
    // The initial capacity can be composite, so also grow if probing ran out.
    if (size_ >= capacity() / 2 || index == capacity()) {
        rehash();
        index = probe(key);
    }
    table_[index] = HashEntry(key, value, true);
    ++size_;
    return true;
}

bool HashTable::remove(const std::string& key) {
    const std::size_t index = probe(key);
    if (index == capacity() || !table_[index].occupied) {
        return false;
    }
    table_[index].key.clear();
    table_[index].occupied = false;
    table_[index].deleted = true;
    --size_;
    return true;
}

bool HashTable::contains(const std::string& key) const {
    const std::size_t index = probe(key);
    return index != capacity() && table_[index].occupied;
}

int HashTable::get(const std::string& key) const {
    const std::size_t index = probe(key);
    if (index == capacity() || !table_[index].occupied) {
        throw std::out_of_range("Key not found");
    }
    return table_[index].value;
}

bool HashTable::update(const std::string& key, int newValue) {
    const std::size_t index = probe(key);
    if (index == capacity() || !table_[index].occupied) {
        return false;
    }
    table_[index].value = newValue;
    return true;
}

std::size_t HashTable::size() const noexcept {
    return size_;
}

std::size_t HashTable::capacity() const noexcept {
    return table_.size();
}

bool HashTable::empty() const noexcept {
    return size_ == 0;
}

double HashTable::loadFactor() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(table_.size());
}

std::vector<std::string> HashTable::keys() const {
    std::vector<std::string> result;
    for (const auto& entry : table_) {
        if (entry.occupied && !entry.deleted) {
            result.push_back(entry.key);
        }
    }
    return result;
}

std::vector<int> HashTable::values() const {
    std::vector<int> result;
    for (const auto& entry : table_) {
        if (entry.occupied && !entry.deleted) {
            result.push_back(entry.value);
        }
    }
    return result;
}

void HashTable::rehash() {
    if (capacity() > std::numeric_limits<std::size_t>::max() / 2) {
        throw std::length_error("Hash table capacity is too large");
    }
    HashTable larger(nextPrime(capacity() * 2));
    for (const auto& entry : table_) {
        if (entry.occupied) {
            larger.insert(entry.key, entry.value);
        }
    }
    table_.swap(larger.table_);
}

bool HashTable::isPrime(std::size_t value) const {
    if (value < 2) {
        return false;
    }
    if (value == 2) {
        return true;
    }
    if (value % 2 == 0) {
        return false;
    }
    for (std::size_t i = 3; i <= value / i; i += 2) {
        if (value % i == 0) {
            return false;
        }
    }
    return true;
}

std::size_t HashTable::nextPrime(std::size_t value) const {
    if (value <= 2) {
        return 2;
    }
    std::size_t candidate = value % 2 == 0 ? value + 1 : value;
    while (!isPrime(candidate)) {
        if (candidate > std::numeric_limits<std::size_t>::max() - 2) {
            throw std::length_error("Hash table capacity is too large");
        }
        candidate += 2;
    }
    return candidate;
}
