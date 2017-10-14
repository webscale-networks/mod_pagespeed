// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: aroman@webscalenetworks.com (Augusto Roman)

#ifndef NET_INSTAWEB_HTTP_PUBLIC_FETCH_RACE_H_
#define NET_INSTAWEB_HTTP_PUBLIC_FETCH_RACE_H_

#include "net/instaweb/http/public/async_fetch.h"
#include "net/instaweb/http/public/request_context.h"
#include "pagespeed/kernel/base/basictypes.h"
#include "pagespeed/kernel/base/scoped_ptr.h"
#include "pagespeed/kernel/base/string_util.h"
#include "pagespeed/kernel/base/thread_annotations.h"
#include "pagespeed/kernel/base/thread_system.h"
#include "pagespeed/kernel/base/writer.h"
#include "pagespeed/kernel/base/condvar.h"

namespace net_instaweb {

class MessageHandler;
class Timer;

// FetchRace allows initiating a number of AsyncFetch's in parallel and using
// the first one that starts writing to the response. The results from all
// others are discarded. Typical usage might look something like:
//
//   bool GetSomeResource(AsyncFetch* result) {
//     const int64 start = timer_.NowMs();
//     FetchRace race(result, thread_system_, message_handler_);
//
//     StartDoingSomeOperationThatMightFetchSlowly(race.NewRacer());
//
//     // Start a fallback if original hasn't started writing to the output
//     // within 15 ms.
//     if (!race.WaitForWinner(timer_, start + 15)) {
//       StartABackupFetch(race.NewRacer());
//     }
//
//     // Start another fallback if either of the previous two haven't started
//     // writing to the output.
//     if (!race.WaitForWinner(timer_, start + 30)) {
//       StartAnotherBackupFetch(race.NewRacer());
//     }
//
//     // Now wait to see if any of the previous fetches start writing to result
//     // within 500ms (from start). If not, give up.
//     if (!race.WaitForWinner(timer_, start + 500)) {
//       return false; // No fetch started writing output in time.
//     }
//
//     // Ooooh, OK, now we're getting somewhere. We have some data written.
//     // Let's give it another second to finish writing, otherwise we give up.
//     const int64 finish_writing_deadline = timer_.NowMs() + 1000;
//     if (race.Winner()->WaitForDone(timer_, finish_writing_deadline)) {
//       return false; // The winner was writing to the output, but too slowly.
//     }
//
//     // Now result is all finished, we have data.  Yay!
//     ... use result ...
//
//     return true;
//   }
//
// FetchRace owns all of the created racer fetches and will manage their
// lifetimes as follows:
//   * Racers may always be accessed while the FetchRace object still exists.
//   * Once FetchRace is destroyed, Racers will delete themselves when Done()
//     is called.
class FetchRace {
 public:
  class RacerFetch; // forward declare the nested RacerFetch class.

  // FetchRace initializes a new fetch race to write to target_fetch.
  FetchRace(
      AsyncFetch* target_fetch,
      ThreadSystem* thread_system,
      MessageHandler* message_handler);
  ~FetchRace();

  // NewRacer constructs a new fetch to compete in this fetch race to be the
  // first to write to target_fetch_.
  RacerFetch* NewRacer();

  // Returns true if there was a winner, or false if this timed out without any
  // winner of the race. Once this returns true, Winner() will never return
  // NULL. This function will loop internally and will not return before the
  // time expires or a winner is chosen.
  bool WaitForWinner(Timer* timer, int64 deadline_ms) const LOCKS_EXCLUDED(mutex_);

  // Winner returns NULL if no fetch has won the race yet or a pointer to the
  // racer that has won the race. This will never return NULL after
  // WaitForWinner returns true. Typical this is used to wait for the winner
  // to be done.
  //
  // The returned pointer is valid while FetchRace is alive.
  RacerFetch* Winner() const LOCKS_EXCLUDED(mutex_);

  // RacerFetch is a fetch participating in a fetch race. If it is the first to
  // write to the output, then it has won the race and is then responsible for
  // for writing to that output from then on -- there are no points for second
  // place.
  //
  // RacerFetch can be either owned by the FetchRace or it can delete itself if
  // the FetchRace has already been destroyed. This is because typically the
  // winner RacerFetch will be completed before the FetchRace destruction and
  // may be accessed after it has completed, but loser racer fetchs may live
  // beyond the lifetime of the parent FetchRace. When the FetchRace has been
  // destroyed, it disqualifies all of the racers and, if they have not already
  // finished, they will take ownership of themselves and delete themselves upon
  // completion.
  class RacerFetch : public AsyncFetch {
   public:
    // WaitForDone waits for the fetch to complete using the given timer and
    // deadline. It returns true if the fetch completed before the timeout and
    // is now done or false if it timed out waiting for the fetch to be done.
    //
    // Note that the deadline is an absolute time, NOT a timeout value. That is,
    // a 50ms timeout would be:
    //    timer->NowMs() + 50
    //
    // If the fetch is already done or the deadline is in the past, it will
    // return immediately.
    //
    // This function will loop internally and will not return before the time
    // expires or the fetch is done.
    bool WaitForDone(Timer* timer, int64 deadline_ms) const LOCKS_EXCLUDED(mutex_);

    // ClaimWin will attempt to set this racer as the winner in the FetchRace.
    // It will return true if it's the first racer to claim victory, otherwise
    // it will return false if another racer is already the winner.
    bool ClaimWin() LOCKS_EXCLUDED(mutex_);

   private:
    // RacerFetch initializes a new racer within a FetchRace. The only
    // interesting bit during construction is copying the headers from the
    // target fetch -- this is assumed to be a safe operation and that the
    // request headers will not change while the race is going on. Because
    // racers can only be constructed by the parent FetchRace, we are guaranteed
    // that target_fetch will be valid (and therefore copying the request
    // headers is legit) for the duration of this constructor.
    //
    // This constructor is protected because only the parent FetchRace should
    // be creating RacerFetch instances due to these preconditions.
    RacerFetch(FetchRace* race);
    virtual ~RacerFetch() {}

    // This disqualifies the racer from the fetch race. It will be permanently
    // removed from the race and will not longer write to target_fetch_
    // regardless of whether has already been selected as the winner. If the
    // fetch has already completed, it will delete itself, otherwise it will
    // delete itself immediately upon completion. Once disqualified, the fetch
    // should not be accessed. This is protected because it should only be
    // called by the parent FetchRace.
    void Disqualify() LOCKS_EXCLUDED(mutex_);

    // Disqualified returns true if this racer has been disqualified from the
    // FetchRace. This is only used internally, and is explicitly NOT safe for
    // external use because once disqualified the fetch will delete itself when
    // done, so external users cannot safely call this.
    bool Disqualified() const LOCKS_EXCLUDED(mutex_);

    // FetchRace needs to be able to construct and disqualify RacerFetchs, and
    // it's not safe for anyone else to do so.
    friend class FetchRace;

   protected:
    // These methods are overridden to implement the AsyncFetch interface. Each
    // of these will attempt to claim victory in the race and, if successful,
    // will pass the write on to the target fetch. If unsuccessful, the writes
    // are silently discarded.
    virtual void HandleDone(bool success) LOCKS_EXCLUDED(mutex_);
    virtual bool HandleWrite(const StringPiece& content, MessageHandler* handler) LOCKS_EXCLUDED(mutex_);
    virtual bool HandleFlush(MessageHandler* handler) LOCKS_EXCLUDED(mutex_);
    virtual void HandleHeadersComplete() LOCKS_EXCLUDED(mutex_);
    // These are also overridden for the AsyncFetch interface, but they don't
    // attempt to claim victory -- they are purely informational.
    virtual bool IsCachedResultValid(const ResponseHeaders& headers) LOCKS_EXCLUDED(mutex_);
    virtual bool IsBackgroundFetch() const LOCKS_EXCLUDED(mutex_);

   private:
    // This is copied from the parent race_ in case we are disqualified but
    // still want to log debug messages.
    MessageHandler* message_handler_;

    // mutex_ protects all reads & writes to race_, target_fetch_, and done_.
    // target_fetch_ should only be written to if this racer is the winner and
    // is the first to actually try to write to target_fetch_.
    scoped_ptr<ThreadSystem::CondvarCapableMutex> mutex_;
    // The parent race that we're competing in.
    FetchRace* race_ GUARDED_BY(mutex_);
    // This is copied from the parent race in case we are disqualified but still
    // think we should write to the target_fetch_ because we're the winner.
    // Though unlikely, it's possible:
    //   -- Main thread --                    -- Fetch thread --
    //   FetchRace is initiated.
    //   Racer starts.
    //   ...                                  ... processing occurs ...
    //   WaitForWinner() starts
    //                                        HandleDone() is called
    //   WaitForWinner() times out
    //                                        if (ClaimWin()) --> returns true
    //   FetchRace is destroyed, racer
    //   is disqualified.
    //                                        target_fetch_->Done(success)
    //                                        KABOOM unless this is local.
    AsyncFetch* target_fetch_ GUARDED_BY(mutex_);
    // Keep track of whether this fetch has completed. This is necessary so
    // we can implement WaitForDone and so we can properly clean up racers that
    // are already done when FetchRace is destroyed.
    bool done_ GUARDED_BY(mutex_);
    // done_cond_ is triggered when done_ is modified, once the racer has finished
    // but after HandleDone has been passed to the target_fetch_ if appropriate.
    scoped_ptr<ThreadSystem::Condvar> done_cond_;

    // Copying the mutex cannot be done safely. Only pointers to RacerFetch
    // should ever be handled, with construction done by the parent FetchRace
    // and destruction handled either by FetchRace or, if orphaned, itself.
    DISALLOW_COPY_AND_ASSIGN(RacerFetch);
  }; // class RacerFetch

 private:
  // Finish is called by each racer when they are attempting to write to the
  // target fetch. It returns true if racer won the race. This may be called
  // multiple times for the same racer and it will always return true if racer
  // was the first to finish.
  bool Finish(RacerFetch* racer) LOCKS_EXCLUDED(mutex_);

 private:
  // The environment that we're running in:
  ThreadSystem* thread_system_;
  MessageHandler* message_handler_;

  // The prize! Who will get it?!
  AsyncFetch* target_fetch_;

  // Winner management.
  scoped_ptr<ThreadSystem::CondvarCapableMutex> mutex_;
  scoped_ptr<ThreadSystem::Condvar> winner_cond_;
  RacerFetch* winner_ GUARDED_BY(mutex_);

  // The list of racers that we've spawned. These need to be cleaned up when we
  // are destroyed.
  std::vector<RacerFetch*> racers_;

  DISALLOW_COPY_AND_ASSIGN(FetchRace);
};

}  // namespace net_instaweb

#endif  // NET_INSTAWEB_HTTP_PUBLIC_FETCH_RACE_H_
