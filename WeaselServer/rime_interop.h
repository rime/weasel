#pragma once

class rime
{
public:
  rime();
  ~rime();
  rime(const rime&) = delete;
  rime& operator=(const rime&) = delete;
  rime(rime&&) = delete;
  rime& operator=(rime&&) = delete;
  
private:
  bool disabled_;
};