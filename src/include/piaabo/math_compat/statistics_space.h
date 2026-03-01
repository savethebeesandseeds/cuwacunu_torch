#pragma once
#include <deque>
#include <set>
#include <cmath>
#include <deque>
#include <vector>
#include <limits>
#include <functional>
#include "piaabo/dutils.h"

namespace cuwacunu {
/* inifinite window statistics_space */
/* statistics_space_t */
struct statistics_space_t {
  unsigned long ctx = 0;                                /* Number of data points */
  double c_max = std::numeric_limits<double>::lowest();   /* Max value */
  double c_min = std::numeric_limits<double>::max();      /* Min value */
  double c_mean = 0;                                     /* Running mean */
  double c_S = 0;                                        /* Running variance * (n-1), this is for the calculation of the sample variance */
  /* Constructor declaration */
  statistics_space_t(double initial_value);
  void  update(double x);
  double normalize(double x) const;
  double mean()              const;
  double variance()          const;
  double stddev()            const;
  double max()               const;
  double min()               const;
  unsigned long count()      const;
  /* Copy Constructor */
  statistics_space_t(const statistics_space_t& src);
};

/* rolling window statistics_space */
struct statistics_space_n_t {
  unsigned long ctx = 0;     /* Number of data points processed */
  unsigned int window_size;
  std::deque<double> window;  /* Circular buffer for last N data points */
  std::multiset<double> window_values; /* Sorted set to track min and max values efficiently */
  double sum = 0.0f;          /* Sum of data points in the window */
  double sum_sq = 0.0f;       /* Sum of squares of data points in the window */
  double c_max = std::numeric_limits<double>::lowest();  /* Max value in the window */
  double c_min = std::numeric_limits<double>::max();     /* Min value in the window */

  /* Constructor */
  statistics_space_n_t(unsigned int window_size);
  void update(double x);
  double normalize(double x) const;
  bool  ready()              const;
  double mean()              const;
  double variance()          const;
  double stddev()            const;
  double max()               const;
  double min()               const;
  unsigned long count()      const;
};

/*
 * Struct: FieldAccessor<T>
 * ------------------------
 * A utility structure that holds getter and setter functions for a specific field within a data structure of type T.
 *
 * Template Parameters:
 * - T: The data type (e.g., a struct or class) containing the fields to be accessed.
 *
 * Members:
 * - getter: A std::function that, given a const reference to T, returns the value of the field as a double.
 * - setter: A std::function that, given a reference to T and a double value, sets the field to the provided value.
 *
 * Purpose:
 * The FieldAccessor structure abstracts the access to individual fields within T, allowing the statistics_pack_t
 * to operate generically on any number of fields regardless of their types within T. This is crucial because
 * statistics_space_n_t operates on single numerical values (double), but T may contain multiple fields of
 * different types (e.g., int, float, double).
 */
template <typename T>
struct FieldAccessor {
  std::function<double(const T&)> getter;
  std::function<void(T&, double)> setter;
};

/*
 * Struct: statistics_pack_t<T>
 * ----------------------------
 * A template structure that manages rolling statistics for multiple fields within a data structure T.
 *
 * Template Parameters:
 * - T: The data type containing multiple fields to be tracked and normalized.
 *
 * Members:
 * - stats_vector: A vector of statistics_space_n_t instances, each maintaining statistics for a single field.
 * - accessors: A vector of FieldAccessor<T>, each providing getter and setter access to a field in T.
 *
 * Purpose:
 * The statistics_pack_t structure addresses the challenge of applying statistics_space_n_t (which operates on a single
 * numerical value) to multiple fields within a complex data structure T. By maintaining a vector of statistics_space_n_t
 * instances and corresponding FieldAccessor objects, statistics_pack_t can update statistics and normalize values for
 * each field individually.
 *
 * Key Functions:
 * - Constructor:
 *   Initializes the stats_vector with statistics_space_n_t instances for each field, and stores the corresponding
 *   FieldAccessor objects.
 *
 * - update(const T& data_point):
 *   Updates the rolling statistics for each field by extracting the field values from the data_point using the getters
 *   and updating the corresponding statistics_space_n_t instances. Only updates if data_point.is_valid() returns true.
 *
 * - normalize(const T& data_point) const:
 *   Creates a normalized copy of data_point by normalizing each field using the corresponding statistics_space_n_t
 *   instance and setting the normalized value back into the copy using the setters. Only normalizes if data_point.is_valid()
 *   returns true.
 *
 * - ready() const:
 *   Checks if all statistics_space_n_t instances are ready (i.e., have accumulated enough data points to compute
 *   meaningful statistics).
 *
 * Usage:
 * - Initialize a statistics_pack_t instance by providing the window size and a vector of FieldAccessor<T> for the fields
 *   wished to track.
 * - Call update() with each new data_point to update the statistics.
 * - Use normalize() to obtain a normalized version of a data_point once the statistics are ready.
 *
 * Note:
 * - The data structure T must provide an is_valid() method to check the validity of the data before updating statistics
 *   or normalizing.
 * - This design allows for flexibility in handling multiple fields of different types within T, by converting all field
 *   values to double for statistical computation.
 */
template <typename T>
struct statistics_pack_t {
  std::vector<statistics_space_n_t> stats_vector;
  std::vector<FieldAccessor<T>> accessors;

  /* Constructor */
  statistics_pack_t(unsigned int window_size, const std::vector<FieldAccessor<T>>& accessors_)
    : stats_vector(accessors_.size(), statistics_space_n_t(window_size)),
      accessors(accessors_) {}

  /* Update function */
  void update(const T& data_point) {
    if(data_point.is_valid()) {
      for (size_t i = 0; i < accessors.size(); ++i) {
        double value = accessors[i].getter(data_point);
        stats_vector[i].update(value);
      }
    }
  }

  /* Normalize function */
  T normalize(const T& data_point) const {
    T normalized_data = data_point;  /* Make a copy */
    if(data_point.is_valid()) {
      for (size_t i = 0; i < accessors.size(); ++i) {
        double original_value = accessors[i].getter(data_point);
        double normalized_value = stats_vector[i].normalize(original_value);
        accessors[i].setter(normalized_data, normalized_value);
      }
    }
    return normalized_data;
  }

  /* ready method */
  bool ready() const {
    for (const auto& stats : stats_vector) {
      if (!stats.ready()) {
        return false;
      }
    }
    return true;
  }
};

} /* namespace cuwacunu */