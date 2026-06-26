# Course Tree ? SFI Methodology Synthesis

## Prerequisites (modules this module depends on)
```
mini-complex-control-theory/
  ?? mini-general-system-theory/
       ?? mini-bertalanffy-open-system/ (general system concepts)
  ?? mini-theory-of-computation/
       ?? (foundational CS theory; no strict dependency)
```

## Dependents (modules that need this one)
```
mini-complex-control-theory/
  ?? 28. mini-complex-adaptive-systems/
       ?? mini-artificial-life-langton/       (uses lambda, CA concepts)
       ?? mini-genetic-algorithms-holland/    (uses schema theorem, NK)
       ?? mini-nk-fitness-landscapes-kauffman/ (shares NK landscape)
       ?? mini-cas-in-economics-arthur/        (uses El Farol, bounded rationality)
       ?? mini-collective-intelligence-swarm/  (uses emergence, self-organisation)
       ?? mini-edge-of-chaos-criticality/     (uses lambda, SOC)
       ?? mini-agents-adaptation-basics/      (uses agent definitions)
       ?? mini-sfi-methodology-synthesis/      ? THIS MODULE (synthesis capstone)
```

## Internal Dependency Graph
```
sfi_macrosystems.h/.c
  ??? sfi_emergence.h/.c (needs macrostate)
  ??? sfi_adaptation.h/.c (needs agent type, independent engine)
  ??? sfi_network_methods.h/.c (needs agent IDs for node mapping)
  ??? sfi_epsilon_machine.h/.c (independent; works on symbolic sequences)

sfi_emergence.h/.c
  ??? (independent statistical library)

sfi_complexity_measures.h/.c
  ??? (independent measures library)

sfi_epsilon_machine.h/.c
  ??? (self-contained; depends only on stdlib)

sfi_network_methods.h/.c
  ??? (self-contained graph library)

sfi_lean.lean
  ??? (no dependencies; Lean 4 core only, no Mathlib)
```

## Knowledge Layer Dependencies
```
L1 (Definitions)
 ??? L2 (Core Concepts)
 ?    ??? L3 (Math Structures)
 ?    ?    ??? L4 (Fundamental Laws) ? requires L1+L2+L3
 ?    ?         ??? L5 (Algorithms) ? implements L4
 ?    ?              ??? L6 (Canonical Problems) ? uses L5
 ?    ?                   ??? L7 (Applications) ? uses L6
 ?    ?                        ??? L8 (Advanced Topics) ? extends L4-L7
 ?    ?                             ??? L9 (Research Frontiers) ? builds on L8
```

## SFI Canonical Reading Order
1. Holland, "Hidden Order" (1995) ? L1-L2 (CAS basics)
2. Kauffman, "Origins of Order" (1993) ? L1-L4 (NK landscape)
3. Epstein and Axtell, "Growing Artificial Societies" (1996) ? L5-L6 (Sugarscape)
4. Arthur, "Increasing Returns and Path Dependence" (1994) ? L2, L6 (El Farol)
5. Schelling, "Micromotives and Macrobehavior" (1978) ? L2, L6 (Segregation)
6. Crutchfield, "The Calculi of Emergence" (1994) ? L3-L8 (Epsilon-machine)
7. Shalizi and Crutchfield, "Computational Mechanics" (2001) ? L5, L8 (CSSR)
8. Bak, "How Nature Works" (1996) ? L4 (SOC)
9. Barabasi, "Network Science" (2016) ? L3, L4 (Scale-free)
10. Miller and Page, "Complex Adaptive Systems" (2007) ? L1-L9 (Synthesis)
