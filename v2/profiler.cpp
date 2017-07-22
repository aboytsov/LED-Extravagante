#include "profiler.h"

#include <FastLED.h>

namespace {

bool profiling = false;

struct ProfileData {
  const char* name;
  
  unsigned long sum_us;
  unsigned int count;
  unsigned long max_us;

  unsigned long previous_time_us;
  unsigned long delay_sum_us;
};
ProfileData* profiles = new ProfileData[0];
int num_profiles = 0;
  
}  // namespace

void EnableProfiling() {
  profiling = true;
}

Profile::Profile(const char* name) : name_(name), start_us_(profiling ? micros() : 0) {
}

Profile::~Profile() {
  if (profiling) {
    unsigned long time_us = micros() - start_us_;

    // Find the profile.
    ProfileData* profile = nullptr;
    for (int i = 0; i < num_profiles; ++i) {
      if (profiles[i].name == name_) {  // Intentional pointer comparison.
        profile = &profiles[i];
        break;
      }
    }

    if (profile == nullptr) {
      ProfileData* new_profiles = new ProfileData[num_profiles + 1];
      memcpy(new_profiles, profiles, num_profiles * sizeof(ProfileData));
      delete[] profiles;
      profiles = new_profiles;

      profiles[num_profiles].name = name_;
      
      profiles[num_profiles].sum_us = 0;
      profiles[num_profiles].count = 0;
      profiles[num_profiles].max_us = 0;
      
      profiles[num_profiles].previous_time_us = start_us_;
      profiles[num_profiles].delay_sum_us = 0;
      
      ++num_profiles;

      // We intentionally ignore the first measurement.
    } else {
      profile->sum_us += time_us;
      ++profile->count;
      profile->max_us = max(profile->max_us, time_us);

      profile->delay_sum_us += start_us_ - profile->previous_time_us;
      profile->previous_time_us = start_us_;
    }

    EVERY_N_MILLISECONDS(10000) {
      for (int i = 0; i < num_profiles; ++i) {
        ProfileData& profile = profiles[i];
        Serial.print(profile.name);
        Serial.print(": average time is ");
        Serial.print(profile.sum_us / profile.count);
        Serial.print(" us, max time is ");
        Serial.print(profile.max_us);
        Serial.print(" us, called on average every ");
        Serial.print(profile.delay_sum_us / profile.count);
        Serial.println(" us");
      }
      Serial.println();
    }
  }
}

