# The Unique Games Conjecture: Khot's 2002 Paper

> **Primary paper:** Subhash Khot, “On the Power of Unique 2-Prover 1-Round Games” (2002).  
> **Scope:** A research note on the paper's model, conjecture, algorithmic result, conditional hardness consequences, and its role in PCP-based inapproximability. This document is a reading guide, not a proof of the conjecture.

## 1. Why this paper matters

The paper introduces the **Unique Games Conjecture (UGC)** as a statement about the hardness of distinguishing almost-satisfiable unique two-prover one-round games from games in which every strategy satisfies only a small fraction of constraints.

Its significance is methodological as well as computational. Khot proposes using a unique game as an **outer verifier** and constructing probabilistically checkable proof systems (PCPs) on top of it. The uniqueness condition is important to the analysis of those PCP constructions. The paper then derives conditional hardness results for several constraint and graph problems.

The conjecture is a hypothesis about computational hardness. The consequences below are therefore conditional: they follow if the conjecture holds; they are not unconditional hardness theorems.

## 2. The underlying two-prover game

A two-prover one-round game consists of a verifier, questions sent to two provers, answers returned by the provers, and a predicate determining whether the verifier accepts. The provers may agree on a strategy in advance but cannot communicate after receiving their questions.

The **value** of a game, written \(\operatorname{val}(G)\), is the maximum acceptance probability over the provers' strategies.

A game is **unique** when the answer of either prover uniquely determines the answer expected from the other prover. In the common constraint-graph representation:

- vertices represent variables and edges represent constraints;
- each variable receives a label from a finite alphabet \(\Sigma\), with \(|\Sigma|=k\);
- an oriented edge \((u,v)\) carries a permutation (bijection) \(\pi_{uv}:\Sigma\to\Sigma\);
- the edge constraint is satisfied exactly when

\[
x_v=\pi_{uv}(x_u).
\]

Because \(\pi_{uv}\) is a bijection, each label at one endpoint has exactly one compatible label at the other endpoint. Reversing the edge uses the inverse permutation.

The optimization problem is to choose labels for all vertices to maximize the fraction (or weighted probability) of satisfied edge constraints.

## 3. The Unique Games Conjecture

In the paper's formulation, for arbitrarily small constants \(\zeta,\delta>0\), there is a constant alphabet size \(k=k(\zeta,\delta)\) such that it is NP-hard to distinguish these two cases for a unique two-prover game with answer domain size \(k\):

**YES case — almost satisfiable**

\[
\operatorname{val}(G)\geq 1-\zeta.
\]

**NO case — highly unsatisfiable**

\[
\operatorname{val}(G)\leq\delta.
\]

The quantifier order matters:

\[
\forall\zeta,\delta>0\;\exists k=k(\zeta,\delta)
\]

with \(k\) a constant for each fixed pair of error parameters. The conjecture does **not** say that one fixed alphabet size works for every choice of \(\zeta\) and \(\delta\), nor does it assert that every instance is easy to classify.

The gap between \(1-\zeta\) and \(\delta\) is the hardness gap. UGC predicts that, for suitable constant alphabet size, even distinguishing these two extreme regimes is NP-hard.

## 4. The SDP approximation result in the paper

Khot also presents a semidefinite-programming-based algorithm for finding good prover strategies. In the paper's stated guarantee, given a unique two-prover game with value \(1-\zeta\) and answer-domain size \(k\), the algorithm finds strategies whose acceptance probability is at least

\[
1-O\!\left(k^2\zeta^{1/5}\sqrt{\log(1/\zeta)}\right).
\]

This is an approximation guarantee in the **near-satisfiable regime**. It is not a general exact algorithm, and the expression is meaningful as a useful lower bound only in parameter ranges where the error term is sufficiently small.

The result also constrains how the conjecture can hold: for a fixed alphabet size, the algorithm would recover a high-value strategy on sufficiently near-satisfiable instances. Thus, if UGC is true for arbitrarily small soundness \(\delta\), the alphabet size \(k(\zeta,\delta)\) must grow sufficiently as the gap parameters are tightened. The paper explicitly draws this qualitative conclusion.

## 5. Conditional hardness consequences

The paper shows that a positive resolution of UGC would imply hardness results for other problems. These implications are conditional on UGC.

### 5.1 Linear equations modulo 2

For any fixed

\[
\frac{1}{2}<t<1,
\]

and sufficiently small constant \(\epsilon>0\), it is NP-hard to distinguish instances of **2-Linear-Equations mod 2** in which:

- there is an assignment satisfying at least a \(1-\epsilon\) fraction of equations; or
- every assignment satisfies at most a \(1-\epsilon^t\) fraction of equations.

The paper further gives **Min-2CNF-Deletion** as a corollary, stating that it is NP-hard to approximate that problem within any constant factor, assuming UGC.

### 5.2 NAE constraints over \(\mathrm{GF}(3)\)

For the constraint satisfaction problem whose constraints are

\[
\operatorname{NAE}(a,b,c),\qquad a,b,c\in\mathrm{GF}(3),
\]

the paper states that, for arbitrarily small constant \(\epsilon>0\), it is NP-hard to distinguish:

- instances with an assignment satisfying at least \(1-\epsilon\) of the constraints; from
- instances for which no assignment satisfies more than \(\frac{8}{9}+\epsilon\) of the constraints.

The paper identifies this result as relevant to hardness for coloring 3-colorable 3-uniform hypergraphs. This is a connection to a coloring problem, not a claim that every hypergraph-coloring formulation is identical to the NAE-GF(3) CSP.

### 5.3 Independent set consequence

The paper's abstract also states a consequence for a variation of UGC: for arbitrarily small constant \(\delta>0\), it is hard to find an independent set of size \(\delta n\) in a graph guaranteed to contain an independent set of size \(\Omega(n)\). The qualification “a variation of the UGC” is important; this should not be silently restated as a consequence of every formulation of the conjecture.

## 6. The PCP / outer-verifier viewpoint

A central constructional idea is to use the unique two-prover game supplied by the conjecture as an **outer verifier**, then build a PCP on top of it.

At a high level:

1. The outer game supplies a global consistency structure through its edge permutations.
2. A PCP construction adds local tests and proof structure around that game.
3. The uniqueness property is used in analyzing how local answers must correspond across constraints.
4. If the outer game has the conjectured YES/NO gap, the composed construction can transfer that gap into hardness for a target optimization or constraint problem.

This is the conceptual bridge from a statement about two-prover games to inapproximability results for familiar combinatorial problems. It is not a complete PCP composition proof; the technical details belong to the original paper and its cited constructions.

## 7. How to read the conjecture alongside later work

UGC became a widely used conditional source of hardness-of-approximation results. Later work studied both consequences of UGC and algorithms for unique games. For example, Khot and Regev used the conjecture to derive strong conditional hardness for Vertex Cover, including a hypergraph formulation.

When comparing later results, keep the exact game model and quantifiers visible. In particular:

- **Unique (1-to-1) games:** each label on one side has exactly one compatible label on the other side, via a bijection.
- **2-to-1 games:** the constraint structure has a different preimage/fiber condition; it is not simply another name for a unique game.
- **2-to-2 and 4-to-1 variants:** these are distinct constraint models with their own hardness statements and assumptions.

A theorem proved for a 2-to-1, 2-to-2, or 4-to-1 game does not, without an explicit reduction or equivalence theorem, prove the original 1-to-1 UGC. Likewise, an algorithm for a restricted variant should not be presented as resolving the original conjecture.

This note does not claim that UGC has been proved or disproved. Any claimed resolution must be evaluated from its actual theorem statement, reduction assumptions, publication record, and independent verification—not from a headline or a related variant.

## 8. What the 2002 paper establishes—and what it does not

| Statement | Status in this paper |
|---|---|
| Definition of unique two-prover one-round games and their value | Defined |
| The near-perfect versus low-value NP-hardness assertion | Conjectured (UGC) |
| SDP-based strategy-finding guarantee | Algorithmic result stated in the paper |
| Hardness for 2-Linear-Equations mod 2 and Min-2CNF-Deletion | Conditional on UGC |
| NAE-GF(3) gap and hypergraph-coloring relevance | Conditional on UGC |
| Independent-set consequence | Stated for a variation of UGC |
| Proof or disproof of UGC | Not supplied; the paper describes it as open |

## 9. Suggested reading order

1. Read the abstract and introduction to understand the conjecture's motivation and the outer-verifier idea.
2. Formalize the game value and the bijective edge constraints.
3. Write the YES/NO promise problem with its quantifiers before reading any derived hardness result.
4. Study the SDP guarantee and identify its dependence on alphabet size and completeness error.
5. Trace each conditional reduction separately; do not collapse the 2-Lin, Min-2CNF-Deletion, NAE-GF(3), and hypergraph-coloring statements into one theorem.
6. Compare later UGC-based results only after checking whether they use the original unique-game model or a variant.

## 10. Bibliographic and source notes

There are two 2002 proceedings records associated with this title in the sources consulted. The accessible full-text copy linked below identifies itself as appearing in the **Proceedings of the 17th IEEE Annual Conference on Computational Complexity (CCC 2002)**. Separately, DBLP and the ACM DOI record a Subhash Khot paper with the same title in **Proceedings of the 34th ACM Symposium on Theory of Computing (STOC 2002), pages 767–775**. These records should be checked against the particular edition being cited; this note does not assume the two proceedings records are bibliographically interchangeable.

1. Subhash Khot, “On the Power of Unique 2-Prover 1-Round Games,” *Proceedings of the 17th IEEE Annual Conference on Computational Complexity (CCC 2002)*. [Accessible full-text copy (PDF)](https://static.aminer.org/pdf/PDF/000/104/637/on_the_power_of_unique_prover_round_games.pdf). The PDF itself carries the CCC'02 proceedings footer. [IEEE DOI: 10.1109/CCC.2002.1004334](https://doi.org/10.1109/CCC.2002.1004334).
2. Subhash Khot, “On the Power of Unique 2-Prover 1-Round Games,” *Proceedings of the 34th Annual ACM Symposium on Theory of Computing (STOC 2002)*, pp. 767–775. [ACM DOI: 10.1145/509907.510017](https://doi.org/10.1145/509907.510017). [DBLP bibliographic record](https://dblp.org/rec/conf/stoc/Khot02a.html).
3. Subhash Khot and Oded Regev, “Vertex Cover Might Be Hard to Approximate to Within 2 − ε,” *Journal of Computer and System Sciences* 74(3), 335–349 (2008). [DOI: 10.1016/j.jcss.2007.06.019](https://doi.org/10.1016/j.jcss.2007.06.019). This is a later example of a result based on the conjecture, not a proof of UGC.

### Verification boundary

The formal conjecture, SDP expression, conditional hardness statements, and outer-verifier description above are transcribed or closely paraphrased from the accessible 2002 paper copy. The proceedings metadata is cross-checked against the IEEE DOI record and the DBLP/ACM STOC record; the existence of two records is called out rather than silently resolved. The later Vertex Cover reference is a bibliographic example of a UGC-based consequence. This document is a focused research note, not an exhaustive literature survey or a claim that every later UGC variant has been covered.
