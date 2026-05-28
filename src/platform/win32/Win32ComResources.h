#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>

namespace ovtr::win32 {

class OleInitializer {
public:
    OleInitializer() noexcept
        : result_(OleInitialize(nullptr))
    {
    }

    ~OleInitializer()
    {
        if (SUCCEEDED(result_)) {
            OleUninitialize();
        }
    }

    OleInitializer(const OleInitializer&) = delete;
    OleInitializer& operator=(const OleInitializer&) = delete;

private:
    HRESULT result_ = E_FAIL;
};

class CoTaskMemItemList {
public:
    explicit CoTaskMemItemList(PIDLIST_ABSOLUTE itemList = nullptr) noexcept
        : itemList_(itemList)
    {
    }

    ~CoTaskMemItemList()
    {
        reset();
    }

    CoTaskMemItemList(const CoTaskMemItemList&) = delete;
    CoTaskMemItemList& operator=(const CoTaskMemItemList&) = delete;

    PIDLIST_ABSOLUTE get() const noexcept
    {
        return itemList_;
    }

    void reset(PIDLIST_ABSOLUTE itemList = nullptr) noexcept
    {
        if (itemList_ && itemList_ != itemList) {
            CoTaskMemFree(itemList_);
        }
        itemList_ = itemList;
    }

private:
    PIDLIST_ABSOLUTE itemList_ = nullptr;
};

} // namespace ovtr::win32
