# Course Alignment ? SFI Methodology Synthesis

## Nine-School Curriculum Mapping

### SFI (Santa Fe Institute) ? CSSS Complex Systems Summer School
| Topic | This Module |
|-------|------------|
| Agent-Based Modeling | sfi_macrosystems.c ? Full ABM engine |
| Emergence | sfi_emergence.c ? Detection and quantification |
| Computational Mechanics | sfi_epsilon_machine.c ? CSSR algorithm |
| Network Science | sfi_network_methods.c ? ER/BA/WS generation + analysis |
| Fitness Landscapes | sfi_adaptation.c ? NK model + adaptive walks |
| Evolutionary Game Theory | sfi_adaptation.c ? Replicator dynamics |
| Scaling Laws / SOC | sfi_emergence.c ? Power-law fitting |
| Complexity Measures | sfi_complexity_measures.c ? Multi-metric profile |

### MIT ? 6.841 Advanced Complexity Theory
| Topic | Implementation |
|-------|---------------|
| Information-Theoretic Complexity | sfi_entropy_rate(), sfi_excess_entropy() |
| Kolmogorov Complexity | sfi_kolmogorov_approx_gzip() |
| Emergence and Reduction | sfi_detect_emergence() |

### Stanford ? CS274 Multi-Agent Systems
| Topic | Implementation |
|-------|---------------|
| Agent Architectures | sfi_agent_t with genotype/strategy |
| Interaction Topologies | sfi_topology_t ? 7 types |
| Coordination | El Farol example |
| Emergent Behaviour | Schelling segregation example |

### Berkeley ? CS278 Computational Complexity
| Topic | Implementation |
|-------|---------------|
| Information in Dynamics | sfi_transfer_entropy() |
| Causal Architecture | sfi_causal_architecture_compute() |
| Complexity-Entropy Diagram | CEPhase in Lean |

### CMU ? 24-654 Systems Thinking
| Topic | Implementation |
|-------|---------------|
| Complex Adaptive Systems | Full CAS model (sfi_macrosystems.h) |
| Agent Heterogeneity | Per-agent strategy and genotype |
| Emergence | Multi-metric detection protocol |
| Feedback Loops | Sugarscape resource-agent co-dynamics |

### Princeton ? COS 551 Advanced Complexity
| Topic | Implementation |
|-------|---------------|
| Bounded Rationality | El Farol inductive agents |
| Game Theory Foundations | Replicator dynamics |
| Social Dynamics | Schelling tipping point |

### Caltech ? CS 154 Limits of Computation
| Topic | Implementation |
|-------|---------------|
| Computational Irreducibility | El Farol: must simulate to know outcome |
| Cellular Automata | sfi_compute_lambda(), Wolfram class |
| Langton's Lambda | Full lambda profile computation |

### Cambridge ? Part III Advanced Complexity
| Topic | Implementation |
|-------|---------------|
| Statistical Complexity C_mu | sfi_statistical_complexity() |
| Epsilon-Machines | Full CSSR + minimisation |
| Phase Transitions in CA | Lambda + Wolfram class estimation |

### Oxford ? C20 Adaptive Control / Complex Systems
| Topic | Implementation |
|-------|---------------|
| Adaptation on Landscapes | NK model + greedy adaptive walk |
| Evolutionary Dynamics | Replicator dynamics |
| Self-Organisation | Sugarscape emergent patterns |

### ETH ? 252-0400 Logic and Computation
| Topic | Implementation |
|-------|---------------|
| Formal Definitions | All Lean structures and theorems |
| Unifilarity Proof | Lean: unifilarity_deterministic |
| Minimality Lemma | Lean: bisimilar_merge_reduces_states |
