/*
    SevenInstall
    Copyright (c) 2013-2017 Frank Richter

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or
    distribute this software, either in source code form or as a compiled
    binary, for any purpose, commercial or non-commercial, and by any
    means.

    In jurisdictions that recognize copyright laws, the author or authors
    of this software dedicate any and all copyright interest in the
    software to the public domain. We make this dedication for the benefit
    of the public at large and to the detriment of our heirs and
    successors. We intend this dedication to be an overt act of
    relinquishment in perpetuity of all present and future rights to this
    software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
    OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.

    For more information, please refer to <http://unlicense.org>
 */

/**\file
 * Progress reporting utility
 */

#ifndef SEVENI_PROGRESSREPORTER_HPP_
#define SEVENI_PROGRESSREPORTER_HPP_

#include <stdint.h>

#include <memory>
#include <vector>

class BurnPipe;

struct ProgressReporter
{
  virtual ~ProgressReporter() {}

  /// Progress feedback: whether to continue or cancel processing
  enum struct Processing { Continue, Cancel };

  virtual void SetTotal (uint64_t total) = 0;
  virtual Processing SetCompleted (uint64_t completed) = 0;
};

class ProgressReporterDummy : public ProgressReporter
{
public:
  void SetTotal (uint64_t /*total*/) override {}
  Processing SetCompleted (uint64_t /*completed*/) override { return Processing::Continue; }
};

class ProgressReporterMultiStep : protected ProgressReporter
{
public:
  ProgressReporterMultiStep (ProgressReporter& target);

  typedef size_t phase_type;
  phase_type AddPhase (uint64_t total);

  ProgressReporter& GetPhase (phase_type phase);
protected:
  ProgressReporter& target;

  struct PhaseInfo
  {
    uint64_t start, size;
  };
  std::vector<PhaseInfo> phases;
  phase_type currentPhase = (phase_type)-1;
  uint64_t currentTotal = 0;

  bool phasesDirty = true;

  void SetTotal (uint64_t total) override;
  Processing SetCompleted (uint64_t completed) override;
};

class ProgressReporterPipe : public ProgressReporter
{
public:
  ProgressReporterPipe (BurnPipe& pipe);

  void SetTotal (uint64_t total) override;
  Processing SetCompleted (uint64_t completed) override;
private:
  BurnPipe& pipe;
  uint64_t total = 0;
};

/// Get a default progress reporter.
std::shared_ptr<ProgressReporter> GetDefaultProgress (BurnPipe& pipe);

#endif // SEVENI_PROGRESSREPORTER_HPP_
