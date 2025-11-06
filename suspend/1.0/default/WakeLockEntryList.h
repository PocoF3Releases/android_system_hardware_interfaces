/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H
#define ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H

#include <android-base/unique_fd.h>
#include <android/system/suspend/internal/WakeLockInfo.h>
#include <utils/Mutex.h>

#include <list>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

using ::android::system::suspend::internal::WakeLockInfo;

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

using android::base::unique_fd;
using TimestampType = int64_t;

/*
 * WakeLockEntryList to collect wake lock stats.
 * This class is thread safe.
 */
class WakeLockEntryList {
   public:
    WakeLockEntryList(size_t capacity, unique_fd kernelWakelockStatsFd);
    void updateOnAcquire(const std::string& name, int pid);
    void updateOnRelease(const std::string& name, int pid);
    // updateNow() should be called before getWakeLockStats() to ensure stats are
    // updated wrt the current time.
    void updateNow();
    void getWakeLockStats(int wakeLockInfoFieldBitMask,
                          std::vector<WakeLockInfo>* aidl_return) const;
    int nrActiveHolders(const std::string& name, int pid);
    void clear();
    friend std::ostream& operator<<(std::ostream& out, const WakeLockEntryList& list);

   private:
    void evictIfFull() REQUIRES(mLock);
    void insertEntry(WakeLockInfo entry) REQUIRES(mLock);
    void deleteEntry(std::list<WakeLockInfo>::iterator entry) REQUIRES(mLock);
    void updateEntry(WakeLockInfo updatedEntry,
                     std::list<WakeLockInfo>::iterator staleEntry)
            REQUIRES(mLock);
    WakeLockInfo createNativeEntry(const std::string& name, int pid, TimestampType timeNow) const;
    WakeLockInfo createKernelEntry(const std::string& name) const;

    // Used by createKernelEntry to reduce heap churn on successive calls.
    struct ScratchSpace {
        static constexpr const int BUFF_SIZE = 1024;
        char readBuff[BUFF_SIZE];
        std::string statName, valStr;
        ScratchSpace() {
            valStr.reserve(BUFF_SIZE);
            statName.reserve(BUFF_SIZE);
        }
    };
    WakeLockInfo createKernelEntry(ScratchSpace* ss, int wakeLockInfoFieldBitMask,
                                   const std::string& name) const;

    void getKernelWakelockStats(int wakeLockInfoFieldBitMask,
                                std::vector<WakeLockInfo>* aidl_return) const;

    // Hash for WakeLockEntry key (pair<std::string, int>)
    struct LockHash {
        std::size_t operator()(const std::pair<std::string, int>& key) const {
            return std::hash<std::string>()(key.first) ^ std::hash<int>()(key.second);
        }
    };

    mutable std::mutex mLock;

    size_t mCapacity;
    unique_fd mKernelWakelockStatsFd;

    class FdWithMutex {
       public:
        FdWithMutex(unique_fd fd) : mFd(std::move(fd)) {}

        // RAII helper class that guarantees that all FD access happens under a lock.
        //
        // mFdWithMutex remains valid throughout because
        // (a) elements are never erased elements from mFdWithMutexCache and
        // (b) if mFdWithMutexCache gets rebucketed, that does not invalidate
        //     references (and pointers) to values.
        class LockedFd {
           public:
            int get() const { return mFdWithMutex->mFd; }
            void set(unique_fd&& newFd) { mFdWithMutex->mFd = std::move(newFd); }

            bool isValid() const { return mFdWithMutex->mFd != -1; }
            void invalidate() { mFdWithMutex->mFd.reset(); }

            // Need move ctor and assignment for std::optional<LockedFd>.
            LockedFd(LockedFd&& other) {
                mFdWithMutex = other.mFdWithMutex;
                other.mFdWithMutex = nullptr;

                mLocked = std::move(other.mLocked);
            }

            LockedFd& operator=(LockedFd&& other) {
                mFdWithMutex = other.mFdWithMutex;
                other.mFdWithMutex = nullptr;

                mLocked = std::move(other.mLocked);
                return *this;
            }

           private:
            FdWithMutex* mFdWithMutex;
            std::unique_lock<std::mutex> mLocked;

            LockedFd(FdWithMutex* fdWithMutex)
                : mFdWithMutex(fdWithMutex), mLocked(fdWithMutex->mLock) {};

            friend class FdWithMutex;
        };

        LockedFd acquire() { return LockedFd(this); }

       private:
        std::mutex mLock;
        unique_fd mFd;
    };

    int openFDAndUpdateCache(const std::string& statName, const std::string& kwlId,
                             std::optional<FdWithMutex::LockedFd>* acquiredFd) const
        REQUIRES(mLock);

    mutable std::unordered_map<std::string, unique_fd> mFdCache GUARDED_BY(mLock);
    mutable std::unordered_map<std::string, FdWithMutex> mFdWithMutexCache GUARDED_BY(mLock);

    // std::list and std::unordered map are used to support both inserting a stat
    // and eviction of the LRU stat in O(1) time. The LRU stat is maintained at
    // the back of the list.
    std::list<WakeLockInfo> mStats GUARDED_BY(mLock);
    std::unordered_map<std::pair<std::string, int>, std::list<WakeLockInfo>::iterator, LockHash>
        mLookupTable GUARDED_BY(mLock);
    // Map to track the live hold count for each wakelock.
    std::unordered_map<std::pair<std::string, int>, int, LockHash> mNrActiveHolders GUARDED_BY(mLock);
};

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android

#endif  // ANDROID_SYSTEM_SUSPEND_WAKE_LOCK_ENTRY_LIST_H
