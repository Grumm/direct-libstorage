#pragma once

namespace ta{

clss LenearRegression{
public:
    /* https://prog-cpp.ru/mnk/ */
    std::pair<float, float>
    calculate_linear_coeff(const std::vector<std::pair<float, WT>> &points,
            std::pair<size_t, size_t> range){
        //calculate avg
        //for each point calculate diff between it and avg -> abs()

        float sumx = 0;
        float sumx2 = 0;
        float sumy = 0;
        float sumxy = 0;

        for(size_t i = range.first; i <= range.second; i++){
            float y = points[i].first;//first only do unweighted TODO
            float x = i;
            sumx += x;
            sumx2 += x*x;
            sumy += y;
            sumxy += x*y;
        }

        float n = range.second - range.first + 1;
        float a = (n*sumxy - sumx * sumy) / (n * sumx2 - sumx * sumx);
        float b = (sumy - a * sumx) / n;

        return std::make_pair(a, b);
    }

    std::map<std::pair<float, float>, DistanceResults>
    calulcate_least_distance(const std::vector<std::pair<float, WT>> &points,
            std::pair<size_t, size_t> range, const std::pair<float, float> &lcoeff,
            const std::vector<std::pair<float, float>> &percentiles /* <percent, no more than> */
            ){
        //calculate avg
        //for each point calculate diff between it and avg -> abs()

        std::vector<float> dvect(points.size());
        std::map<std::pair<float, float>, DistanceResults> sigma;
        float a = lcoeff.first;
        float b = lcoeff.second;
        for(size_t i = range.first; i <= range.second; i++){
            /* 
                https://brilliant.org/wiki/dot-product-distance-between-point-and-a-line/
                y = ax + b;
                ax -1y +b = 0
                d(x0, y0) = |ax0 - y0 + b| / sqrt(a*a + 1)
            */
            float x0 = i;
            float y0 = points[i].first;
            float distance = abs(a * x0 - y0 + b) / sqrt(a * a + 1);

            //first only do unweighted TODO
            dvect.push_back(distance);
        }
        std::sort(dvect.begin(), dvect.end(), [](float a, float b){return a < b;});

        for(auto pc: percentiles){
            std::vector<std::pair<float, WT>> distance_points;
            for(size_t i = 0; i < dvect.size(); i++){
                float d = dvect[i];
                //float pcnt = i/dvect.size();
                float nextpcnt = (i + 1)/dvect.size();
                /* check max and ignore last */
                if(d < pc.second && nextpcnt > 1){
                    /* check percentile */
                    auto thispcnt = 1 - pc.first;
                    if(thispcnt > std::numeric_limits<float>::epsilon()){
                        if(thispcnt > nextpcnt){
                            continue;
                        }
                    }
                }

                distance_points.push_back(std::make_pair(d, 1));
            }
            float sum = std::accumulate(distance_points.begin(), distance_points.end(), 0.0, [&]
                (double a, const std::pair<float, WT> &b){
                    return a + b.first;
                });
            float avg = sum / distance_points.size();
            float avg_sq = std::accumulate(distance_points.begin(), distance_points.end(), 0.0, [&]
                (double a, const std::pair<float, WT> &b){
                    return a + (b.first * b.first);
                });
            float stdev = std::sqrt(avg_sq / distance_points.size() - avg * avg);
            auto max = std::max_element(distance_points.begin(), distance_points.end());
            auto lin_regr_coeff = calculate_linear_coeff(distance_points,
                        std::make_pair(0, distance_points.size() - 1));

            sigma[pc] = std::make_tuple(sum, avg, avg_sq, stdev, max->first, lin_regr_coeff.first * 100000);
            //sigma[pc] /= count;
        }

        return sigma;
    }

    std::pair<std::pair<float, float>, DistanceResults>
    get_linear_regr(const std::pair<size_t, size_t> &r,
            const std::vector<std::pair<float, WT>> &points,
            const std::pair<float, float> &percentile = std::pair<float, float>{0.97, 0.3}){
        auto coeff = calculate_linear_coeff(points, std::make_pair(r.first, r.second));

        auto distancemap = calulcate_least_distance(points,
            std::make_pair(r.first, r.second), coeff, std::vector{percentile});

        return std::make_pair(coeff, distancemap[percentile]);
    }
};
}