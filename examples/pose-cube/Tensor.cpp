#include "Tensor.h"
#include <iostream>


Tensor::Tensor() : data_(nullptr), dtypeSize_(0), totalSize_(0) {}

Tensor::Tensor(const std::vector<size_t>& shape, size_t dtypeSize)
    : shape_(shape), dtypeSize_(dtypeSize) {
    computeStrides();
    allocateMemory();
}

Tensor::~Tensor() {
    freeMemory();
}

Tensor::Tensor(Tensor&& other) noexcept
    : data_(other.data_), shape_(std::move(other.shape_)),
      strides_(std::move(other.strides_)), dtypeSize_(other.dtypeSize_),
      totalSize_(other.totalSize_) {
    other.data_ = nullptr;
}

Tensor& Tensor::operator=(Tensor&& other) noexcept {
    if (this != &other) {
        freeMemory();
        data_ = other.data_;
        shape_ = std::move(other.shape_);
        strides_ = std::move(other.strides_);
        dtypeSize_ = other.dtypeSize_;
        totalSize_ = other.totalSize_;
        other.data_ = nullptr;
    }
    return *this;
}

void Tensor::reshape(const std::vector<size_t>& newShape) {
    shape_ = newShape;
    computeStrides();
    allocateMemory(); // Reallocation necessary if totalSize_ changes
}

void Tensor::allocateMemory() {
    freeMemory();
    totalSize_ = dtypeSize_ * std::accumulate(shape_.begin(), shape_.end(), 1, std::multiplies<size_t>());
    data_ = malloc(totalSize_);
    assert(data_); // Replace with more sophisticated error handling in production
    printf("Succesfull Malloc %zu bits", totalSize_);
    printf("Memory allocated for tensor at address: %p\n", static_cast<void*>(data_));
}

void* Tensor::getDataPointer() const {
    return data_;
}

void Tensor::freeMemory() {
    if (data_) {
        free(data_);
        data_ = nullptr;
    }
}

std::vector<size_t> Tensor::getStrides(){
    return strides_;
}
std::vector<size_t> Tensor::getShape(){
    return shape_;
}



void Tensor::checkIndices(const std::vector<size_t>& indices) const {
    if (indices.size() != shape_.size()) {
        throw std::out_of_range("Indices size does not match tensor dimension.");
    }
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] >= shape_[i]) {
            throw std::out_of_range("Index out of range.");
        }
    }
}

size_t Tensor::calculateOffset(const std::vector<size_t>& indices) const {
    return std::inner_product(indices.begin(), indices.end(), strides_.begin(), 0UL);
}

void Tensor::computeStrides() {
    strides_.resize(shape_.size());
    size_t stride = 1;
    for (auto it = shape_.rbegin(); it != shape_.rend(); ++it) {
        size_t current_dim = *it;
        strides_[it - shape_.rbegin()] = stride;
        printf("s=%d", stride);
        stride *= current_dim;
       
    }
}
