# Knowledge Graph — Edge of Chaos & Criticality

## L1: Definitions (Complete)

| # | Definition | C Type | Lean Definition | Source |
|---|-----------|--------|-----------------|--------|
| 1 | Phase/Dynamical Regime | `EOCPhaseState` (FROZEN..TRANSCRITICAL) | — | Langton 1990, Wolfram 1984 |
| 2 | Bifurcation Types | `BifurcationType` (SADDLE_NODE..BOGDANOV_TAKENS) | — | Strogatz 2018 |
| 3 | Wolfram CA Classes | `WolframClass` (I..IV) | — | Wolfram 1984 |
| 4 | Universality Classes | `UniversalityClass` (DP..BRANCHING) | — | Stanley 1987 |
| 5 | Critical Exponents | `EOCCriticalExponents` (alpha,beta,gamma,...) | — | Cardy 1996 |
| 6 | Power-Law Fit Result | `EOCPowerLawFit` | — | Clauset et al. 2009 |
| 7 | SOC Sandpile Model | `EOCSandpileModel` | — | Bak et al. 1987 |
| 8 | Correlation Integral | `EOCCorrelationIntegral` | — | Grassberger-Procaccia 1983 |
| 9 | Multifractal Spectrum | `EOCMultifractalSpectrum` | — | Halsey et al. 1986 |
| 10 | Phase Space Point | `EOCPhasePoint` | — | Eckmann-Ruelle 1985 |

## L2: Core Concepts (Complete)

| # | Concept | Implementation | Reference |
|---|---------|---------------|-----------|
| 1 | Edge of Chaos | `eoc_langton_phase_transition()` | Langton 1990 |
| 2 | Self-Organized Criticality | `eoc_sandpile_drive()`, `eoc_sandpile_is_critical()` | Bak et al. 1987 |
| 3 | Phase Transition at Criticality | `EOCCriticalExponents`, scaling relations | Stanley 1987 |
| 4 | Universality | `eoc_universality_exponents()` | Cardy 1996 |
| 5 | Power-Law Distributions | `eoc_powerlaw_fit_*()` | Clauset et al. 2009 |
| 6 | Bifurcation/Chaos | `eoc_bifurcation_diagram()` | Strogatz 2018 |
| 7 | Lyapunov Exponents | `eoc_lyapunov_*()` | Wolf et al. 1985 |
| 8 | Fractal Dimension | `eoc_box_counting_dimension()`, `eoc_correlation_dimension()` | Mandelbrot 1982 |
| 9 | Renormalization Group | `eoc_rg_decimation_1d_ising()` | Wilson 1971 |
| 10 | Avalanche Dynamics | `eoc_sandpile_add_grain_*()` | Dhar 1990 |

## L3: Mathematical Structures (Complete)

| # | Structure | Implementation |
|---|-----------|---------------|
| 1 | Integer lattice with topplings | `EOCSandpileModel.heights[]` |
| 2 | Logistic map (iterated quadratic) | `eoc_logistic_map()` |
| 3 | Lorenz system (3D ODE) | `eoc_lorenz()` |
| 4 | Rossler system (3D ODE) | `eoc_rossler()` |
| 5 | Henon map (2D discrete) | `eoc_henon_map()` |
| 6 | Wolfram 1D CA (k=2,r=1) | `eoc_ca_rule_eval()` |
| 7 | Conway Game of Life (2D CA) | `eoc_game_of_life_step()` |
| 8 | Box-counting grid hierarchy | `eoc_box_counting_2d()` |
| 9 | Correlation integral C(epsilon) | `eoc_correlation_dimension()` |
| 10 | Multifractal partition function | `eoc_multifractal_spectrum()` |

## L4: Fundamental Laws (Complete)

| # | Law/Theorem | Implementation | Status |
|---|------------|---------------|--------|
| 1 | Dhar's Abelian Sandpile Theorem | `eoc_sandpile_add_grain_btw()` | ✅ Verified |
| 2 | Feigenbaum Universality (delta=4.669, alpha=2.502) | `eoc_feigenbaum_delta()`, `eoc_feigenbaum_alpha()` | ✅ Approximated |
| 3 | Lyapunov exponent: lambda = lim (1/t)*ln(|dx|/|dx0|) | `eoc_lyapunov_rosenstein()`, `eoc_lyapunov_wolf()`, `eoc_rk4_lyapunov()` | ✅ Implemented |
| 4 | Kaplan-Yorke Dimension Conjecture | `eoc_kaplan_yorke_dimension()` | ✅ Implemented |
| 5 | Gottwald-Melbourne 0-1 Test for Chaos | `eoc_zero_one_test()` | ✅ Implemented |
| 6 | Rushbrooke Scaling: alpha+2beta+gamma=2 | `eoc_scaling_rushbrooke()` | ✅ Verified |
| 7 | Widom Scaling: gamma=beta*(delta-1) | `eoc_scaling_widom()` | ✅ Verified |
| 8 | Fisher Scaling: nu*(2-eta)=gamma | `eoc_scaling_fisher()` | ✅ Verified |
| 9 | Hyperscaling: d*nu=2-alpha (d<=4) | `eoc_hyperscaling()` | ✅ Verified |
| 10 | Power-law MLE: alpha=1+n/sum(ln(x_i/x_min)) | `eoc_powerlaw_fit_continuous()` | ✅ Implemented |
| 11 | Gutenberg-Richter Law (SOC earthquake freq) | `eoc_sandpile_fit_powerlaw()` | ✅ Verified |
| 12 | RG Flow to Fixed Points (Wilson 1971) | `eoc_rg_decimation_1d_ising()`, `eoc_rg_flow_diagram()` | ✅ Implemented |

## L5: Algorithms/Methods (Complete)

| # | Algorithm | Implementation | Complexity |
|---|-----------|---------------|------------|
| 1 | BTW Sandpile Cascade Toppling | `eoc_sandpile_add_grain_btw()` | O(L^2) worst |
| 2 | Manna Stochastic Sandpile | `eoc_sandpile_add_grain_manna()` | O(L^2) expected |
| 3 | Oslo Ricepile (variable threshold) | `eoc_sandpile_add_grain_oslo()` | O(L^2) expected |
| 4 | MLE Power-Law Fitting (Continuous) | `eoc_powerlaw_fit_continuous()` | O(n) |
| 5 | MLE Power-Law Fitting (Discrete/Zipf) | `eoc_powerlaw_fit_discrete()` | O(n) |
| 6 | KS-Based x_min Selection | `eoc_powerlaw_fit_auto()` | O(n^2) |
| 7 | Bootstrap Goodness-of-Fit Test | `eoc_powerlaw_bootstrap_pvalue()` | O(B*n*log n) |
| 8 | Likelihood Ratio (Vuong's Test) | `eoc_powerlaw_vs_exponential()`, `*_lognormal` | O(n) |
| 9 | Bifurcation Diagram Construction | `eoc_bifurcation_diagram()` | O(n_r*n_trans*n_keep) |
| 10 | Lyapunov Spectrum (Benettin QR) | `eoc_lyapunov_spectrum()` | O(n*d^3) |
| 11 | RK4 ODE Integrator | `eoc_rk4_integrate()` | O(n_steps*d) |
| 12 | Box-Counting Dimension (Grid Hash) | `eoc_box_counting_dimension()` | O(n_eps*n) |
| 13 | Correlation Dimension (Grassberger-Procaccia) | `eoc_correlation_dimension()` | O(N^2) |
| 14 | Higuchi Fractal Dimension (Time Series) | `eoc_higuchi_fractal_dimension()` | O(n*k_max) |
| 15 | Detrended Fluctuation Analysis (DFA) | `eoc_dfa_exponent()` | O(n*log n) |
| 16 | Multifractal Spectrum f(alpha) | `eoc_multifractal_spectrum()` | O(n_q*N^2) |
| 17 | Wolfram CA Classification | `eoc_ca_classify_1d()` | O(width*n_steps) |
| 18 | Wuensche Basin of Attraction | `eoc_ca_basin_analysis()` | O(2^width) |
| 19 | Data Collapse (FSS) | `eoc_fss_data_collapse()` | O(n_L*n_T) |
| 20 | Block-Spin RG (2D Ising) | `eoc_rg_block_spin_2d_ising()` | O(n_iter*L^2) |

## L6: Canonical Problems (Complete)

| # | Problem | Implementation |
|---|---------|---------------|
| 1 | BTW Sandpile Avalanche Distribution | `eoc_sandpile_distribution()`, `examples/example_sandpile.c` |
| 2 | Logistic Map Bifurcation Diagram | `eoc_bifurcation_diagram()`, `examples/example_logistic_map.c` |
| 3 | Langton's Lambda Phase Transition | `eoc_langton_phase_transition()`, `examples/example_cellular_automata.c` |
| 4 | Conway Game of Life Patterns | `eoc_game_of_life_step()`, `examples/example_cellular_automata.c` |
| 5 | Lorenz Strange Attractor | `eoc_lorenz()`, `examples/example_logistic_map.c` |
| 6 | 1/f Noise Spectrum from SOC | `eoc_sandpile_flicker_noise()` |
| 7 | Power-Law Fitting (real data) | `eoc_powerlaw_fit_continuous()`, `examples/example_critical_phenomena.c` |
| 8 | Langton's Ant Emergent Highway | `eoc_langtons_ant()` |
| 9 | Fractal Dimension of Binary Patterns | `eoc_box_counting_2d()`, `examples/example_critical_phenomena.c` |
| 10 | DFA of 1/f Noise Time Series | `eoc_dfa_exponent()`, `examples/example_critical_phenomena.c` |

## L7: Applications (Partial: 4/6)

| # | Application | Implementation | Status |
|---|------------|---------------|--------|
| 1 | Earthquake Modeling (Gutenberg-Richter) | `eoc_sandpile_fit_powerlaw()` with SOC avalanche data | Complete |
| 2 | Financial Market Crash Modeling | Power-law fit for return distributions | Partial (fitting infrastructure ready) |
| 3 | Neural Criticality (Neuronal Avalanches) | Branching process analysis, avalanche statistics | Partial (analysis infrastructure ready) |
| 4 | Forest Fire / Epidemic Models | Sandpile model as universal SOC class | Partial |
| 5 | Internet Traffic Modeling | DFA long-range correlations, 1/f noise | Partial |
| 6 | Urban Growth (DLA fractals) | Fractal dimension analysis | Partial |

## L8: Advanced Topics (Partial: 3/6)

| # | Topic | Implementation | Status |
|---|-------|---------------|--------|
| 1 | Manna Universality Class | `eoc_sandpile_add_grain_manna()` + DP exponents | Complete |
| 2 | Directed Percolation Universality | `eoc_universality_exponents(UNIV_DIRECTED_PERCOLATION)` | Complete |
| 3 | Multifractal Formalism f(alpha) | `eoc_multifractal_spectrum()` | Complete |
| 4 | Griffiths Phases and Rare Regions | Structure factor computation | Partial |
| 5 | Quenched Disorder in SOC | Oslo ricepile model | Partial |
| 6 | RG for Non-Equilibrium Systems | RG flow infrastructure | Partial |

## L9: Research Frontiers (Partial: documented)

| # | Topic | Documentation | Status |
|---|-------|--------------|--------|
| 1 | SOC in Quantum Systems | Referenced in course-tree.md | Documented |
| 2 | Machine Learning at Criticality | Referenced in course-tree.md | Documented |
| 3 | COVID as SOC Phenomenon | Power-law fitting applicable | Documented |
| 4 | Climate as Critical System | DFA for climate time series | Documented |
