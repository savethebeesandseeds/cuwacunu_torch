#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <map>

// Helper namespace for mathematical functions
namespace kdemath {

    // Calculate variance of a vector
    template <typename T>
    T variance(const std::vector<T>& data) {
        T mean = std::accumulate(data.begin(), data.end(), T(0)) / data.size();
        T var = std::accumulate(data.begin(), data.end(), T(0), [mean](T sum, T x) {
            return sum + (x - mean) * (x - mean);
        }) / (data.size() - 1);
        return var;
    }

    // Calculate standard deviation of a vector
    template <typename T>
    T std_dev(const std::vector<T>& data) {
        return std::sqrt(variance(data));
    }

    // Return the value of pi
    template <typename T>
    constexpr T pi() {
        return T(3.14159265358979323846);
    }
}

// EntropyCalculatorKDE class
class EntropyCalculatorKDE {
public:
    // Constructor with optional bandwidth method ("scott" or "silverman")
    EntropyCalculatorKDE(const std::string& bandwidth_method = "scott")
        : totalDataPoints(0), bandwidth(0.0), bandwidthMethod(bandwidth_method) {}

    // Add data in batch
    void addData(const std::vector<double>& data) {
        if (data.size() < 2) {
            throw std::invalid_argument("Need at least two data points.");
        }
        dataPoints.insert(dataPoints.end(), data.begin(), data.end());
        totalDataPoints = dataPoints.size();

        // Update bandwidth using selected method
        updateBandwidth();
        precomputeConstants();
    }

    // Add data incrementally
    void addDataPoint(double dataPoint) {
        dataPoints.push_back(dataPoint);
        totalDataPoints = dataPoints.size();

        if (totalDataPoints < 2) {
            throw std::invalid_argument("Need at least two data points.");
        }

        // Update bandwidth using selected method
        updateBandwidth();
        precomputeConstants();
    }

    // Compute entropy
    double computeEntropy() {
        if (totalDataPoints == 0) {
            return 0.0;
        }

        // Estimate the density at each data point
        double entropy = 0.0;
        for (const auto& x : dataPoints) {
            double density = evaluateKernelDensity(x);
            if (density > 0) {
                entropy -= std::log(density);
            }
        }
        entropy /= totalDataPoints;
        entropy += std::log2(std::exp(1)); // Convert from natural log to log2

        return entropy;
    }

    // Compute surprise value for a data point
    double computeSurprise(double dataPoint) {
        if (totalDataPoints == 0) {
            return std::numeric_limits<double>::infinity();
        }

        double density = evaluateKernelDensity(dataPoint);
        if (density > 0) {
            return -std::log2(density);
        } else {
            return std::numeric_limits<double>::infinity();
        }
    }

    // Reset the calculator
    void reset() {
        dataPoints.clear();
        totalDataPoints = 0;
        bandwidth = 0.0;
        precomputedFactor = 0.0;
    }

private:
    std::vector<double> dataPoints;
    size_t totalDataPoints;
    double bandwidth;
    std::string bandwidthMethod;
    double precomputedFactor; // Precomputed constant factor for kernel

    // Update the bandwidth using Scott's or Silverman's rule
    void updateBandwidth() {
        if (totalDataPoints < 2) {
            bandwidth = 1.0;
            return;
        }

        double stdDev = kdemath::std_dev(dataPoints);
        if (stdDev == 0.0) {
            throw std::runtime_error("Standard deviation is zero; all data points are identical.");
        }

        if (bandwidthMethod == "silverman") {
            // Silverman's rule of thumb
            bandwidth = 1.06 * stdDev * std::pow(static_cast<double>(totalDataPoints), -0.2);
        } else {
            // Default to Scott's rule
            bandwidth = stdDev * std::pow(static_cast<double>(totalDataPoints), -1.0 / 5.0);
        }

        if (bandwidth <= 0) {
            throw std::runtime_error("Calculated bandwidth is non-positive.");
        }
    }

    // Precompute constants for efficiency
    void precomputeConstants() {
        precomputedFactor = 1.0 / (bandwidth * std::sqrt(2 * kdemath::pi<double>()));
    }

    // Evaluate the kernel density estimate at a given point
    double evaluateKernelDensity(double x) const {
        double sum = 0.0;
        for (const auto& xi : dataPoints) {
            double u = (x - xi) / bandwidth;
            sum += std::exp(-0.5 * u * u);
        }
        double density = precomputedFactor * sum / totalDataPoints;
        return density;
    }
};


// Function to compute permutation entropy
double computePermutationEntropy(const std::vector<double>& data, int m, int tau) {
    if ((int)data.size() < m * tau) return 0.0;

    std::map<std::vector<int>, int> patternCounts;
    int n = data.size();

    for (int i = 0; i <= n - m * tau; ++i) {
        // Extract the sequence
        std::vector<double> window;
        for (int j = 0; j < m; ++j) {
            window.push_back(data[i + j * tau]);
        }

        // Get the permutation pattern
        std::vector<int> pattern(m);
        std::vector<size_t> indices(m);
        for (int k = 0; k < m; ++k) indices[k] = k;

        std::sort(indices.begin(), indices.end(),
            [&window](size_t a, size_t b) { return window[a] < window[b]; });

        for (int k = 0; k < m; ++k) {
            pattern[k] = indices[k];
        }

        // Count the pattern
        patternCounts[pattern]++;
    }

    // Compute probabilities
    double totalPatterns = patternCounts.size();
    std::vector<double> probabilities;
    for (const auto& pair : patternCounts) {
        probabilities.push_back(static_cast<double>(pair.second) / totalPatterns);
    }

    // Compute entropy
    double entropy = 0.0;
    for (const auto& p : probabilities) {
        entropy -= p * std::log2(p);
    }

    return entropy;
}

#include "piaabo/dutils.h"
// Example usage
int main() {
    try {
        // Create an instance of EntropyCalculatorKDE with Scott's rule
        EntropyCalculatorKDE calculator("scott");

        // Add data in batch
        std::vector<double> dataBatch = {
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0
        };
        calculator.addData(dataBatch);

        // Add data incrementally
        // calculator.addDataPoint(5.0);
        // calculator.addDataPoint(5.5);

        // Compute entropy
        TICK(time_entropy);
        double entropy = calculator.computeEntropy();
        PRINT_TOCK_ns(time_entropy);
        std::cout << "Entropy: " << entropy << std::endl;

        // Compute permutation entropy
        int m = 10;    // Embedding dimension
        int tau = 1;  // Time delay
        TICK(time_permutation_entropy);
        double pe = computePermutationEntropy(dataBatch, m, tau);
        PRINT_TOCK_ns(time_permutation_entropy);
        std::cout << "Permutation Entropy: " << pe << std::endl;

        // Compute surprise value for specific data points
        TICK(time_surprice);
        double surprise1 = calculator.computeSurprise(4.0);
        PRINT_TOCK_ns(time_surprice);
        std::cout << "Surprise value for 4.0: " << surprise1 << std::endl;

        double surprise2 = calculator.computeSurprise(5.0);
        std::cout << "Surprise value for 5.0: " << surprise2 << std::endl;

        double surprise3 = calculator.computeSurprise(6.0);
        std::cout << "Surprise value for 6.0 (unseen data): " << surprise3 << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}