#pragma once

/* 
We need  algos:
    1) detect trend
        - flat
        - up/down with steepness degree
        - within time range find different trend regions(with varying t.)
    considering
        - price range
        - timeframe

    2) detect volume spikes
        - summarise over few small lots
        - detect slow growth of volume over time in huge lots

    3) detect volatility
        - at current time range
        - within time range find and separate volatility regions(with different v.)

    4) range detector
        - detects ranges with different parameters, such as 
            volatility, trend, volume, price levels

    5) Volume Profile
*/

#include "TrendsDetector.hpp"

#include "VolumeProfile.hpp"