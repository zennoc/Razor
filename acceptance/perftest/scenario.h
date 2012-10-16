#ifndef SCENARIO_H
#define SCENARIO_H

#include <glib.h>

typedef struct Event         Event;
typedef struct ScenarioPart  ScenarioPart;
typedef struct Scenario      Scenario;
typedef struct TestSuite     TestSuite;

struct Event {
  ScenarioPart* scenario_part;
  const char*   url;
  gboolean      expect_success;
};

struct ScenarioPart {
  Scenario*   scenario;
  const char* name;
  GPtrArray*  events;
};

struct Scenario {
  const char* name;
  GList*      parts;
};

struct TestSuite {
  struct Stats* stats;
  GThreadPool*  pool;
  GMainLoop*    loop;

  char*  target;
  guint  load;
  guint  physical_nodes;
  guint  virtual_nodes;
  guint  nodes;
  double physical_refresh_percent;
  double virtual_refresh_percent;
  guint  virtual_per_physical;

  double physical_refreshes_per_second;
  guint  physical_refresh_events;
  double virtual_refreshes_per_second;
  guint  virtual_refresh_events;

  guint approximate_runtime;

  guint64 start_time;
  guint64 end_time;

  Scenario* esxi;
  Scenario* ubuntu;
};


TestSuite* test_suite_setup(int* argc, char*** argv);

void scenario_handler(const Scenario* scenario, TestSuite* suite);

#endif /* SCENARIO_H */
