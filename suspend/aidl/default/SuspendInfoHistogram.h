/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_SYSTEM_SYSTEM_SUSPEND_V1_0_SUSPEND_INFO_HISTOGRAM_H
#define ANDROID_SYSTEM_SYSTEM_SUSPEND_V1_0_SUSPEND_INFO_HISTOGRAM_H

#include <vector>
#include <algorithm>

namespace android {
namespace system {
namespace suspend {
namespace V1_0 {

class SuspendInfoHistogram {
   public:
    /**
     * Constructor that takes the bin configuration.
     * @param upperBounds A vector of the exclusive upper bounds for each bin
     * (e.g. {1000, 2500} creates 3 bins: <1000, 1000-2499, >=2500).
     */
    SuspendInfoHistogram(const std::vector<int64_t>& upperBounds)
        : mBinUpperBounds(getSortedCopy(upperBounds)) {
        // The number of bins is always 1 more than the number of boundaries.
        mBinCounts.resize(mBinUpperBounds.size() + 1, 0);
    }

    void addSample(int64_t sample) {
        size_t binIndex = getSampleBinIndex(sample);
        mBinCounts[binIndex]++;
    }

    /**
     * Returns a const reference to the current bin counts.
     * @return A const std::vector<int64_t>& containing the count for each bin.
     */
    const std::vector<int64_t>& getBinCounts() const { return mBinCounts; }

   private:
    static std::vector<int64_t> getSortedCopy(const std::vector<int64_t>& values) {
        auto sortedValues = values;
        std::sort(sortedValues.begin(), sortedValues.end());
        return sortedValues;
    }

    // Holds the boundaries for this specific histogram instance
    const std::vector<int64_t> mBinUpperBounds;

    // Holds the counts for this instance
    std::vector<int64_t> mBinCounts;

    size_t getSampleBinIndex(int64_t sample) const {
        // For the expected small number of bins (<= 10)
        // a linear scan outperforms a binary search.
        for (size_t i = 0; i < mBinUpperBounds.size(); ++i) {
            if (mBinUpperBounds[i] > sample) {
                return i;
            }
        }
        return mBinUpperBounds.size();
    }
};

}  // namespace V1_0
}  // namespace suspend
}  // namespace system
}  // namespace android

#endif  // ANDROID_SYSTEM_SYSTEM_SUSPEND_V1_0_SUSPEND_INFO_HISTOGRAM_H
