#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>
#include <time.h>
using namespace std;

namespace hashes
{

  const uint32_t LARGE_PRIME = 2147483647; // largest prime less than 2^31

  class key_exception
  {
  };

  // One entry in a dictionary.
  template <typename T>
  class entry
  {
  public:
    entry() {}
    entry(uint32_t key, T &&value) noexcept
        : key_(key),
          value_(value) {}

    uint32_t key() const noexcept { return key_; }

    const T &value() const noexcept { return value_; }
    T &value() noexcept { return value_; }
    void set_value(T &&value) noexcept { value_ = value; }

  private:
    uint32_t key_;
    T value_;
  };

  // Abstract base class for hash functions.
  class abstract_hash_func
  {
  public:
    virtual ~abstract_hash_func() {}

    // Evaluate the hash function for the given key.
    virtual uint32_t hash(uint32_t key) const noexcept = 0;
  };

  // Order-2 polynomial, i.e.
  // h(x) = a0 + a1*x
  class poly2_hash_func : public abstract_hash_func
  {
  public:
    poly2_hash_func() noexcept
    {
      a0 = rand() % (LARGE_PRIME - 1);
      a1 = rand() % (LARGE_PRIME - 1);
    }

    virtual uint32_t hash(uint32_t key) const noexcept
    {
      return a0 + key * a1;
    }

  private:
    int a0, a1;
  };

  // Order-5 polynomial, i.e.
  // h(x) = a0 + a1*x + a2*x^2 + a3*x^3 + a4*x^4
  class poly5_hash_func : public abstract_hash_func
  {
  public:
    poly5_hash_func() noexcept
    {
      a.resize(5);
      for (int i = 0; i < 5; i++)
      {
        a[i] = rand() % (LARGE_PRIME - 1);
      }
    }

    virtual uint32_t hash(uint32_t key) const noexcept
    {
      return a[0] + key * (a[1] + key * (a[2] + key * (a[3] + key * a[4])));
    }

  private:
    vector<int> a;
  };

  // Tabular-hash function, i.e. (4) 256-element arrays whose elements
  // are XORed together.
  class tabular_hash_func : public abstract_hash_func
  {
  public:
    tabular_hash_func() noexcept
    {
      t.resize(4, vector<int>(256));
      for (int i = 0; i < 4; i++)
      {
        for (int j = 0; j < 256; j++)
        {
          t[i][j] = rand() % (LARGE_PRIME - 1);
        }
      }
    }

    virtual uint32_t hash(uint32_t key) const noexcept
    {
      uint32_t hashValue = 0;
      for (int i = 0; i < 4; i++)
      {
        hashValue ^= t[i][(key >> i * 8) & 0xFF];
      }
      return hashValue;
    }

  private:
    vector<vector<int> > t;
  };

  // Abstract base class for a dictionary (hash table).
  template <typename T>
  class abstract_dict
  {
  public:
    virtual ~abstract_dict() {}

    // Search for the entry matching key, and return a reference to the
    // corresponding value.
    //
    // Throw std::out_of_range if there is no such key.
    //
    // (It would be better C++ practice to also provide a const overload of
    // this function, but that seems like busy-work for this experimental
    // project, so we're skipping that.)
    virtual T &search(uint32_t key) = 0;

    // Assign key to be associated with val. If key is already in the dictionary,
    // replace that association.
    //
    // Throw std::length_error if the dictionary is too full to add another
    // entry.
    virtual void set(uint32_t key, T &&val) = 0;
  };

  // Naive dictionary (unsorted vector).
  template <typename T>
  class naive_dict : public abstract_dict<T>
  {
  public:
    // Create an empty dictionary, with the given capacity.
    naive_dict(size_t capacity)
    {
    }

    virtual T &search(uint32_t key)
    {
      auto iter = search_iterator(key);
      if (iter != entries_.end())
      {
        return iter->value();
      }
      else
      {
        throw std::out_of_range("key absent in naive_dict::search");
      }
    }

    virtual void set(uint32_t key, T &&val)
    {
      auto iter = search_iterator(key);
      if (iter != entries_.end())
      {
        iter->set_value(std::move(val));
      }
      else
      {
        entries_.emplace_back(key, std::move(val));
      }
    }

  private:
    std::vector<entry<T> > entries_;

    typename std::vector<entry<T> >::iterator search_iterator(uint32_t key)
    {
      return std::find_if(entries_.begin(),
                          entries_.end(),
                          [&](entry<T> &entry)
                          { return entry.key() == key; });
    }
  };

  template <typename T>
  class chain_dict : public abstract_dict<T>
  {
  public:
    // Create an empty dictionary, with the given capacity.
    chain_dict(size_t capacity)
    {
      entries.resize(capacity);
      capacity_ = capacity;
    }

    virtual T &search(uint32_t key)
    {
      uint32_t index = hashFunction.hash(key) % capacity_;
      for (int j = 0; j < entries[index].size(); j++)
      {
        if (entries[index][j].key() == key)
          return entries[index][j].value();
      }
      throw std::out_of_range("key absent in chain_dict::search");
    }

    virtual void set(uint32_t key, T &&val)
    {
      uint32_t index = hashFunction.hash(key) % capacity_;
      entries[index].emplace_back(key, std::move(val));
    }

  private:
    vector<vector<entry<T> > > entries;
    poly2_hash_func hashFunction;
    int capacity_;
  };

  // Hash table with linear probing (LP).
  template <typename T>
  class lp_dict : public abstract_dict<T>
  {
  public:
    // Create an empty dictionary, with the given capacity.
    lp_dict(size_t capacity)
    {
      entries.resize(2 * capacity, NULL);
      capacity_ = 2 * capacity;
    }

    virtual T &search(uint32_t key)
    {
      int index = hashFunction.hash(key) % capacity_;
      while (entries[index] != NULL && entries[index]->key() != key)
      {
        index = (index + 1) % capacity_;
      }
      if (entries[index] != NULL && entries[index]->key() == key)
      {
        return entries[index]->value();
      }
      throw std::out_of_range("key absent in lp_dict::search");
    }

    virtual void set(uint32_t key, T &&val)
    {
      int index = hashFunction.hash(key) % capacity_;
      while (entries[index] != NULL && entries[index]->key() != key)
      {
        index = (index + 1) % capacity_;
      }
      entries[index] = new entry<T>(key, std::move(val));
      return;
    }

  private:
    vector<entry<T>*> entries;
    poly5_hash_func hashFunction;
    int capacity_;
  };

  // Cuckoo hash table.
  template <typename T>
  class cuckoo_dict : public abstract_dict<T>
  {
  public:
    // Create an empty dictionary, with the given capacity.
    cuckoo_dict(size_t capacity)
    {
      tables.resize(2, vector<entry<T>*>(capacity, NULL));
      hashFunctions = new tabular_hash_func[2];
      capacity_ = capacity;
    }

    virtual T &search(uint32_t key)
    {
      int index;
      for(int i = 0; i < 2; i++){
        index = hashFunctions[i].hash(key) % capacity_;
        if(tables[i][index] != NULL && tables[i][index]->key() == key)
          return tables[i][index]->value();
      }
      throw std::out_of_range("key absent in cuckoo_dict::search");
    }

    virtual void set(uint32_t key, T &&val)
    {
      int i = 0, index, temp = key, key_, val_;
      while(true){
        index = hashFunctions[i].hash(key) % capacity_;
        if(tables[i][index] == NULL){
          tables[i][index] = new entry<T>(key, std::move(val));
          break;
        }else{
          //swapping (k,v) and table[h(k)]
          key_ = tables[i][index]->key(), val_ = tables[i][index]->value();
          tables[i][index] = new entry<T>(key, std::move(val));
          key = key_, val = val_;
        }
        //loop detection
        if(key == temp && i == 1){
          rehash(tables, key, val);
          return;
        }
        i = 1 - i;
      }
    }

  private:
    vector< vector<entry<T>*> > tables;
    tabular_hash_func *hashFunctions;
    int capacity_;

    void rehash(vector< vector<entry<T>*>>& hashTable, uint32_t key, uint32_t val){
      hashFunctions = new tabular_hash_func[2];
        vector<entry<T>*> previousKeys;
        //save previously inserted (k,v) and empty the table
        for(int i = 0; i < 2; i++){
          for(int j = 0; j < capacity_; j++){
            if(hashTable[i][j] != NULL){
                previousKeys.push_back(hashTable[i][j]);
                hashTable[i][j] = NULL;
            } 
          }
        }
        previousKeys.push_back(new entry<T>(key, std::move(val)));
        for(int i = 0; i < previousKeys.size(); i++)
        set(previousKeys[i]->key(), std::move(previousKeys[i]->value()));
    }
  };
}
