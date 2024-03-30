// Tensor.h

#ifndef TENSOR_H
#define TENSOR_H

#include <vector>
#include <cassert>
#include <stdexcept>
#include <numeric>
#include <algorithm>

class Tensor {
public:
    // Constructors and destructor
    Tensor();
    Tensor(const std::vector<size_t>& shape, size_t dtypeSize);
    ~Tensor();
    Tensor(Tensor&& other) noexcept;
    Tensor& operator=(Tensor&& other) noexcept;

    // Member functions
    void reshape(const std::vector<size_t>& newShape);
    void allocateMemory();
    void freeMemory();
    
    // Template member functions for setting and getting values
    template <typename T>
    T getValue(const std::vector<size_t>& indices) const {
        checkIndices(indices);
        size_t offset = calculateOffset(indices);
        assert(data_ != nullptr); // Ensure the data has been allocated
        return *(reinterpret_cast<T*>(data_) + offset);
    }

    template <typename T>
    void setValue(const std::vector<size_t>& indices, T value) {
        checkIndices(indices);
        size_t offset = calculateOffset(indices);
        assert(data_ != nullptr); // Ensure the dat a has been allocated
        *(reinterpret_cast<T*>(data_) + offset) = value;
    }

    void* getDataPointer() const;
    std::vector<size_t> getStrides();
    std::vector<size_t> getShape();

private:
    void* data_;
    std::vector<size_t> shape_;
    std::vector<size_t> strides_;
    size_t dtypeSize_;
    size_t totalSize_;

    void checkIndices(const std::vector<size_t>& indices) const;
    size_t calculateOffset(const std::vector<size_t>& indices) const;
    void computeStrides();
};

#endif // TENSOR_H
