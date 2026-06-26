# Knowledge Graph — mini-agents-adaptation-basics

## L1: Definitions (Complete)

| # | Concept | C Implementation | Lean Definition |
|---|---------|-----------------|-----------------|
| 1 | Agent | `struct Agent` in agent.h/c | `structure Agent` in adaptation.lean |
| 2 | Percept | `struct Percept` in agent.h/c | `structure Percept` in adaptation.lean |
| 3 | Action | `struct Action` in agent.h/c | `inductive Action` in adaptation.lean |
| 4 | AgentState | `struct AgentState` in agent.h/c | `structure AgentState` in adaptation.lean |
| 5 | PEAS description | `struct PEAS` in agent.h/c | — |
| 6 | BanditArm | `struct BanditArm` in bandit.h/c | — |
| 7 | MultiArmedBandit | `struct MultiArmedBandit` in bandit.h/c | — |
| 8 | NormalFormGame | `struct NormalFormGame` in game_theory.h/c | — |
| 9 | MixedStrategy | `struct MixedStrategy` in game_theory.h/c | `structure MixedStrategy` in adaptation.lean |
| 10 | QTable (action-value function) | `struct QTable` in rl_agent.h/c | `def QValue` in adaptation.lean |
| 11 | Policy | `struct Policy` in rl_agent.h/c | — |
| 12 | MDP | `Environment`, `TransitionModel`, `RewardFunction` in environment.h | `structure MDP` in adaptation.lean |
| 13 | GridWorld | `struct GridWorld` in environment.h/c | — |
| 14 | NKLandscape | `struct NKLandscape` in environment.h/c | `structure NKLandscape` in adaptation.lean |
| 15 | PSO_Particle / Swarm | `struct PSO_Particle`, `PSO_Swarm` in swarm.h/c | — |
| 16 | ACO_Ant / Colony | `struct ACO_Ant`, `ACO_Colony` in swarm.h/c | — |
| 17 | Boid / Flock | `struct Boid`, `Flock` in swarm.h/c | — |
| 18 | Firefly / FireflySwarm | `struct Firefly`, `FireflySwarm` in swarm.h/c | — |
| 19 | ReplicatorState | `struct ReplicatorState` in game_theory.h/c | — |
| 20 | CorrelatedEquilibrium | `struct CorrelatedEquilibrium` in game_theory.h/c | — |
| 21 | PDHistory / PDStrategy | `struct PDHistory`, `PDStrategy` typedef in game_theory.h/c | — |
| 22 | EXP3State (adversarial bandit) | `struct EXP3State` in bandit.h/c | — |
| 23 | ContextualBandit | `struct ContextualBandit` in bandit.h/c | — |

## L2: Core Concepts (Complete)

| # | Concept | Implementation |
|---|---------|---------------|
| 1 | Perception-Action Loop | `agent_step()` in agent.c |
| 2 | Learning from Experience | `agent_learn_from_experience()` in agent.c |
| 3 | Fitness Evaluation | `agent_fitness()` in agent.c |
| 4 | Bounded Rationality (Simon, 1957) | `agent_satisfice()` in agent.c |
| 5 | Aspiration Adaptation | `agent_adapt_aspiration()` in agent.c |
| 6 | Exploration vs Exploitation | `agent_epsilon_greedy()`, `agent_boltzmann_selection()` in agent.c |
| 7 | Policy Entropy | `agent_policy_entropy()` in agent.c |
| 8 | Social Learning / Imitation | `agent_observe_peer()`, `agent_imitate()` in agent.c |
| 9 | Stigmergic Communication | `agent_send_message()` in agent.c |
| 10 | Bandit Pull/Reward | `bandit_pull()`, `bandit_update_arm()` in bandit.c |
| 11 | Environment Step | `env_step()`, `env_observe()` in environment.c |
| 12 | Best Response in Games | `nfg_best_response_p1()` in game_theory.c |
| 13 | Emergence in Flocking | `flock_step()` in swarm.c |
| 14 | Pheromone-based Stigmergy | `aco_evaporate()`, `aco_deposit_pheromone()` in swarm.c |

## L3: Mathematical Structures (Complete)

| # | Structure | Implementation |
|---|-----------|---------------|
| 1 | MDP (S, A, P, R, gamma) | `TransitionModel`, `RewardFunction` in environment.h/c |
| 2 | State Space / Action Space | `StateSpace`, `ActionSpace` in environment.h/c |
| 3 | NK Fitness Landscape | `nk_create()`, `nk_fitness()`, `nk_ruggedness()` in environment.c |
| 4 | Q-function as flat tensor | `QTable` in rl_agent.h/c |
| 5 | Eligibility Trace tensor | `EligibilityTrace` in rl_agent.h/c |
| 6 | Policy as state->action map | `Policy` in rl_agent.h/c |
| 7 | Payoff matrix (bimatrix game) | `NormalFormGame` in game_theory.h/c |
| 8 | Strategy simplex | `MixedStrategy` in game_theory.h/c |
| 9 | Replicator Jacobian | `replicator_jacobian()` in game_theory.c |
| 10 | State similarity metric (cosine) | `agent_state_similarity()` in agent.c |
| 11 | Bayesian state fusion | `agent_state_merge()` in agent.c |
| 12 | Percept discretization / tiling | `agent_discretize_percept()` in agent.c |
| 13 | Flock order parameter (polarization) | `flock_polarization()` in swarm.c |
| 14 | KL divergence | `bandit_kl_divergence()` in bandit.c |
| 15 | Beta distribution (Gamma method) | `rand_beta()` in bandit.c |
| 16 | PSO velocity/position dynamics | `pso_update()` in swarm.c |

## L4: Fundamental Laws (Complete)

| # | Theorem / Law | C Verification | Lean Statement |
|---|--------------|----------------|----------------|
| 1 | Bellman Optimality Equation | `value_iteration()`, `policy_iteration()` in rl_agent.c | `bellman_fixed_point_is_optimal` in adaptation.lean |
| 2 | Q-Learning Convergence (Watkins & Dayan, 1992) | `qlearning_update()` in rl_agent.c | `q_learning_convergence` in adaptation.lean |
| 3 | SARSA Convergence (Singh et al., 2000) | `sarsa_update()` in rl_agent.c | `sarsa_update` in adaptation.lean |
| 4 | TD(lambda) (Sutton, 1988) | `td_lambda_update()` in rl_agent.c | `td_lambda_weight_update` in adaptation.lean |
| 5 | Policy Iteration Convergence (Howard, 1960) | `policy_iteration()` in rl_agent.c | — |
| 6 | Nash Existence Theorem (Nash, 1951) | `nfg_lemke_howson()`, `nfg_is_nash()` | `nash_existence_theorem` in adaptation.lean |
| 7 | ESS Condition (Maynard Smith & Price, 1973) | `replicator_is_ess()` in game_theory.c | `is_ess` in adaptation.lean |
| 8 | Replicator Dynamics (Taylor & Jonker, 1978) | `replicator_step()` in game_theory.c | `replicator_simplex_invariant` in adaptation.lean |
| 9 | UCB1 Regret Bound (Auer et al., 2002) | `bandit_ucb1_regret_bound()` in bandit.c | `ucb1_regret_bound` in adaptation.lean |
| 10 | Lai-Robbins Lower Bound (1985) | `bandit_lai_robbins_bound()` in bandit.c | — |
| 11 | EXP3 Regret Bound (Auer et al., 2002) | `exp3_regret_bound()` in bandit.c | — |
| 12 | epsilon-Greedy Regret | `bandit_cumulative_regret_sim()` in bandit.c | `epsilon_greedy_regret_bound` in adaptation.lean |

## L5: Algorithms/Methods (Complete)

| # | Algorithm | Implementation |
|---|-----------|---------------|
| 1 | Q-Learning | `qlearning_update()` in rl_agent.c |
| 2 | SARSA | `sarsa_update()` in rl_agent.c |
| 3 | TD(lambda) with eligibility traces | `td_lambda_update()`, `EligibilityTrace` in rl_agent.c |
| 4 | Dyna-Q (integrated planning+learning) | `dyna_q_step()`, `DynaModel` in rl_agent.c |
| 5 | Policy Iteration | `policy_iteration()`, `policy_evaluation()`, `policy_improvement()` in rl_agent.c |
| 6 | Value Iteration | `value_iteration()` in rl_agent.c |
| 7 | epsilon-Greedy Bandit | `bandit_epsilon_greedy()` in bandit.c |
| 8 | UCB1 | `bandit_ucb1()`, `bandit_ucb()` in bandit.c |
| 9 | Thompson Sampling (Beta) | `bandit_thompson_sampling_beta()` in bandit.c |
| 10 | Thompson Sampling (Gaussian) | `bandit_thompson_sampling_gaussian()` in bandit.c |
| 11 | Softmax Bandit | `bandit_softmax()` in bandit.c |
| 12 | epsilon-Decreasing | `bandit_epsilon_decreasing()` in bandit.c |
| 13 | Optimistic Initialization | `bandit_optimistic_init()` in bandit.c |
| 14 | EXP3 (Adversarial Bandit) | `exp3_select_arm()`, `exp3_update()` in bandit.c |
| 15 | LinUCB (Contextual Bandit) | `cbandit_linucb_select()`, `cbandit_linucb_update()` in bandit.c |
| 16 | PSO | `pso_update()`, `pso_run()` in swarm.c |
| 17 | PSO with Linear Inertia | `pso_linear_inertia()` in swarm.c |
| 18 | PSO with Constriction | `pso_set_constriction()`, `pso_constriction_coefficient()` in swarm.c |
| 19 | ACO for TSP | `aco_construct_tour()`, `aco_run()` in swarm.c |
| 20 | Elitist ACO | `aco_elitist_deposit()` in swarm.c |
| 21 | Reynolds Flocking (Boids) | `flock_step()` in swarm.c |
| 22 | Firefly Algorithm | `firefly_move()`, `firefly_run()` in swarm.c |
| 23 | Lemke-Howson (NE computation) | `nfg_lemke_howson()` in game_theory.c |
| 24 | Support Enumeration (all NE) | `nfg_enumeration_find_all()` in game_theory.c |
| 25 | NK Hill Climbing | `nk_hill_climb()` in environment.c |
| 26 | GridWorld Q-training | `gridworld_train_qlearning()`, `gridworld_train_sarsa()` in rl_agent.c |
| 27 | GridWorld Path Extraction | `gridworld_extract_path()` in rl_agent.c |
| 28 | Replicator Dynamics Simulation | `replicator_run()` in game_theory.c |
| 29 | Round-Robin PD Tournament | `pd_round_robin()` in game_theory.c |

## L6: Canonical Problems (Complete)

| # | Problem | Implementation |
|---|---------|---------------|
| 1 | Multi-Armed Bandit | `bandit.h/c` — full implementation + comparison |
| 2 | GridWorld Navigation | `gridworld_example.c`, `gridworld_train_qlearning()` in rl_agent.c |
| 3 | Prisoner's Dilemma | `nfg_prisoners_dilemma()`, `pd_tournament()` in game_theory.c |
| 4 | TSP via ACO | `aco_create()`, `aco_run()` in swarm.c |
| 5 | Function Optimization (Rastrigin, Rosenbrock, Griewank) | `swarm_test_*()` in swarm.c |
| 6 | Rock-Paper-Scissors | `nfg_rock_paper_scissors()` in game_theory.c |
| 7 | Battle of the Sexes | `nfg_battle_of_sexes()` in game_theory.c |
| 8 | Stag Hunt | `nfg_stag_hunt()` in game_theory.c |
| 9 | Hawk-Dove | `nfg_hawk_dove()` in game_theory.c |
| 10 | Matching Pennies | `nfg_matching_pennies()` in game_theory.c |

## L7: Applications (Partial+ — 3 applications)

| # | Application | Implementation |
|---|-------------|---------------|
| 1 | Adaptive Traffic / Navigation | GridWorld path finding via RL — `gridworld_example.c` |
| 2 | Algorithm Portfolio Selection | Bandit algorithm comparison — `bandit_example.c` |
| 3 | Swarm-based Optimization | PSO/ACO/Firefly for engineering optimization — `swarm_example.c` |
| 4 | Evolution of Cooperation | Axelrod-style PD tournament — `prisoner_dilemma.c` |
| 5 | News Recommendation (LinUCB) | `cbandit_linucb_select()` in bandit.c |

## L8: Advanced Topics (Partial+ — 2 advanced topics)

| # | Topic | Implementation |
|---|-------|---------------|
| 1 | Adversarial Bandit (EXP3) | `EXP3State`, `exp3_select_arm()`, `exp3_update()` in bandit.c |
| 2 | Correlated Equilibrium (Aumann, 1974) | `CorrelatedEquilibrium`, `ce_is_valid()` in game_theory.c |
| 3 | NK Fitness Landscape (Kauffman, 1993) | Full implementation with ruggedness, local optima in environment.c |
| 4 | Co-evolutionary Dynamics | Replicator dynamics with Jacobian — `replicator_jacobian()` in game_theory.c |

## L9: Research Frontiers (Partial — documented)

| # | Topic | Status |
|---|-------|--------|
| 1 | Meta-Learning Agents | Documented in course-tree.md |
| 2 | Multi-Agent Emergent Communication | Documented in gap-report.md |
| 3 | Deep RL Integration | Roadmap item in gap-report.md |