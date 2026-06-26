# Course Alignment — mini-artificial-life-langton

## Santa Fe Institute — Complex Systems Summer School (CSS)
| SFI Topic | This Module |
|-----------|------------|
| Artificial Life (Langton) | `langton_ant.c` — complete Langton Ant simulation |
| Cellular Automata | `cellular_automaton.c` — 1D/2D CA engine |
| Edge of Chaos | `langton_lambda.c` — λ parameter and phase transition |
| Emergence | `alife_metrics.c` — emergence, complexity, self-organization metrics |
| Genetic Algorithms | `alife_evolution.c` — GA for CA rule evolution |
| Pattern Formation | `alife_patterns.c` — pattern detection and classification |
| Agent-Based Modeling | `lant_sim_t` — multi-ant simulation with interactions |

## MIT — 6.045/18.400 Automata, Computability, and Complexity
| MIT Topic | This Module |
|-----------|------------|
| Turing Machines | Langton Ant as Turing-complete system (`langton_ant_is_universal` in Lean) |
| Decidability | CA prediction problem (undecidable for general CA) |
| Reducibility | Game of Life → Turing machine reduction |

## MIT — 6.841 Advanced Complexity Theory
| MIT Topic | This Module |
|-----------|------------|
| Circuit Complexity | CA as uniform circuit family (Wolfram classification) |
| Kolmogorov Complexity | `alife_lempel_ziv_complexity()` |
| Information Theory | Shannon entropy, mutual information, KL divergence |

## Stanford — CS358 Circuit Complexity
| Stanford Topic | This Module |
|---------------|------------|
| Boolean Circuits | CA rule tables as Boolean functions |
| Circuit Lower Bounds | (Documented in L9 — natural proofs barrier connection) |
| NC Hierarchy | CA rule space classification by computational power |

## Berkeley — CS172 Computability and Complexity
| Berkeley Topic | This Module |
|---------------|------------|
| Turing Machines | Langton Ant universality |
| Undecidability | CA prediction/halting problems |
| NP-Completeness | (Documented — density classification as NP-hard for general CA) |

## CMU — 15-855 Graduate Complexity Theory
| CMU Topic | This Module |
|-----------|------------|
| PCP Theorem | (L9 documented connection to CA tiling problems) |
| Average-Case Complexity | Fitness landscape ruggedness (Kauffman NK model) |
| Derandomization | (Documented — CA as pseudorandom generators) |

## Princeton — COS 522 Computational Complexity
| Princeton Topic | This Module |
|----------------|------------|
| Interactive Proofs | (Documented — CA as verifier in MIP) |
| Cryptography | (Documented — CA-based stream ciphers) |
| Complexity of Counting | Glider counting as #P-complete problem |

## Caltech — CS 151 Complexity Theory
| Caltech Topic | This Module |
|--------------|------------|
| Cellular Automata Universality | Rule 110, Game of Life, Langton Ant universality |
| Self-Organization | `alife_self_organization()` |
| Physics of Computation | Thermodynamic depth, integrated information (Φ) |

## Cambridge — Part II Complexity Theory
| Cambridge Topic | This Module |
|----------------|------------|
| Complexity Classes | CA computational power (P, PSPACE, undecidable) |
| Space Complexity | CA memory requirements (O(width × height)) |
| Alternation | (Documented — alternating CA) |

## Oxford — Computational Complexity
| Oxford Topic | This Module |
|-------------|------------|
| Proof Complexity | (Documented — CA as proof systems) |
| Quantum Complexity | (L9 documented — Quantum CA) |
| Parameterized Complexity | Fitness landscape as parameterized problem |

## ETH — 263-4650 Advanced Complexity
| ETH Topic | This Module |
|-----------|------------|
| Logic and Computation | Lean 4 formalization of CA properties |
| Descriptive Complexity | (Documented — CA definability in logical fragments) |
| Algebraic Complexity | (Documented — CA rule space as algebraic structure) |
