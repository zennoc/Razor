#ifndef STATS_H
#define STATS_H

typedef struct Stats Stats;

#include "scenario.h"

#include <glib.h>

typedef void (*StatsEventFunc)(Stats* stats, gpointer data);

Stats* stats_new();
void stats_free();

void stats_print_report(Stats* stats);

typedef struct EventFinished {
  const Event* event;
  gboolean     successful;
  guint64      bytes;
  guint64      start;
  guint64      first_data;
  guint64      finish;
} EventFinished;

/**
 * Report stats when a URL event has completed.
 * @param[in] stats  the stats collection to report against.
 * @param[in] data   a EventFinished structure containing the data.
 *
 * the EventFinished pointer must be allocated by the caller with
 * stats_event_finished_new(), and this function will free the structure when
 * it is finished using it.
 */
void stats_event_finished(Stats* stats, EventFinished* data);

/**
 * Allocate a new EventFinished structure.  The structure will be
 * zero-filled, other than the event pointer.
 *
 * @param[in] event  the Event that was completed.
 * @returns[caller frees] the EventFinished message.
 */
EventFinished* stats_event_finished_new(const Event* event);

/**
 * Report a concurrency stats event.
 * @param[in] stats    the stats object to report against
 * @param[in] pending  number of pending scenarios awaiting scheduling
 * @param[in] running  number of currently running scenarios
 * @param[in] queued   number of scenarios scheduled but blocked waiting on
 * an executor thread
 */
void stats_report_concurrency(Stats* stats, guint pending, guint running, guint queued);

#endif /* STATS_H */

