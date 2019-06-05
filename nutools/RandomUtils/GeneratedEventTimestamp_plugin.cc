/**
 * @file    GeneratedEventTimestamp_plugin.cc
 * @brief   Assigns an empty event a time stamp from the clock
 * @author  Gianluca Petrillo (petrillo@fnal.gov)
 * 
 * This file defines a plug in for art framework; no interface is needed since
 * users will interface with the abstract base class.
 */

// C/C++ standard libraries
#include <chrono>
#include <random>

// framework libraries
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "canvas/Persistency/Provenance/Timestamp.h"
#include "canvas/Persistency/Provenance/EventID.h"
#include "art/Framework/Core/EmptyEventTimestampPlugin.h"

// Event generation namespace
namespace evgen {
  
  /**
   * @brief Plugin to assign an empty event a time stamp from the clock
   * @see `art::EmptyEventTimestampPlugin`
   *
   * The plug in returns a time stamp that is taken from the current time 
   * on the execution node, in nanoseconds.
   * 
   * The time is currently defined as absolute from the UNIX "epoch" (first day
   * of year 1970), but its absolute precision should not be relied upon.
   * 
   * Also note that the time is not guaranteed to be monotonic, that is,
   * generating two events in sequence, it is not guaranteed that the second one
   * has a time stamp larger than the previous. This may for example happen if
   * the clock relies on a CPU internal counter, on a machine with multiple CPUs
   * (that is probably all of them).
   * 
   * 
   * Configuration
   * --------------
   * 
   * None so far.
   * 
   */
  class GeneratedEventTimestamp: public art::EmptyEventTimestampPlugin {
      public:
    
    /// Constructor: nothing specific
    GeneratedEventTimestamp(fhicl::ParameterSet const& pset);
    
    
    /// Returns the time stamp for the specified event
    virtual art::Timestamp eventTimestamp(art::EventID const& id) override;
    
    /// Resets the status; since this plug in is stateless, this is a no-op
    virtual void rewind() override {}
    
    
      private:
    /// Offset to be added to the chosen clock to get an absolute time.
    art::TimeValue_t const fOffsetFromEpoch = 0;
    
  }; // class GeneratedEventTimestamp
  
  
  
  
} // namespace evgen

//------------------------------------------------------------------------------
//--- Implementation
//---
//---

namespace evgen {
  namespace details {
    
    //--------------------------------------------------------------------------
    template <typename T>
    struct Average {
      using data_t = T;
      
      void clear() { fTotal = data_t(0); fN = 0U; }
      void insert(data_t value) { fTotal += value; ++fN; }
      
      data_t n() const { return fN; }
      data_t average() const { return fTotal / fN; }
      
        private:
      unsigned int fN = 0U;
      data_t       fTotal = data_t(0);
      
    }; // class Average
    
    
    //--------------------------------------------------------------------------
    /// Returns the multiple of `period` closest to `value`.
    template <typename T>
    auto discretize(T value, T period) {
      auto const excess = value % period;
      auto const base = value - excess;
      return (excess < (period / T(2)))? base: base + period;
    } // discretize()
    
    
    //--------------------------------------------------------------------------
    /// Class reading a `Clock` and converting the value to a specific `Unit`.
    template <typename Clock, typename Unit>
    class TimeInUnitsBase {
        public:
      
      /// Type of the time duration as returned by this class.
      using duration_t = art::TimeValue_t;
      
      /// Reads and returns the current time the clock.
      duration_t operator() () { return read_clock(); }
      
      /// Reads and returns the current time the clock.
      static duration_t read_clock() { return timeFromEpoch(Clock::now()); }
      
      /// Computes an approximation of the offset of the current time from the
      /// epoch.
      static duration_t currentOffsetFromEpoch();
      
        protected:
      
      /// Converts a `std::chrono::duration` into our duration metric.
      template <typename TimeInterval>
      static constexpr duration_t toDuration(TimeInterval dt)
        {
          return static_cast<duration_t>
            (std::chrono::duration_cast<Unit>(dt).count());
        }
      
      /// Returns the duration (`duration_t`) of a period type.
      template <typename Rep, typename Period>
      static constexpr auto periodToDuration()
        { return toDuration(std::chrono::duration<Rep, Period>(1)); }
      
      /// Returns the time elapsed from the epoch to `t`.
      template <typename TimePoint>
      static duration_t timeFromEpoch(TimePoint t)
        { return toDuration(t.time_since_epoch()); }
      
    }; // class TimeInUnitsBase<>
    
    
    template <typename Clock, typename Duration>
    auto TimeInUnitsBase<Clock, Duration>::currentOffsetFromEpoch()
      -> duration_t
    {
      /*
       * The plan is to compare the `Clock` we use with the system clock, which
       * is guaranteed by the C++20 standard to refer to a well defined absolute
       * time point (the UNIX epoch, January 1, 1970).
       * Chances are that the resolution of the system clock is not as good as
       * the one of `Clock`. If the difference between the two clocks is less
       * than a few seconds, we attribute the difference to chance and we don't
       * correct for it.
       * 
       * Otherwise, the same time (almost!) is taken from the two clocks, and
       * the difference in `Duration` units is used as a correction.
       * 
       */
      using clock_t = Clock;
      using system_clock_t = std::chrono::system_clock;
      using namespace std::chrono_literals;
      
      // no point in doing the exercise if we are already using the system clock
      if (std::is_same<clock_t, system_clock_t>()) {
        MF_LOG_DEBUG("GeneratedEventTimestamp")
          << "Using system clock for timestamp: no offset needed.";
        return static_cast<duration_t>(0);
      }
      
      auto clock_time = clock_t::now();
      auto sys_clock_time = system_clock_t::now();
      
      // if the system clock is close to our clock, of if it is ahead of it,
      // use no offset (the latter stems from the consideration that that the
      // two clocks are equivalent although they suffer from some jitter)
      if (
        (timeFromEpoch(sys_clock_time) - timeFromEpoch(clock_time))
          < toDuration(5s)
        )
      {
        MF_LOG_DEBUG("GeneratedEventTimestamp")
          << "Offset with system clock is small ("
            << (timeFromEpoch(sys_clock_time) - timeFromEpoch(clock_time))
            << ", " << timeFromEpoch(sys_clock_time)
            << " vs. " << timeFromEpoch(clock_time)
            << "): no offset needed."
          ;
        return static_cast<duration_t>(0);
      }
      
      //
      // pick the largest of the resolutions for the comparison
      //
      using clock_period_t = typename clock_t::period;
      using system_clock_period_t = system_clock_t::period;
      using largest_period_t = std::conditional_t<
        (
          clock_period_t::num * system_clock_period_t::den
          > system_clock_period_t::num * clock_period_t::den
        ),
        clock_period_t,
        system_clock_period_t
        >;
      // this is the period expressed in the Duration unit
      constexpr auto largest_period
        = periodToDuration<typename clock_t::rep, largest_period_t>();
      
      //
      // compare and round
      //
      constexpr unsigned int times = 10U; // average 10 samples
      Average<duration_t> offset;
      for (unsigned int i = 0; i < times; ++i) {
        
        offset.insert
          (timeFromEpoch(sys_clock_time) - timeFromEpoch(clock_time));
        clock_time     = clock_t::now();
        sys_clock_time = system_clock_t::now();
        
      } // for
      
      MF_LOG_DEBUG("GeneratedEventTimestamp")
        <<   "System clock period: "
          << periodToDuration<typename clock_t::rep, system_clock_period_t>()
        << "\nUser clock period:   "
          << periodToDuration<typename clock_t::rep, clock_period_t>()
        << "\nOffset:              " << offset.average()
          << " (rounded to: " << largest_period << ")"
        ;
      
      // round off the offset with one "largest period"
      return discretize(offset.average(), largest_period);
      
    } // TimeInUnitsBase<>::currentOffsetFromEpoch()
    
    
    //--------------------------------------------------------------------------
    /// Class reading a clock and converting the value to a specific unit;
    /// if the unit is more precise than the clock, random padding fills the gap
    template <typename Clock, typename Duration, typename = void>
    class TimeInUnits: public TimeInUnitsBase<Clock, Duration> {};


    // Implementation of the random-gap-filling version
    template <typename Clock, typename Duration>
    class TimeInUnits<Clock, Duration, typename std::enable_if<
        (Clock::period::num * Duration::period::den > Duration::period::num * Clock::period::den)
        >::type
      >
      : public TimeInUnitsBase<Clock, Duration>
    {
      using Base_t = TimeInUnitsBase<Clock, Duration>;
      using typename Base_t::duration_t;
      
      // if the period of the clock is larger than the unit we are requested,
      // there will be some padding
      using ClockPeriod = typename Clock::period;
      using ReqPeriod = typename Duration::period;
      // requested clock / unit:
      using PeriodRatio = std::ratio<
        ClockPeriod::num * ReqPeriod::den, ReqPeriod::num * ClockPeriod::den
        >;
      static constexpr intmax_t paddingfactor
        = PeriodRatio::num / PeriodRatio::den; // > 1 enforced by enable_if<>
      
        public:
      TimeInUnits(): engine(), flat(0, paddingfactor - 1) {}
      
      /// Return the clock value with random padding added
      duration_t operator() ()
        { return Base_t::read_clock() + flat(engine); }
      
        private:
      std::default_random_engine engine;
      std::uniform_int_distribution<duration_t> flat;
    }; // class TimeInUnits<> (padded)
    
    
    using ns_clock_t = details::TimeInUnits
      <std::chrono::high_resolution_clock, std::chrono::nanoseconds>;
    
    //--------------------------------------------------------------------------
    
  } // namespace details
} // namespace evgen


//------------------------------------------------------------------------------
evgen::GeneratedEventTimestamp::GeneratedEventTimestamp
  (fhicl::ParameterSet const& pset)
  : art::EmptyEventTimestampPlugin(pset)
  , fOffsetFromEpoch(details::ns_clock_t::currentOffsetFromEpoch())
{
  
  mf::LogInfo("GeneratedEventTimestamp")
    << "Timestamp plugin: timestamp from local clock time in nanoseconds";
  if (fOffsetFromEpoch != 0) {
    MF_LOG_TRACE("GeneratedEventTimestamp")
      << "  Time offset from epoch: " << fOffsetFromEpoch << " ns";
  }
  
} // evgen::GeneratedEventTimestamp::GeneratedEventTimestamp()


//------------------------------------------------------------------------------
art::Timestamp evgen::GeneratedEventTimestamp::eventTimestamp
  (art::EventID const& id)
{
  // obtain from the high resolution clock the current time, from the "epoch",
  // in nanoseconds; if the clock is less precise than the nanosecond,
  // the precision gap is filled with randomness
  details::ns_clock_t get_time;
  
  const long long int now_ns = fOffsetFromEpoch + get_time();
  
  // convert into a timestamp
  art::Timestamp ts(now_ns);
  
  mf::LogTrace("GeneratedEventTimestamp")
    << "Generated time stamp: " << ts.value() << " for event " << id;
  
  return ts;
} // evgen::GeneratedEventTimestamp::eventTimestamp()

// make art aware that we have a plugin
DEFINE_ART_EMPTYEVENTTIMESTAMP_PLUGIN(evgen::GeneratedEventTimestamp)

