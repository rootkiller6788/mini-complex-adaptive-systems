#ifndef EOC_APPLICATION_H
#define EOC_APPLICATION_H

#include "eoc_core.h"
#include "eoc_powerlaw.h"

/* Application-level structures and APIs */

typedef struct {
    double b_value;
    double b_value_error;
    double a_value;
    double mc;
    int n_events;
    bool is_soc_consistent;
} EOCGutenbergRichterResult;

typedef struct {
    int* grid;
    int width;
    int height;
    double p_growth;
    double f_spark;
    double p_spread;
    int* cluster_sizes;
    int n_fires;
    int max_fires;
    double total_trees;
    double total_burned;
} EOCForestFireModel;

typedef enum {
    SIGNAL_WHITE_NOISE = 0,
    SIGNAL_PINK_NOISE = 1,
    SIGNAL_BROWNIAN = 2,
    SIGNAL_LONG_MEMORY = 3,
    SIGNAL_ANTI_PERSISTENT = 4,
    SIGNAL_UNKNOWN = 5
} EOCSignalClass;

EOCGutenbergRichterResult eoc_earthquake_gutenberg_richter(
    double* magnitudes, int n);

EOCForestFireModel* eoc_forest_fire_create(int w, int h,
    double p_growth, double f_spark, double p_spread);
void eoc_forest_fire_free(EOCForestFireModel* ff);
int eoc_forest_fire_step(EOCForestFireModel* ff);
EOCPowerLawFit eoc_forest_fire_powerlaw_fit(EOCForestFireModel* ff);

EOCSignalClass eoc_classify_signal_criticality(double* time_series, int n);
double* eoc_generate_pink_noise(int n);

EOCPowerLawFit eoc_network_degree_powerlaw(int* degrees, int n_nodes);
double eoc_network_criticality_score(int* degrees, int* edges_from,
    int* edges_to, int n_nodes, int n_edges);

#endif
