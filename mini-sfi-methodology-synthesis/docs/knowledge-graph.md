# Knowledge Graph — SFI Methodology Synthesis

## L1: Definitions
| Entry | C Struct | Lean Definition |
|-------|----------|-----------------|
| Agent | sfi_agent_t | Agent |
| Complex Adaptive System | sfi_environment_t, sfi_population_t | CASState |
| Interaction Topology | sfi_topology_t (enum) | — |
| Emergence Type | sfi_emergence_type_t (enum) | EmergenceType |
| NK Fitness Landscape | sfi_nk_landscape_t | NKLandscape |
| Replicator Dynamics | sfi_replicator_state_t | — |
| Classifier Rule | sfi_classifier_t | — |
| Causal State | sfi_causal_state_t | — |
| Epsilon-Machine | sfi_epsilon_machine_t | EpsilonMachine |
| Network | sfi_network_t | — |
| Community | sfi_community_t, sfi_community_set_t | — |
| Order Parameter | sfi_order_parameter_t | — |
| Complexity Profile | sfi_complexity_profile_t | — |
| Lambda Profile | sfi_lambda_profile_t | — |
| Emergence Detector | sfi_emergence_detector_t | — |
| CSSR Config | sfi_cssr_config_t | — |
| Causal Architecture | sfi_causal_architecture_t | — |
| Network Statistics | sfi_network_stats_t | — |
| Macrostate | sfi_macrostate_t | — |
| Bounded Rationality | — | BoundedRationality |
| Self-Organised Criticality | — | SOCSignature |
| Complexity Level (Transitions) | — | ComplexityLevel |

## L2: Core Concepts
| Concept | Implementation |
|---------|---------------|
| Macrostate/Microstate Duality | sfi_compute_macrostate() |
| Weak Emergence (Bedau) | sfi_detect_emergence() |
| Strong Emergence (Downward Causation) | sfi_detect_emergence() TE macro→micro |
| Bottom-Up Self-Organisation | Sugarscape example |
| Path Dependence / Increasing Returns | El Farol example |
| Tipping Points | Schelling example |
| Bounded Rationality (Simon) | El Farol agents |
| Generative Social Science (Epstein) | All ABM examples |
| Inductive Reasoning | El Farol predictors |
| Co-Evolution / Dancing Landscapes | sfi_landscape_dynamics_t |
| Edge of Chaos (Langton) | sfi_compute_lambda() |
| Self-Organised Criticality (Bak) | sfi_fit_power_law() |
| Scale-Free Networks | sfi_topology_generate_scale_free() |
| Small-World Networks | sfi_topology_generate_small_world() |
| Preferential Attachment | sfi_network_generate_barabasi_albert() |
| Agent Heterogeneity | sfi_agent_t with per-agent strategy |

## L3: Mathematical Structures
| Structure | Representation |
|-----------|---------------|
| Agent State Space | (x, y, energy, strategy[128], genotype[128]) |
| Interaction Graph | Adjacency list (sfi_neighbour_t[]) |
| Resource Landscape | 2-D torus double[] with Laplacian diffusion |
| Fitness Function | NK lookup table (fitness_table[2^N * (K+1)]) |
| Payoff Matrix | n×n double matrix for replicator dynamics |
| Causal State Partition | Set of histories with equivalent futures |
| Epsilon-Machine Transition | Deterministic: (state, symbol) → next state |
| Network Adjacency | uint32_t** neighbour lists |
| Power-Law Distribution | MLE fit with KS test |
| Information-Theoretic Measures | MI, TE, entropy rate, excess entropy |

## L4: Fundamental Laws/Theorems
| Theorem | Implementation | Lean Proof |
|---------|---------------|------------|
| NK Complexity Catastrophe (Kauffman) | sfi_nk_autocorrelation(), sfi_nk_count_local_optima() | nk_fujiyama_single_peak |
| Langton Lambda Phase Transition | sfi_compute_lambda() | langton_frozen_at_zero, langton_chaotic_at_max |
| Schema Theorem (Holland) | sfi_schema_theorem_expected_copies() | schema_theorem_lower_bound |
| Power-Law Degree Distribution (BA model) | sfi_network_generate_barabasi_albert() | pref_attach_positive |
| Unifilarity Property (Crutchfield) | sfi_epsilon_machine_is_unifilar() | unifilarity_deterministic |
| Epsilon-Machine Minimality | sfi_epsilon_machine_minimise() | bisimilar_merge_reduces_states |
| Replicator Dynamics Fixed Point | sfi_replicator_is_fixed_point() | replicator_fixed_point |
| Newman-Girvan Modularity | sfi_network_modularity() | modularity_Q |
| Bounded Rationality (Simon) | — | bounded_rationality_satisficing |
| Major Transitions Increase Complexity | — | transition_increases_complexity |

## L5: Algorithms/Methods
| Algorithm | Implementation |
|-----------|---------------|
| Agent-Based Simulation Loop | sfi_macrosystems.c (full ABM engine) |
| Sugarscape Diffusion/Regrowth | sfi_environment_diffuse(), sfi_environment_regrow() |
| NK Landscape Generation | sfi_nk_landscape_generate() |
| Greedy Adaptive Walk | sfi_nk_greedy_walk() |
| Replicator Dynamics (Euler) | sfi_replicator_step() |
| Bucket Brigade (Classifier) | sfi_classifier_bucket_brigade() |
| CSSR (Causal-State Splitting) | sfi_cssr_reconstruct() |
| Mutual Information (Histogram) | sfi_mutual_information_micro_macro() |
| Transfer Entropy | sfi_transfer_entropy() |
| Power-Law Fitting (MLE+KS) | sfi_fit_power_law() |
| Hurst Exponent (R/S) | sfi_hurst_exponent_rs() |
| Gini Coefficient | sfi_gini_coefficient() |
| Multi-Scale Entropy | sfi_multiscale_entropy() |
| Lempel-Ziv Complexity | sfi_lempel_ziv_complexity() |
| Kolmogorov Approx (RLE) | sfi_kolmogorov_approx_gzip() |
| Entropy Rate (Block) | sfi_entropy_rate() |
| Langton Lambda Computation | sfi_compute_lambda() |
| Wolfram Class Estimation | sfi_wolfram_class_estimate() |
| Box-Counting Dimension | sfi_box_counting_dimension() |
| Largest Lyapunov Exponent | sfi_largest_lyapunov() |
| ER Random Graph | sfi_network_generate_erdos_renyi() |
| BA Scale-Free Network | sfi_network_generate_barabasi_albert() |
| WS Small-World Network | sfi_network_generate_watts_strogatz() |
| Newman-Girvan Communities | sfi_network_community_girvan_newman() |
| Betweenness Centrality (Brandes) | sfi_network_betweenness_centrality() |
| Closeness Centrality | sfi_network_closeness_centrality() |
| PageRank | sfi_network_pagerank() |
| Epsilon-Machine Simulation | sfi_epsilon_machine_simulate() |
| Epsilon-Machine Merge | sfi_epsilon_machine_merge() |
| Epsilon-Machine Stationary Dist | sfi_epsilon_machine_stationary() |
| Causal Architecture | sfi_causal_architecture_compute() |
| Emergence Detection Protocol | sfi_detect_emergence() |

## L6: Canonical Problems
| Problem | Example |
|---------|---------|
| Sugarscape (Epstein & Axtell) | example_sugarscape_simulation.c |
| El Farol Bar (Arthur) | example_el_farol_bar.c |
| Schelling Segregation | example_schelling_segregation.c |
| NK Adaptive Walk | tests/test_sfi_adaptation.c |

## L7: Applications
| Application | Implementation |
|-------------|---------------|
| Wealth Inequality Dynamics | Gini in Sugarscape macrostate |
| Market Coordination (No Central Planner) | El Farol emergent equilibrium |
| Residential Segregation | Schelling tipping point |
| Epidemic Spreading (ABM) | Population with interaction topology |
| Social Network Influence | PageRank + betweenness centrality |

## L8: Advanced Topics
| Topic | Implementation |
|-------|---------------|
| Computational Mechanics | sfi_epsilon_machine.c (full CSSR) |
| Causal Architecture | sfi_causal_architecture_compute() |
| Complexity-Entropy Diagram | CEPhase induction + C-H ratio |
| Open-Ended Evolution | open_ended_evolution (Lean) |
| Major Evolutionary Transitions | ComplexityLevel (Lean) |
| Epsilon-Machine Merging | sfi_epsilon_machine_merge() |
| Multi-Scale Entropy (Costa) | sfi_multiscale_entropy() |
| Effective Complexity (Gell-Mann) | sfi_complexity_profile_t |

## L9: Research Frontiers
| Frontier | Documentation |
|----------|--------------|
| Digital Twin Synchronisation | digital_twin_sync_condition (Lean) |
| AI Agent Societies | Referenced in course-alignment.md |
| Multi-Scale ABM | Referenced in coverage-report.md |
| Computational Social Science | Referenced in gap-report.md |
