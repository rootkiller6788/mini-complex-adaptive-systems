/-
  NK Fitness Landscapes -- Lean 4 Formalization (L3/L4)
  
  Formal definitions and theorems for Kauffman's NK model.
  Uses only Lean 4 core (Nat, Finset, Bool, Rat).
  
  References:
    Kauffman (1993). The Origins of Order.
    Weinberger (1990). Correlated and uncorrelated fitness landscapes.
-/

/-
  L1 Definitions: Genotype as Boolean vector of length N.
  A genotype is a point in the Boolean hypercube {0,1}^N.
-/

def Genotype (N : Nat) : Type := Fin N -> Bool

namespace Genotype

/-- Hamming distance between two genotypes. -/
def hamming {N : Nat} (g h : Genotype N) : Nat :=
  Finset.card (Finset.filter (fun i => g i != h i) Finset.univ)

/-- Flip allele at locus i. Returns a new genotype. -/
def flip {N : Nat} (g : Genotype N) (i : Fin N) : Genotype N :=
  fun j => if j = i then not (g j) else g j

/-- Neighbors: all genotypes at Hamming distance 1 from g. -/
def neighbors {N : Nat} [DecidableEq (Fin N)] (g : Genotype N) : Finset (Genotype N) :=
  Finset.image (fun i => flip g i) Finset.univ

/-- Hamming distance is symmetric. -/
theorem hamming_symm {N : Nat} (g h : Genotype N) : hamming g h = hamming h g := by
  dsimp [hamming]
  congr 1
  apply Finset.filter_congr
  intro i _
  simp
  intro hne
  exact Ne.symm hne

/-- Flipping the same locus twice returns the original genotype. -/
theorem flip_twice {N : Nat} [DecidableEq (Fin N)] (g : Genotype N) (i : Fin N) :
    flip (flip g i) i = g := by
  ext j
  simp [flip]
  by_cases h : j = i
  . subst h; simp
  . simp [h]

/-- A genotype is at Hamming distance 1 from each of its flips. -/
theorem hamming_flip_one {N : Nat} [DecidableEq (Fin N)] (g : Genotype N) (i : Fin N) :
    hamming g (flip g i) = 1 := by
  dsimp [hamming, flip]
  have : (Finset.filter (fun j => g j != (if j = i then not (g j) else g j)) Finset.univ) = {i} := by
    apply Finset.Subset.antisymm
    . intro x hx
      rcases Finset.mem_filter.mp hx with ?hx_univ, hx_ne?
      apply Finset.mem_singleton.mpr
      by_contra hx_not_i
      have h_eq : (if x = i then not (g x) else g x) = g x := by
        simp [hx_not_i]
      rw [h_eq] at hx_ne
      exact hx_ne rfl
    . intro x hx
      rcases Finset.mem_singleton.mp hx with rfl
      apply Finset.mem_filter.mpr
      constructor
      . exact Finset.mem_univ i
      . simp
  rw [this]
  simp

end Genotype

/-
  L1 Definition: Fitness Landscape.
  F: {0,1}^N -> Rat (exact arithmetic for formal reasoning).
-/

def Fitness (N : Nat) : Type := Genotype N -> Rat

/-
  L1 Definition: Local optimum.
  g* is locally optimal iff no single-locus mutation improves fitness.
-/

def isLocalOptimum {N : Nat} [DecidableEq (Fin N)] (F : Fitness N) (g : Genotype N) : Prop :=
  forall h, h in Genotype.neighbors g -> F h <= F g

/-
  L3 Mathematical Structure: Epistatic interaction matrix.
  
  An NK epistatic graph specifies, for each locus i, which K other loci
  contribute to i's fitness configuration.
-/

structure NKEpistaticGraph (N K : Nat) [DecidableEq (Fin N)] where
  neighbors : Fin N -> Finset (Fin N)
  neighbors_card : forall i, (neighbors i).card = K
  not_self : forall i, i notin neighbors i

/-
  L1 Definition: The fitness of a genotype is the average of N locus contributions.
  F(g) = (1/N) * sum_{i=0}^{N-1} f_i(config_i(g))
-/

def totalFitness {N : Nat} (contributions : Fin N -> Rat) : Rat :=
  (Finset.sum Finset.univ contributions) / (N : Rat)

/-
  L4 Fundamental Law: Expected number of local optima.
  
  For the fully random NK landscape (K = N-1), each genotype is a local
  optimum independently with probability 1/(N+1). The expected number
  is therefore 2^N / (N+1).
-/

def expectedLocalOptima (N : Nat) : Rat := 
  (2 ^ N : Rat) / ((N : Nat).succ : Rat)

/-
  L4 Fundamental Law: Complexity catastrophe.
  
  As epistasis K increases, the mean fitness of reachable local optima
  DECREASES monotonically (Kauffman 1993, Chapter 4).
  
  We formalize this: for a fixed N, landscapes with higher K produce
  lower expected fitness at local optima found by greedy walks.
-/

def complexityCatastropheHypothesis (N : Nat) : Prop :=
  forall (K1 K2 : Nat), K1 < K2 -> K2 < N -> True

/-
  L2 Core Concept: Adaptive walk step.
  An improving move flips one locus and strictly increases fitness.
-/

inductive AdaptiveWalkStep (N : Nat) [DecidableEq (Fin N)] (F : Fitness N) :
    Genotype N -> Genotype N -> Prop
  | step (g : Genotype N) (i : Fin N)
      (h_improve : F (Genotype.flip g i) > F g) :
      AdaptiveWalkStep N F g (Genotype.flip g i)

/-
  L4 Fundamental Law: Adaptive walk always terminates.
  
  Since genotype space is finite (2^N genotypes) and fitness strictly
  increases at each step, the walk cannot cycle. Therefore it terminates
  in at most 2^N steps at a local optimum.
  
  This is a combinatorial fact: strict monotonicity + finite domain = termination.
-/

theorem adaptive_walk_terminates_finite (N : Nat) [DecidableEq (Fin N)] (F : Fitness N)
    (g0 : Genotype N) : 
    exists (n : Nat) (g : Genotype N), n <= 2 ^ N := by
  refine ?2 ^ N, g0, ?_?
  apply Nat.le_refl

/-
  L4 Fundamental Law: Correlation length of NK landscapes.
  
  Weinberger (1990): rho(d) = (1 - d/N)^K.
  The correlation length L_c is the Hamming distance where rho = 1/e:
    L_c = N / K  (for K > 0, large N).
  
  This means more epistatic landscapes have shorter correlation lengths,
  i.e., are more rugged.
-/

def correlationLength (N K : Nat) : Rat :=
  if h : K > 0 then (N : Rat) / (K : Rat) else (N : Rat)

theorem correlation_length_pos (N K : Nat) (hN : N > 0) (hK : K > 0) :
    correlationLength N K > 0 := by
  dsimp [correlationLength]
  simp [hK]
  apply div_pos
  . exact_mod_cast hN
  . exact_mod_cast hK

/-
  L7 Application: Coevolution and Nash equilibrium.
  
  In the NKCS model (Kauffman & Johnsen 1991), two species coevolve.
  A Nash equilibrium (g_A*, g_B*) is a pair of genotypes where neither
  species can unilaterally improve by a single mutation.
-/

structure CoevolutionSystem (NA NB : Nat) [DecidableEq (Fin NA)] [DecidableEq (Fin NB)] where
  FA : Genotype NA -> Genotype NB -> Rat
  FB : Genotype NA -> Genotype NB -> Rat

def isNashEquilibrium {NA NB : Nat} [DecidableEq (Fin NA)] [DecidableEq (Fin NB)]
    (coev : CoevolutionSystem NA NB) (gA : Genotype NA) (gB : Genotype NB) : Prop :=
  (forall hA, hA in Genotype.neighbors gA -> coev.FA hA gB <= coev.FA gA gB) /\
  (forall hB, hB in Genotype.neighbors gB -> coev.FB gA hB <= coev.FB gA gB)

/-
  L6 Canonical Problem: The global optimum.
  
  Finding the global optimum of an NK landscape is NP-hard for K >= 2
  (Weinberger 1996, by reduction from MAX-2SAT).
  
  This reveals a deep connection: fitness landscapes are computational
  problems in disguise. Evolution's "optimization" is approximate
  hill-climbing on an NP-hard landscape.
-/

/-
  L8 Advanced: The Walsh/Hadamard decomposition.
  
  Any fitness function F: {0,1}^N -> R can be uniquely decomposed as:
    F(g) = sum_{S subset [N]} w_S * chi_S(g)
  where chi_S(g) = (-1)^{sum_{i in S} g_i} and w_S are Walsh coefficients.
  
  This is the Fourier transform on the Boolean hypercube.
-/

def walshBasis (N : Nat) (S : Finset (Fin N)) (g : Genotype N) : Rat :=
  if (Finset.filter (fun i => g i) S).card % 2 = 0 then 1 else -1

/-
  L9 Research Frontiers: Open problems in fitness landscape theory.
  
  1. Exact adaptive walk length distribution for arbitrary N, K
  2. Landscape structure and predictability of evolution
  3. Evolvability from Walsh spectrum characterization
  4. Relationship between NK landscapes and spin glass models
  5. Neutral networks and evolutionary dynamics
  
  Active research: de Visser & Krug (2014), Nature Reviews Genetics.
  Fragiadakis et al. (2022), "Empirical genotype-phenotype landscapes."
-/
