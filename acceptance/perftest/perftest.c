#include "stats.h"
#include "scenario.h"

#include <glib.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <sysexits.h>
#include <math.h>

typedef struct ScenarioClosure {
  const char*   name;
  TestSuite*    suite;
  guint         runs;
  Scenario*     scenario;
} ScenarioClosure;

typedef struct ProgressClosure {
  TestSuite*       suite;
  ScenarioClosure* physical;
  ScenarioClosure* virtual;
} ProgressClosure;


static gboolean scenario_progress(ProgressClosure* closure) {
  /* turn this into a number of seconds, rounding down... */
  guint runtime = (g_get_monotonic_time() - closure->suite->start_time) / 1000000;

  guint pending = closure->physical->runs + closure->virtual->runs;
  guint threads = g_thread_pool_get_num_threads(closure->suite->pool);
  guint queued  = g_thread_pool_unprocessed(closure->suite->pool);

  g_print(
    "after %4d second%s %3d to be scheduled, %3d running, %3d queued\n",
    runtime, runtime == 1 ? ": " : "s:", pending, threads, queued
  );

  stats_report_concurrency(closure->suite->stats, pending, threads, queued);

  /* now, work out if we are actually *finished* our simulation... */
  if (pending == 0 && threads == 0 && queued == 0)
    g_main_loop_quit(closure->suite->loop);

  return TRUE;
}

static gboolean scenario_scheduler(ScenarioClosure* closure) {
  /* schedule another operation to pool, if needed */
  if (closure->runs > 0) {
    /** @todo danielp 2012-10-10: this needs to assign more than the basic
     * scenario, and to balance load over phys/virt machines.
     */
    g_thread_pool_push(closure->suite->pool, closure->scenario, NULL);

    closure->runs = closure->runs - 1;
  }

  return TRUE;
}


int main(int argc, char* argv[]) {
  curl_global_init(CURL_GLOBAL_ALL);
  TestSuite* suite = test_suite_setup(&argc, &argv);

  /* the worker pool has unlimited size, but uses shared threads to allow for
   * an unlimited number of overlapping operations during the scenario - since
   * we are modelling performance based on arrival rate, not on active
   * node count.
   */
  suite->pool = g_thread_pool_new(
    (GFunc)scenario_handler, suite, -1, FALSE, NULL
  );

  /* ...and the main loop that handles scheduling and exiting. */
  suite->loop = g_main_loop_new(NULL, FALSE);

  /* start our scenario scheduler */
  ScenarioClosure physical_closure = {
    .name      = "physical refresh",
    .suite     = suite,
    .runs      = suite->physical_refresh_events,
    .scenario  = suite->esxi
  };

  ScenarioClosure virtual_closure = {
    .name    = "virtual refresh",
    .suite   = suite,
    .runs    = suite->virtual_refresh_events,
    /** @todo danielp 2012-10-11: this should have more than one scenario... */
    .scenario = suite->ubuntu
  };

  g_print(
    "Testing will run for %d second%s performing scheduling\n"
    "  with %4d (simulated) physical refresh%s\n"
    "  and  %4d (simulated) virtual refresh%s\n"
    "  total rate approximately %.2f refreshes per second\n",
    suite->approximate_runtime, suite->approximate_runtime == 1 ? "" : "s",
    physical_closure.runs, physical_closure.runs == 1 ? "" : "es",
    virtual_closure.runs,  virtual_closure.runs  == 1 ? "" : "es",
    suite->physical_refreshes_per_second + suite->virtual_refreshes_per_second
  );

  g_timeout_add_full(
    G_PRIORITY_HIGH, floor(1000.0 / suite->physical_refreshes_per_second),
    (GSourceFunc)scenario_scheduler, &physical_closure, NULL
  );

  g_timeout_add_full(
    G_PRIORITY_HIGH, floor(1000.0 / suite->virtual_refreshes_per_second),
    (GSourceFunc)scenario_scheduler, &virtual_closure, NULL
  );

  ProgressClosure progress = {
    .suite       = suite,
    .physical    = &physical_closure,
    .virtual     = &virtual_closure
  };
  g_timeout_add_seconds(1, (GSourceFunc)scenario_progress, &progress);

  suite->start_time = g_get_monotonic_time();

  /* submit the first event on both schedules at time zero */
  scenario_scheduler(&physical_closure);
  scenario_scheduler(&virtual_closure);

  /* ...and allow the scheduler to run the rest. */
  g_main_loop_run(suite->loop);

  suite->end_time  = g_get_monotonic_time();

  stats_print_report(suite->stats);

  return 0;
}
