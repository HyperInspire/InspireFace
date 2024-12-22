//
//  BufferAllocator.cpp
//  MNN
//
//  Created by MNN on 2018/12/30.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include "core/BufferAllocator.hpp"
#include "core/Macro.h"

//#define DUMP_USAGE
//#define MNN_DEBUG_MEMORY
namespace MNN {
class DefaultAllocator : public BufferAllocator::Allocator {
public:
    DefaultAllocator() {
        // Do nothing
    }
    virtual ~ DefaultAllocator() {
        // Do nothing
    }
    virtual std::pair<void*, size_t> onAlloc(size_t size, size_t align) {
        return std::make_pair(MNNMemoryAllocAlign(size, MNN_MEMORY_ALIGN_DEFAULT), 0);
    }
    virtual void onRelease(std::pair<void*, size_t> ptr) {
        MNN_ASSERT(ptr.second == 0);
        MNNMemoryFreeAlign(ptr.first);
    }
};
class RecurseAllocator : public BufferAllocator::Allocator {
public:
    RecurseAllocator(BufferAllocator* parent) {
        mParent = parent;
    }
    virtual ~ RecurseAllocator() {
        // Do nothing
    }
    virtual std::pair<void*, size_t> onAlloc(size_t size, size_t align) override {
        return mParent->alloc(size, false, align);
    }
    virtual void onRelease(std::pair<void*, size_t> ptr) override {
        mParent->free(ptr);
    }
private:
    BufferAllocator* mParent;
};

std::shared_ptr<BufferAllocator::Allocator> BufferAllocator::Allocator::createDefault() {
    std::shared_ptr<BufferAllocator::Allocator> _res;
    _res.reset(new DefaultAllocator);
    return _res;
}
std::shared_ptr<BufferAllocator::Allocator> BufferAllocator::Allocator::createRecurse(BufferAllocator* parent) {
    std::shared_ptr<BufferAllocator::Allocator> _res;
    _res.reset(new RecurseAllocator(parent));
    return _res;
}

BufferAllocator::Node::~Node() {
    if (nullptr == parent.get()) {
        outside->onRelease(pointer);
    }
}
std::pair<void*, size_t> BufferAllocator::alloc(size_t size, bool separate, size_t align) {
#ifdef DUMP_USAGE
    auto memoryUsed = size / 1024.0f / 1024.0f;
    MNN_PRINT("Alloc: %f\n", memoryUsed);
#endif
    if (0 == align) {
        align = mAlign;
    }
    std::pair<void*, size_t> pointer;
    // reuse if possible
    if (!separate) {
        if (nullptr != mCurrentFreeList) {
            pointer = getFromFreeList(mCurrentFreeList, size, false, align);
        }
        if (nullptr != pointer.first) {
            return pointer;
        }
        pointer = getFromFreeList(&mFreeList, size, true, align);
        if (nullptr != pointer.first) {
            return pointer;
        }
    }

    // alloc otherwise
    pointer = mAllocator->onAlloc(size, align);
    if (nullptr == pointer.first) {
        return pointer;
    }
    mTotalSize += size;

    // save node
    SharedPtr<Node> node(new Node);
    node->size         = size;
    node->pointer      = pointer;
    mUsedList[pointer] = node;
    node->outside      = mAllocator.get();
    MNN_ASSERT(pointer.second % align == 0);

#ifdef DUMP_USAGE
    MNN_PRINT("mTotalSize: %f\n", mTotalSize / 1024.0f / 1024.0f);
#endif
    return pointer;
}

void BufferAllocator::returnMemory(FREELIST* listP, SharedPtr<Node> node, bool permitMerge) {
    auto& list = *listP;
    list.insert(std::make_pair(node->size, node));
    // update parent use count
    if (nullptr != node->parent.get() && permitMerge) {
        auto parent = node->parent;
        parent->useCount -= 1;

        // merge if all subnodes were freed
        auto needMerge = parent->useCount == 0;
        while (needMerge) {
            // collect all subnodes
            for (auto iter = list.begin(); iter != list.end();) {
                if (iter->second->parent.get() == parent.get()) {
                    iter = list.erase(iter);
                    continue;
                }
                iter++;
            }

            // do merge downside up
            list.insert(std::make_pair(parent->size, parent));
            needMerge = false;
            if (parent->parent.get() != nullptr) {
                parent = parent->parent;
                parent->useCount -= 1;
                needMerge = parent->useCount == 0;
            }
        }
    }
}

bool BufferAllocator::free(std::pair<void*, size_t> pointer) {
    // get node
    auto x = mUsedList.find(pointer);
    if (x == mUsedList.end()) {
        MNN_ASSERT(false);
        
        return false;
    }
    // mark as reusable
    auto node = x->second;
    mUsedList.erase(x);
    if (nullptr != mCurrentFreeList) {
        returnMemory(mCurrentFreeList, node, false);
    } else {
        returnMemory(&mFreeList, node);
    }
#ifdef DUMP_USAGE
    auto memoryUsed = x->second->size / 1024.0f / 1024.0f;
    MNN_PRINT("Free: %f\n", memoryUsed);
#endif
    return true;
}

void BufferAllocator::release(bool allRelease) {
    MNN_ASSERT(mGroups.empty());
    if (allRelease) {
        mUsedList.clear();
        mFreeList.clear();
        mTotalSize = 0;
        return;
    }
    for (auto f : mFreeList) {
        if (f.second->parent.get() == nullptr) {
            MNN_ASSERT(mTotalSize >= f.first);
            mTotalSize -= f.first;
        }
    }
    mFreeList.clear();
}

void BufferAllocator::barrierBegin() {
    MNN_ASSERT(mGroups.empty());
}

void BufferAllocator::barrierEnd() {
    for (auto& freeGroup : mGroups) {
        auto freeList = *freeGroup;
        for (auto& iter : freeList) {
            returnMemory(&mFreeList, iter.second);
        }
    }
    mGroups.clear();
}

void BufferAllocator::beginGroup() {
    std::shared_ptr<FREELIST> newFreeList(new FREELIST);
    mCurrentFreeList = newFreeList.get();
    mGroups.emplace_back(newFreeList);
}

void BufferAllocator::endGroup() {
    mCurrentFreeList = nullptr;
}

std::pair<void*, size_t> BufferAllocator::getFromFreeList(FREELIST* list, size_t size, bool permiteSplit, size_t align) {
#ifdef MNN_DEBUG_MEMORY
    return std::make_pair(nullptr, 0);
#endif
    size_t realSize = size;
    bool needExtraSize = mAlign % align != 0;
    if (needExtraSize) {
        realSize = size + align - 1;
    }
    // get node larger than size
    auto x = list->lower_bound(realSize);
    if (x == list->end()) {
        return std::make_pair(nullptr, 0);
    }
    // update parent use count
    auto pointer = x->second->pointer;
    // Align offset
    if (needExtraSize) {
        size_t originOffset = pointer.second;
        pointer.second = UP_DIV(originOffset, align) * align;
        realSize = size + pointer.second - originOffset;
    }
    if (permiteSplit && nullptr != x->second->parent.get()) {
        x->second->parent->useCount += 1;
    }

    // uses up all aligned space
    auto sizeAlign = UP_DIV(realSize, mAlign) * mAlign;
    if (sizeAlign >= x->first || (!permiteSplit)) {
        mUsedList.insert(std::make_pair(pointer, x->second));
        list->erase(x);
        MNN_ASSERT(pointer.second % align == 0);
        return pointer;
    }

    // split otherwise
    SharedPtr<Node> first(new Node);
    first->parent  = x->second;
    first->size    = sizeAlign;
    first->pointer = x->second->pointer;
    mUsedList.insert(std::make_pair(pointer, first));
    x->second->useCount += 1;

    SharedPtr<Node> second(new Node);
    second->parent  = x->second;
    second->size    = x->second->size - sizeAlign;
    second->pointer.first = x->second->pointer.first;
    second->pointer.second = x->second->pointer.second + sizeAlign;
    list->erase(x);
    list->insert(std::make_pair(second->size, second));
    MNN_ASSERT(pointer.second % align == 0);
    return pointer;
}
} // namespace MNN
