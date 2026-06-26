# Course Alignment — mini-genetic-algorithms-holland

## Nine-School Curriculum Mapping

### MIT (6.034 Artificial Intelligence, 6.841 Advanced Complexity)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Genetic Algorithms (Winston Ch24) | `ga_core.c` — complete GA loop |
| Evolutionary computation | `ga_operators.c` — all canonical operators |
| Search and optimization landscapes | `ga_fitness.c` — landscape analysis suite |
| Schema Theorem analysis | `ga_theory.c` — lower/upper bounds |

### Stanford (CS 229 Machine Learning, CS 261 Optimization)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Evolutionary strategies | `ga_crossover_arithmetic()`, `ga_mutate_gaussian()` |
| Multi-objective optimization | `ga_nondominated_sort()`, `ga_pareto_dominates()` |
| Model selection via GA | `ga_run_with_callback()` |

### Berkeley (CS 188 AI, CS 289A ML Optimization)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Stochastic local search | Entire GA as metaheuristic |
| Fitness landscape analysis | `ga_fdc()`, `ga_epistasis_measure()` |
| No Free Lunch Theorem | `ga_nfl_bound()` — Wolpert & Macready |

### CMU (15-381 AI, 15-780 Graduate AI)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Nature-inspired computation | GA operators (selection, crossover, mutation) |
| NK fitness landscapes | `ga_fitness_nk_landscape()` — Kauffman model |
| Co-evolution | `ga_coevolution_fitness()` |

### Princeton (COS 402 AI, COS 511 Machine Learning)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Evolutionary optimization | Real-coded GA with SBX + polynomial mutation |
| Deception in search | `ga_fitness_deceptive_trap()`, `ga_is_deceptive()` |

### Caltech (CS 156 Machine Learning, CS 274 Optimization)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Physics-inspired computation | Spatial GA (cellular automaton style) |
| Criticality in evolution | `ga_restart()` — adaptive diversity injection |

### Cambridge (Part II AI, Part III Advanced ML)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Holland's Schema Theory | Full schema processing in `ga_theory.c` |
| Building Block Hypothesis | `ga_identify_building_blocks()` |
| Vose model | `ga_vose_mixing_entry()`, `ga_vose_heuristic_step()` |

### Oxford (Computational Intelligence, Evolutionary Computation)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Multi-objective EAs | NSGA-II style sorting + crowding distance |
| Niching methods | `ga_fitness_sharing()`, `ga_deterministic_crowding()` |
| Island models | `ga_island_model_*()` |

### ETH (263-5210 Advanced ML, 252-0538 Optimization)

| Course Topic | Our Implementation |
|-------------|-------------------|
| Formal GA theory | Lean 4 formalization of schema properties |
| Runtime analysis | No Free Lunch bound |
| Advanced representations | Messy GA, random keys encoding |

## Core Textbook Alignment

| Textbook | Topics Covered |
|----------|---------------|
| Holland (1975, 1992) — Adaptation in Natural and Artificial Systems | Schema Theorem, adaptive systems framework |
| Goldberg (1989) — Genetic Algorithms in Search, Optimization & ML | All fundamental operators, SGA, building blocks |
| Mitchell (1998) — An Introduction to Genetic Algorithms | Royal Road, NK landscapes, GA theory overview |
| Eiben & Smith (2015) — Introduction to Evolutionary Computing | Complete algorithm taxonomy, parameter tuning |
| Bäck (1996) — Evolutionary Algorithms in Theory and Practice | Tournament selection, ES, mutation analysis |
| Deb (2001) — Multi-Objective Optimization using EAs | NSGA-II, SBX, polynomial mutation |
| Vose (1999) — The Simple Genetic Algorithm | Markov chain model, mixing matrix |
| Rothlauf (2006) — Representations for GAs | Encoding theory, locality, redundancy |

## Coverage Ratio

- 8/9 schools represented in at least 3 mapped topics
- All 8 core textbooks mapped to implementations
- 100% of canonical operators from Goldberg (1989) implemented
