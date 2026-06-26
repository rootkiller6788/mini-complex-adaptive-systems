/**
 * @file    example_el_farol_bar.c
 * @brief   El Farol Bar Problem (Arthur 1994)
 *
 * N agents with bounded rationality decide weekly whether to attend
 * a bar. Each has inductive predictors; they choose the most accurate
 * recently. Emergent property: attendance self-organises around the
 * capacity threshold without central coordination.
 *
 * Knowledge: L1 (bounded rationality), L2 (inductive reasoning),
 *   L5 (agent simulation), L6 (canonical SFI problem)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define NUM_AGENTS        100
#define BAR_CAPACITY       60
#define NUM_WEEKS         200
#define MEMORY_LENGTH       5
#define NUM_PREDICTORS     10

typedef struct {
    int   predictors[NUM_PREDICTORS];
    int   predictor_accuracy[NUM_PREDICTORS];
    int   chosen_predictor;
    int   attended;
} el_farol_agent_t;

static int weekly_attendance[NUM_WEEKS + MEMORY_LENGTH + 1];

static int predict_cyclic(int week, int cycle_len) {
    if (week < cycle_len) return 0;
    int past = weekly_attendance[week - cycle_len];
    return (past < BAR_CAPACITY) ? 1 : 0;
}

static int predict_average(int week, int window) {
    if (week < window) return 1;
    int sum = 0;
    for (int i = 1; i <= window; i++)
        sum += weekly_attendance[week - i];
    double avg = (double)sum / (double)window;
    return (avg < (double)BAR_CAPACITY) ? 1 : 0;
}

static int predict_trend(int week) {
    if (week < 3) return 1;
    int a1 = weekly_attendance[week - 1];
    int a2 = weekly_attendance[week - 2];
    int a3 = weekly_attendance[week - 3];
    int est = a1 + (a1 - a2) + ((a1 - a2) - (a2 - a3)) / 2;
    return (est < BAR_CAPACITY) ? 1 : 0;
}

static int predict_mirror(int week) {
    if (week < 1) return 1;
    return (weekly_attendance[week - 1] >= BAR_CAPACITY) ? 1 : 0;
}

static int predict_fixed_low(void) { return 1; }
static int predict_fixed_high(void) { return 0; }
static int predict_random(void) { return (rand() % 100) < 60 ? 1 : 0; }

int main(void) {
    printf("=== El Farol Bar Problem (Arthur 1994) ===\n");
    printf("Agents: %d, Bar capacity: %d, Weeks: %d\n\n",
           NUM_AGENTS, BAR_CAPACITY, NUM_WEEKS);

    srand(12345);
    memset(weekly_attendance, 0, sizeof(weekly_attendance));

    el_farol_agent_t agents[NUM_AGENTS];
    memset(agents, 0, sizeof(agents));

    for (int a = 0; a < NUM_AGENTS; a++) {
        for (int p = 0; p < NUM_PREDICTORS; p++) {
            agents[a].predictors[p] = rand() % 7;
            agents[a].predictor_accuracy[p] = 5;
        }
        agents[a].chosen_predictor = rand() % NUM_PREDICTORS;
    }

    printf("Week | Attendance | Crowded? | Notes\n");
    printf("-----|------------|----------|------\n");

    for (int week = 1; week <= NUM_WEEKS; week++) {
        int attendance = 0;

        for (int a = 0; a < NUM_AGENTS; a++) {
            int best_predictor = 0;
            int best_accuracy = -1;
            for (int p = 0; p < NUM_PREDICTORS; p++) {
                if (agents[a].predictor_accuracy[p] > best_accuracy) {
                    best_accuracy = agents[a].predictor_accuracy[p];
                    best_predictor = p;
                }
            }
            agents[a].chosen_predictor = best_predictor;

            int prediction;
            int ptype = agents[a].predictors[best_predictor];
            switch (ptype) {
                case 0: prediction = predict_cyclic(week, 7); break;
                case 1: prediction = predict_average(week, 4); break;
                case 2: prediction = predict_trend(week); break;
                case 3: prediction = predict_mirror(week); break;
                case 4: prediction = predict_fixed_low(); break;
                case 5: prediction = predict_fixed_high(); break;
                default: prediction = predict_random(); break;
            }

            if (prediction == 1) { agents[a].attended = 1; attendance++; }
            else agents[a].attended = 0;
        }

        weekly_attendance[week] = attendance;
        int crowded = (attendance > BAR_CAPACITY) ? 1 : 0;

        if (crowded) {
            for (int a = 0; a < NUM_AGENTS; a++) {
                if (agents[a].attended)
                    agents[a].predictor_accuracy[agents[a].chosen_predictor]--;
                else
                    agents[a].predictor_accuracy[agents[a].chosen_predictor]++;
            }
        } else {
            for (int a = 0; a < NUM_AGENTS; a++) {
                if (agents[a].attended)
                    agents[a].predictor_accuracy[agents[a].chosen_predictor]++;
                else
                    agents[a].predictor_accuracy[agents[a].chosen_predictor]--;
            }
        }

        if (week % 20 == 1 || week <= 5) {
            printf(" %3d  |    %3d     |   %s    |",
                   week, attendance, crowded ? "YES" : "no ");
            if (abs(attendance - BAR_CAPACITY) <= 5)
                printf(" Near equilibrium!");
            printf("\n");
        }
    }

    double mean = 0.0, variance = 0.0;
    for (int w = MEMORY_LENGTH + 1; w <= NUM_WEEKS; w++)
        mean += (double)weekly_attendance[w];
    mean /= (double)(NUM_WEEKS - MEMORY_LENGTH);
    for (int w = MEMORY_LENGTH + 1; w <= NUM_WEEKS; w++) {
        double diff = (double)weekly_attendance[w] - mean;
        variance += diff * diff;
    }
    variance /= (double)(NUM_WEEKS - MEMORY_LENGTH);

    printf("\n=== Analysis ===\n");
    printf("Mean attendance:     %.1f\n", mean);
    printf("Std deviation:       %.1f\n", sqrt(variance));
    printf("Bar capacity:        %d\n", BAR_CAPACITY);
    int good_weeks = 0;
    for (int w = 1; w <= NUM_WEEKS; w++)
        if (weekly_attendance[w] <= BAR_CAPACITY) good_weeks++;
    printf("Attendance at/below capacity: %d/%d (%.1f%%)\n",
           good_weeks, NUM_WEEKS, 100.0 * (double)good_weeks / (double)NUM_WEEKS);

    printf("\n=== SFI Methodology Insight ===\n");
    printf("Arthur's El Farol Problem demonstrates that:\n");
    printf("1. With bounded rationality, agents use inductive reasoning.\n");
    printf("2. No predictor is universally best (the 'bar' adapts back).\n");
    printf("3. Attendance self-organises around the threshold.\n");
    printf("4. This is emergent coordination WITHOUT central planning.\n");
    printf("5. The problem is computationally irreducible -- you must\n");
    printf("   simulate to know the outcome (SFI generative methodology).\n");

    return 0;
}
