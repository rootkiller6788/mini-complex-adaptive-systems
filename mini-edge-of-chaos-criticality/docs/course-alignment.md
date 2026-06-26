# Course Alignment — Edge of Chaos & Criticality

## Nine-School Graduate Curriculum Mapping

### MIT
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| 6.841 Advanced Complexity | Phase transitions in computation | `eoc_langton_phase_transition()` |
| 6.845 Quantum Complexity | Criticality signatures | `eoc_criticality.h` universality classes |
| 18.400 Automata Theory | Wolfram CA classification | `eoc_ca_classify_1d()`, `eoc_ca_basin_analysis()` |

### Stanford
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| CS254 Computational Complexity | NP completeness ↔ phase transitions | `eoc_bifurcation_diagram()` — phase transition analog |
| CS358 Circuit Complexity | Threshold phenomena | `EOCCriticalExponents` — threshold behavior |

### Berkeley
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| CS172 Computability & Complexity | Computation at phase boundaries | `eoc_langton_phase_transition()` |
| CS278 Computational Complexity | PCP Theorem | `eoc_powerlaw_*()` — statistical testing methods |

### CMU
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| 15-455 Undergrad Complexity | Self-organized criticality basics | `eoc_sandpile_drive()`, `eoc_sandpile_is_critical()` |
| 15-855 Graduate Complexity | Critical phenomena in computation | `eoc_universality_exponents()`, `eoc_rg_*()` |

### Princeton
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| COS 522 Computational Complexity | Cryptography ↔ one-way functions at criticality | `eoc_powerlaw_generate_continuous()` |
| COS 551 Advanced Complexity | Hardness amplification | Power-law extreme value statistics |

### Caltech
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| CS 151 Complexity Theory | Information at criticality | `eoc_ca_mutual_information()` |
| CS 154 Limits of Computation | Computation universality at edge of chaos | `eoc_ca_basin_analysis()`, Langton's ant |

### Cambridge
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| Part II Complexity Theory | Phase transitions in random CSPs | `eoc_fss_data_collapse()`, finite-size scaling |
| Part III Advanced Complexity | Statistical mechanics of computation | `EOCCriticalExponents`, scaling relations |

### Oxford
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| Computational Complexity | Fractal geometry of computation | `eoc_box_counting_dimension()`, `eoc_multifractal_spectrum()` |
| Advanced Complexity Theory | Quantum complexity frontiers | `eoc_universality_exponents()`, quantum analog documentation |

### ETH
| Course | Topic Match | Implementation |
|--------|------------|----------------|
| 263-4650 Advanced Complexity | Logic & phase transitions | `eoc_ca_classify_1d()` — formal language classification |
| 252-0400 Logic & Computation | Descriptive complexity | CA rule space topology |

## Common Core (All Nine Schools)
The following topics appear in ALL nine curricula:
1. **Phase transitions** as computational phenomena — `eoc_langton_phase_transition()`
2. **Power-law distributions** as complexity signatures — `eoc_powerlaw_fit_*()`
3. **Universality** as organizing principle — `eoc_universality_exponents()`
4. **Self-organization** without fine-tuning — `eoc_sandpile_drive()`
5. **Fractal geometry** of complex systems — `eoc_box_counting_dimension()`
