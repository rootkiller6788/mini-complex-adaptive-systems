# Knowledge Graph — mini-artificial-life-langton

## L1: Definitions (Complete)

| # | Concept | C Implementation | Lean Formalization |
|---|---------|-----------------|-------------------|
| 1 | Langton's Ant | `langton_ant_t`, `lant_color_t`, `lant_dir_t` — `langton_ant.h` | `LantAnt`, `LantDirection`, `LantColor` |
| 2 | Cellular Automaton Grid | `langton_grid_t` — `langton_ant.h` | `LantGrid` |
| 3 | CA State Space (1D/2D) | `ca_state_t`, `ca_config_t` — `cellular_automaton.h` | `CA1DState`, `CA2DState` |
| 4 | Langton's λ parameter | `lambda_config_t`, `lambda_rule_t` — `langton_lambda.h` | `lambdaParameter` |
| 5 | Wolfram Classes (I-IV) | `ca_wolfram_class_t` — `cellular_automaton.h` | (enum via CA classifications) |
| 6 | Emergence metric | `alife_emergence_t` — `alife_metrics.h` | (derived from entropy analysis) |
| 7 | Self-organization | `alife_self_organization()` — `alife_metrics.h` | (derived from spatial autocorrelation) |
| 8 | Pattern types | `pattern_type_t`, `pattern_instance_t` — `alife_patterns.h` | (cataloged patterns) |

## L2: Core Concepts (Complete)

| # | Concept | Implementation |
|---|---------|---------------|
| 1 | Ant movement and turning | `lant_ant_step()`, `lant_ant_turn()`, `turnRight`, `turnLeft` |
| 2 | Grid access with boundaries | `lant_grid_get()`, `ca_state_get()` |
| 3 | Toroidal vs fixed boundaries | `CA_BOUNDARY_PERIODIC`, `CA_BOUNDARY_FIXED`, `CA_BOUNDARY_REFLECTIVE` |
| 4 | Neighborhood types | `CA_NEIGH_VON_NEUMANN`, `CA_NEIGH_MOORE`, `CA_NEIGH_EXTENDED_MOORE` |
| 5 | Rule types (elementary/totalistic) | `ca_rule_type_t`, `ca_rule_elementary_t`, `ca_rule_totalistic_t` |
| 6 | High-level simulation | `lant_sim_t`, `ca_evolve_step()` |
| 7 | Pattern detection via Zobrist hashing | `pattern_zobrist_hash()`, `pattern_canonical_hash()` |
| 8 | Entropy and information measures | `alife_shannon_entropy()`, `alife_mutual_information()` |

## L3: Mathematical Structures (Complete)

| # | Structure | Implementation |
|---|-----------|---------------|
| 1 | Z^2 grid topology | `Int × Int` in Lean, `(x,y)` coordinates in C |
| 2 | Boolean algebra on cell states | `LantColor` (2-element type), `Fin 2` |
| 3 | Rule table as function space k^N → k | `uint8_t *rule_table`, `rule_lut` |
| 4 | Cyclic group Z_4 acting on directions | `turnRight`/`turnLeft` forming Z_4 group — Lean proofs |
| 5 | Moore neighborhood as 8-element set | `mooreNeighbors` returning List of 8 positions |
| 6 | Spacetime as N × Z^d | 1D CA history: `ca_save_spacetime_pgm()` |
| 7 | Covariance matrix for colony analysis | `lant_colony_anisotropy()` (PCA via eigenvalue decomposition) |
| 8 | Probability distributions on CA states | `alife_shannon_entropy()`, `alife_kl_divergence()` |

## L4: Fundamental Laws (Complete)

| # | Theorem/Law | C Verification | Lean Formalization |
|---|------------|---------------|-------------------|
| 1 | Langton Ant unbounded growth (Bunimovich & Troubetzkoy 1992) | Highway detection via `lant_detect_highway()` | `langton_ant_unbounded` |
| 2 | Langton Ant Turing completeness (Gajardo et al. 2006) | Documented in `langton_ant.c` | `langton_ant_is_universal` |
| 3 | Turn operations form Z_4 group | Verified in `test_ant_turn` | `turnRight_four_times_identity`, `turnLeft_four_times_identity` |
| 4 | Color flip is involutive (f(f(x)) = x) | `lant_grid_flip()` unit test | `flipColor_involutive` |
| 5 | Edge of chaos hypothesis (Langton 1990) | `lambda_detect_edge_of_chaos()` | `lambda_nonneg`, `lambda_le_one` |
| 6 | Moore neighbor sum bounded by 8 | `ca_neighbor_sum_2d()` | `mooreNeighborSum_bounded` |
| 7 | Game of Life all-dead stability | `gol_all_dead_stable` test | `gol_all_dead_stable` |
| 8 | One step increments counters by exactly 1 | `test_sim_run_steps` | `langtonStep_increments_steps`, `langtonStep_increments_flips` |

## L5: Algorithms/Methods (Complete)

| # | Algorithm | Implementation |
|---|-----------|---------------|
| 1 | Langton Ant step simulation | `lant_ant_step()` — O(1) per step, O(wh) for grid expansion |
| 2 | Fisher-Yates rule generation | `lambda_rule_generate()` |
| 3 | Lambda-constrained random walk mutation | `lambda_rule_mutate_preserve_lambda()` |
| 4 | Phase transition sweeping | `lambda_sweep_phase()` |
| 5 | 1D elementary CA evolution | `ca_evolve_1d_elementary()` |
| 6 | 2D totalistic/outer-totalistic CA evolution | `ca_evolve_2d_totalistic()`, `ca_evolve_2d_outer_totalistic()` |
| 7 | Wolfram classification via entropy+damage | `ca_classify_wolfram_1d()` |
| 8 | Highway detection via autocorrelation | `lant_detect_highway()`, `lant_find_highway_period()` |
| 9 | Floyd's cycle-finding for pattern periods | `pattern_detect_period()` |
| 10 | Flood-fill connected component extraction | `flood_fill_component()` (static) |
| 11 | Zobrist hashing for translation-invariant matching | `pattern_zobrist_hash()`, `pattern_canonical_hash()` |
| 12 | Tournament selection genetic algorithm | `alife_tournament_select()` |
| 13 | Crossover and mutation operators on CA rules | `alife_crossover()`, `alife_mutate()` |
| 14 | Lempel-Ziv 1978 complexity | `alife_lempel_ziv_complexity()` |
| 15 | Multi-scale complexity (Bar-Yam) | `alife_multiscale_complexity()` |

## L6: Canonical Problems (Complete)

| # | Problem | Example File |
|---|---------|-------------|
| 1 | Single Langton Ant highway formation | `examples/example_langton_ant.c` |
| 2 | Lambda phase transition analysis | `examples/example_lambda_analysis.c` |
| 3 | Emergence & complexity in Game of Life | `examples/example_emergence.c` |
| 4 | Period detection for oscillators/gliders | (tested in `test_cellular_automaton.c`) |
| 5 | Conway Game of Life blinker evolution | `test_gol_rule()` |
| 6 | Wolfram classification of 1D CA rules | `test_wolfram_classify()` |
| 7 | Highway period detection (canonical = 104) | `lant_find_highway_period()` |
| 8 | Multi-ant colony dynamics | `lant_multi_ant_run()`, `lant_colony_radius()` |

## L7: Applications (Partial+)

| # | Application | Implementation |
|---|------------|---------------|
| 1 | Artificial Life simulation (SFI methodology) | `examples/example_langton_ant.c` — full ant simulation with highway detection |
| 2 | Complex systems phase transition analysis | `examples/example_lambda_analysis.c` — edge of chaos detection |
| 3 | Emergence quantification for complex systems | `examples/example_emergence.c` — comprehensive ALife test |

## L8: Advanced Topics (Partial+)

| # | Topic | Implementation |
|---|-------|---------------|
| 1 | Edge of chaos hypothesis testing | `lambda_test_edge_hypothesis()`, `lambda_critical_exponent()` |
| 2 | Statistical complexity (Crutchfield's ɛ-machine) | `alife_statistical_complexity_calc()` |
| 3 | Multi-scale complexity (Bar-Yam) | `alife_multiscale_complexity()` |
| 4 | Integrated information (Φ) approximation | `alife_phi_integrated_info()` |
| 5 | Genetic algorithm for CA rule evolution | `alife_evolution.c` — full GA with tournament selection |
| 6 | Fitness landscape analysis (Kauffman NK) | `alife_landscape_analyze()` |
| 7 | Open-ended evolution detection | `alife_detect_open_ended()` |
| 8 | Self-reproduction detection | `alife_detect_self_reproduction()` |

## L9: Research Frontiers (Partial)

| # | Topic | Status |
|---|-------|--------|
| 1 | Space-filling ant conjecture | Formalized in Lean (`spaceFillingAntConjecture`) |
| 2 | Exact λ_c for computational complexity maximization | Documented in Lean (`exactLambdaCriticalValue`) |
| 3 | Open-ended evolution at edge of chaos | Documented in Lean (`openEndedEvolutionConjecture`) |
| 4 | Digital physics and CA as computational substrate | Documented in README |
