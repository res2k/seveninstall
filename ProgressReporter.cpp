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

#include "ProgressReporter.hpp"

#include "MulDiv64.hpp"

#include <assert.h>

ProgressReporterMultiStep::ProgressReporterMultiStep (ProgressReporter& target) : target (target) {}

ProgressReporterMultiStep::phase_type ProgressReporterMultiStep::AddPhase (uint64_t total)
{
  uint64_t phasesTotal = 0;
  if (!phases.empty())
  {
    const auto& lastPhase = phases.back();
    phasesTotal = lastPhase.start + lastPhase.size;
  }

  auto new_id = phases.size();
  PhaseInfo newPhase;
  newPhase.start = phasesTotal;
  newPhase.size = total;
  phases.push_back (newPhase);
  phasesDirty = true;
  return new_id;
}

ProgressReporter& ProgressReporterMultiStep::GetPhase (phase_type phase)
{
  assert(!phases.empty());
  if (phasesDirty)
  {
    const auto& lastPhase = phases.back();
    target.SetTotal (lastPhase.start + lastPhase.size);
    phasesDirty = false;
  }

  currentPhase = phase;
  currentTotal = 0;
  target.SetCompleted (phases[currentPhase].start);
  return *this;
}

void ProgressReporterMultiStep::SetTotal (uint64_t total)
{
  currentTotal = total;
}

void ProgressReporterMultiStep::SetCompleted (uint64_t completed)
{
  const auto& phaseInfo = phases[currentPhase];
  uint64_t targetCompleted = phaseInfo.start;
  if (currentTotal != 0) targetCompleted += MulDiv64 (completed, phaseInfo.size, currentTotal);
  target.SetCompleted (targetCompleted);
}

//---------------------------------------------------------------------

std::shared_ptr<ProgressReporter> GetDefaultProgress ()
{
  return std::make_shared<ProgressReporterDummy> ();
}
