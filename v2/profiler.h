#ifndef _PROFILER_H_
#define _PROFILER_H_

void EnableProfiling();

class Profile {
 public:
  Profile(const char* name);
  ~Profile();

 private:
  const char* name_;
  unsigned long start_us_;
};

#endif
