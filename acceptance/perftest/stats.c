#include "stats.h"

#include <glib.h>
#include <uriparser/Uri.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

struct Stats {
  TestSuite*    suite;
  GThreadPool*  pool;

  /* data relating to individual URL fetch performance, and group fetch
   * performance, indexed by the name of what was fetched */
  GTree*        by_url;
  GTree*        by_scenario_part;
  GTree*        by_scenario;

  /* data related to concurrency, indexed by time, sampled through the life of
   * the run */
  GPtrArray*    concurrency;
};

typedef struct StatsEvent {
  StatsEventFunc  handler;
  gpointer        data;
} StatsEvent;

static void stats_handler(StatsEvent* event, Stats* stats);
static void stats_send_event(GThreadPool* pool, StatsEventFunc handler, gpointer data);
static void stats_record_event_finished(Stats* stats, EventFinished* event);
static void stats_record_concurrency(Stats* stats, gpointer data);

static inline EventFinished* event_finished_array_get(GPtrArray* array, guint index) {
  return (EventFinished*)g_ptr_array_index(array, index);
}

static inline gdouble relative_time(guint64 start, guint64 event) {
  if (event < start)
    return 0;

  double when = event - start;
  return when / 1000000;
}

static gint compare_pointer(gconstpointer a, gconstpointer b);

/** as much URI as we need to parse here... */
typedef struct URI {
  const char* scheme;
  const char* user;
  const char* host;
  guint       port;
  const char* path;
  const char* query;
  const char* fragment;
} URI;

static URI* parse_uri(const char* uri);
static void free_uri(URI* uri);

/************************************************************************
 * Private types
 */
typedef struct ConcurrencyClosure {
  guint64 when;
  guint   pending;
  guint   running;
  guint   queued;
} ConcurrencyClosure;

/**************************************************************************
 * Public interface
 */
Stats* stats_new(TestSuite* suite) {
  Stats* stats            = g_new0(Stats, 1);
  stats->suite            = suite;
  stats->by_url           = g_tree_new((GCompareFunc)g_strcmp0);
  stats->by_scenario_part = g_tree_new(compare_pointer);
  stats->by_scenario      = g_tree_new(compare_pointer);
  stats->concurrency      = g_ptr_array_new();
  stats->pool             = g_thread_pool_new((GFunc)stats_handler, stats, 1, TRUE, NULL);
  return stats;
}

EventFinished* stats_event_finished_new(const Event* event) {
  EventFinished* data = g_slice_new0(EventFinished);
  data->event = event;
  return data;
}

void stats_event_finished(Stats* stats, EventFinished* data) {
  stats_send_event(stats->pool, (StatsEventFunc)stats_record_event_finished, data);
}

void stats_report_concurrency(Stats* stats, guint pending, guint running, guint queued) {
  ConcurrencyClosure* data = g_slice_new(ConcurrencyClosure);

  data->when    = g_get_monotonic_time();
  data->pending = pending;
  data->running = running;
  data->queued  = queued;

  stats_send_event(stats->pool, (StatsEventFunc)stats_record_concurrency, data);
}

static void write_concurrency(Stats *stats) {
  FILE* c = fopen("concurrency.csv", "wb");
  fprintf(c, "when, running, pending, queued\n");
  fprintf(c, "0.0, 0, 0, 0\n"); /* start at zero! */
  for (int i = 0; i < stats->concurrency->len; ++i) {
    ConcurrencyClosure* data = stats->concurrency->pdata[i];
    fprintf(
      c, "%f, %d, %d, %d\n",
      relative_time(stats->suite->start_time, data->when),
      data->running, data->pending, data->queued
    );
  }
  fclose(c);
}

typedef struct WriteNetworkClosure {
  FILE*        csv;
  GHashTable*  jtl;
} WriteNetworkClosure;

static FILE* get_jtl_file_handle(
  GHashTable* table, const gchar* scenario, const gchar* part, const gchar* service
) {
  gchar* filename = g_strdup_printf("%s-%s-%s.jtl", scenario, part, service);

  FILE* result = g_hash_table_lookup(table, filename);
  if (result) {
    g_free(filename);
    return result;
  }

  result = fopen(filename, "wb");
  if (!result) {
    g_critical("can't open %s for output: %s", filename, strerror(errno));
    exit(1);
  }

  /* the hash table now owns the filename string */
  g_hash_table_insert(table, filename, result);

  /* the file header... */
  fprintf(result, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(result, "<testResults version=\"2.1\">\n");

  return result;
}

static void close_jtl_file(gpointer data_) {
  FILE* jtl = data_;
  fprintf(jtl, "</testResults>\n");
  fclose(jtl);
}

static gboolean write_network_url_entry(
  gpointer key_, gpointer value_, gpointer data_
) {
  const char*          url     = key_;
  GPtrArray*           samples = value_;
  WriteNetworkClosure* closure = data_;

  URI* uri = parse_uri(url);

  const char* service = "unknown";
  switch (uri->port) {
  case 8026:
    service = "api";
    break;
  case 8027:
    service = "files";
    break;
  default:
    if (!uri->scheme)
      service = "undefined";
    else if (g_ascii_strcasecmp(uri->scheme, "tftp") == 0)
      service = "tftp";
    break;
  }

  /** @todo danielp 2012-10-11: this should be pre-decoded for us, maybe? */
  for (int i = 0; i < samples->len; ++i) {
    EventFinished* record = samples->pdata[i];
    const Event*   event  = record->event;
    fprintf(
      closure->csv, "%s, %s, %s, %s, %s, %f, %f\n",
      event->scenario_part->scenario->name, event->scenario_part->name,
      uri->scheme, service, uri->path,
      relative_time(record->start, record->first_data),
      relative_time(record->start, record->finish)
    );

    FILE* jtl = get_jtl_file_handle(
      closure->jtl,
      event->scenario_part->scenario->name, event->scenario_part->name,
      service
    );
    fprintf(
      jtl,
      "  <sample sc=\"1\" ts=\"%ld\" t=\"%f\" lt=\"%f\" ec=\"%d\" s=\"%s\" "
      "by=\"%ld\" lb=\"%s\" />\n",
      record->start / 1000,                             /* timestamp, milliseconds */
      relative_time(record->start, record->finish),     /* elapsed time */
      relative_time(record->start, record->first_data), /* latency */
      record->successful ? 0 : 1,                       /* error count */
      record->successful ? "true" : "false",            /* success */
      record->bytes,                                    /* byte count */
      record->event->url                                /* label */
    );
  }

  free_uri(uri);

  return FALSE;                 /* continue traversal */
}

static void write_network_data(Stats *stats) {
  WriteNetworkClosure closure = {
    .csv = fopen("network.csv", "wb"),
    /* hash string => FILE* */
    .jtl = g_hash_table_new_full(
      g_str_hash, g_str_equal, g_free, close_jtl_file
    )
  };
  fprintf(closure.csv, "scenario, part, scheme, service, path, first_byte, total\n");
  g_tree_foreach(stats->by_url, write_network_url_entry, &closure);
  fclose(closure.csv);
  /* this will close all files, free the keys, and destroy the object */
  g_hash_table_unref(closure.jtl);
}


static gboolean write_scenario_part_data(
  gpointer key_, gpointer value_, gpointer data_
) {
  ScenarioPart* part    = key_;
  GPtrArray*    samples = value_;
  FILE*         out     = data_;

  for (int i = 0; i < samples->len; ++i) {
    EventFinished* record = samples->pdata[i];
    fprintf(
      out, "%s, %s, %f, %f\n",
      part->scenario->name, part->name,
      relative_time(record->start, record->first_data),
      relative_time(record->start, record->finish)
    );
  }

  return FALSE;                 /* continue traversal */
}

static void write_scenario_data(Stats *stats) {
  FILE* c = fopen("scenario.csv", "wb");
  fprintf(c, "scenario, part, first_byte, total\n");
  g_tree_foreach(stats->by_scenario_part, write_scenario_part_data, c);
  fclose(c);
}


void stats_print_report(Stats* stats) {
  g_print("Writing stats reports:\n");

  /* concurrency data */
  g_print(" - concurrency.csv: ");
  write_concurrency(stats);
  g_print("done\n");

  g_print(" - network.csv, network-*.jtl: ");
  write_network_data(stats);
  g_print("done\n");

  g_print(" - scenario.csv: ");
  write_scenario_data(stats);
  g_print("done\n");
}


/**************************************************************************
 * Private helpers
 */
static gint compare_pointer(gconstpointer a, gconstpointer b) {
  return GPOINTER_TO_INT(a) - GPOINTER_TO_INT(b);
}

static void stats_send_event(
  GThreadPool* pool, StatsEventFunc handler, gpointer data
) {
  StatsEvent* event = g_slice_new(StatsEvent);
  event->handler    = handler;
  event->data       = data;
  g_thread_pool_push(pool, event, NULL);
}

static void stats_handler(StatsEvent* event, Stats* stats) {
  event->handler(stats, event->data);
  g_slice_free(StatsEvent, event);
}

static void add_event_finished_record(GTree* tree, gpointer key, EventFinished* data) {
  GPtrArray* array = g_tree_lookup(tree, key);
  if (!array) {
    array = g_ptr_array_new();
    g_tree_insert(tree, key, array);
  }

  g_ptr_array_add(array, data);
}

static void stats_record_event_finished(Stats* stats, EventFinished* data) {
  add_event_finished_record(stats->by_url, (gpointer)data->event->url, data);
  add_event_finished_record(stats->by_scenario, data->event->scenario_part->scenario, data);
  add_event_finished_record(stats->by_scenario_part, data->event->scenario_part, data);
}

static void stats_record_concurrency(Stats* stats, gpointer raw) {
  /* just record the data for later reporting; we have no indexing to do */
  g_ptr_array_add(stats->concurrency, raw);
}


static gchar* dup_uri_range(UriTextRangeA range) {
  if (!range.first || !range.afterLast || range.first == range.afterLast)
    return NULL;
  return g_strndup(range.first, range.afterLast - range.first);
}

static URI* parse_uri(const char* text) {
  UriUriA          uri;
  UriParserStateA  uri_state = { .uri = &uri };

  URI* result = NULL;

  if (uriParseUriA(&uri_state, text) != URI_SUCCESS) {
    g_print("failed to parse URL %s\n", text);
    goto out;
  }

  result = g_new0(URI, 1);
  result->scheme   = dup_uri_range(uri.scheme);
  result->user     = dup_uri_range(uri.userInfo);
  result->host     = dup_uri_range(uri.hostText);
  /* port is hard */
  /* path is hard */
  result->query    = dup_uri_range(uri.query);
  result->fragment = dup_uri_range(uri.fragment);

  /* ick, thanks uriparser, and const struct members */
  const char* port = dup_uri_range(uri.portText);
  if (port)
    result->port = g_ascii_strtoll(port, NULL, 10);
  g_free((gpointer)port);

  /* also here, ick! */
  GString* path = g_string_new("");
  for (UriPathSegmentA *e = uri.pathHead; e; e = e->next) {
    g_string_append_c(path, '/');
    g_string_append_len(path, e->text.first, e->text.afterLast - e->text.first);
  }
  result->path = g_string_free(path, FALSE);

out:
  uriFreeUriMembersA(&uri);
  return result;
}

static void free_uri(URI* uri) {
  g_free((gpointer)uri->scheme);
  g_free((gpointer)uri->user);
  g_free((gpointer)uri->host);
  /* no need to free port */
  g_free((gpointer)uri->path);
  g_free((gpointer)uri->query);
  g_free((gpointer)uri->fragment);
  g_free(uri);
}
