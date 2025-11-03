#include <gtest/gtest.h>
#include <vector>
#include "SuspendInfoHistogram.h"

using android::system::suspend::V1_0::SuspendInfoHistogram;

TEST(SuspendInfoHistogramTest, BasicTest) {
    SuspendInfoHistogram hist({10, 20, 30});
    // (<10), [10, 20), [20, 30), (>=30)

    hist.addSample(5);   // Bin 0 (<10)
    hist.addSample(9);   // Bin 0 (<10)
    hist.addSample(15);  // Bin 1 [10, 20)
    hist.addSample(25);  // Bin 2 [20, 30)
    hist.addSample(28);  // Bin 2 [20, 30)
    hist.addSample(28);  // Bin 2 [20, 30)
    hist.addSample(29);  // Bin 2 [20, 30)
    hist.addSample(35);  // Bin 3 (>=30)
    hist.addSample(40);  // Bin 3 (>=30)
    hist.addSample(100); // Bin 3 (>=30)

    const auto& counts = hist.getBinCounts();
    ASSERT_EQ(counts.size(), 4);
    EXPECT_EQ(counts[0], 2);  // Samples 5, 9
    EXPECT_EQ(counts[1], 1);  // Sample 15
    EXPECT_EQ(counts[2], 4);  // Sample 25, 28, 28, 29
    EXPECT_EQ(counts[3], 3);  // Samples 35, 40, 100
}

TEST(SuspendInfoHistogramTest, BasicTestUnsorted) {
    SuspendInfoHistogram hist({20, 10, 30});
    // (<10), [10, 20), [20, 30), (>=30)

    hist.addSample(5);   // Bin 0 (<10)
    hist.addSample(9);   // Bin 0 (<10)
    hist.addSample(15);  // Bin 1 [10, 20)
    hist.addSample(25);  // Bin 2 [20, 30)
    hist.addSample(28);  // Bin 2 [20, 30)
    hist.addSample(28);  // Bin 2 [20, 30)
    hist.addSample(29);  // Bin 2 [20, 30)
    hist.addSample(35);  // Bin 3 (>=30)
    hist.addSample(40);  // Bin 3 (>=30)
    hist.addSample(100); // Bin 3 (>=30)

    const auto& counts = hist.getBinCounts();
    ASSERT_EQ(counts.size(), 4);
    EXPECT_EQ(counts[0], 2);  // Samples 5, 9
    EXPECT_EQ(counts[1], 1);  // Sample 15
    EXPECT_EQ(counts[2], 4);  // Sample 25, 28, 28, 29
    EXPECT_EQ(counts[3], 3);  // Samples 35, 40, 100
}

TEST(SuspendInfoHistogramTest, SampleOnBoundary) {
    SuspendInfoHistogram hist({10, 15, 20, 25});
    // (<10), [10, 15), [15, 20), [20, 25), (>=25)

    hist.addSample(10);  // Bin 1 [10, 15)
    hist.addSample(10);  // Bin 1 [10, 15)
    hist.addSample(20);  // Bin 3 [20, 25)
    hist.addSample(15);  // Bin 2 [15, 20)
    hist.addSample(15);  // Bin 2 [15, 20)
    hist.addSample(15);  // Bin 2 [15, 20)
    hist.addSample(25);  // Bin 4 (>=25)
    hist.addSample(25);  // Bin 4 (>=25)
    hist.addSample(25);  // Bin 4 (>=25)
    hist.addSample(25);  // Bin 4 (>=25)


    const auto& counts = hist.getBinCounts();
    ASSERT_EQ(counts.size(), 5);
    EXPECT_EQ(counts[0], 0);
    EXPECT_EQ(counts[1], 2);
    EXPECT_EQ(counts[2], 3);
    EXPECT_EQ(counts[3], 1);
    EXPECT_EQ(counts[4], 4);
}

TEST(SuspendInfoHistogramTest, NoBounds) {
    SuspendInfoHistogram hist({});

    hist.addSample(0);
    hist.addSample(100);
    hist.addSample(1000);

    const auto& counts = hist.getBinCounts();
    ASSERT_EQ(counts.size(), 1);
    EXPECT_EQ(counts[0], 3);
}


TEST(SuspendInfoHistogramTest, SingleBound) {
    SuspendInfoHistogram hist({10});
    // (<10), (>=10)

    hist.addSample(5);   // Bin 0 (<10)
    hist.addSample(1);   // Bin 0 (<10)
    hist.addSample(9);   // Bin 0 (<10)
    hist.addSample(3);   // Bin 0 (<10)
    hist.addSample(10);  // Bin 1 (>=10)
    hist.addSample(100); // Bin 1 (>=10)

    const auto& counts = hist.getBinCounts();
    ASSERT_EQ(counts.size(), 2);
    EXPECT_EQ(counts[0], 4);  // Samples 1, 3, 5, 9
    EXPECT_EQ(counts[1], 2);  // Samples 10, 100
}