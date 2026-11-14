// dependency_fabric::test — regression: superseding a historical generation
// never rolls the authoritative generation back, and re-activating an edge
// never closes a cycle.
// Copyright 2026 Summon Software Labs.
// SPDX-License-Identifier: Apache-2.0

#include "test_util.hpp"

using namespace df_test;
using namespace dependency_fabric;

int main() {
  // --- Generation rollback through a historical supersession ---------------
  {
    tf::Fixture f;
    CHECK_EQ(f.declare(1, NodeKind::MODEL, "a"), Outcome::ACCEPTED);
    CHECK_EQ(f.pub(1, 1), Outcome::ACCEPTED);
    CHECK_EQ(f.pub(1, 5), Outcome::ACCEPTED);
    // Generation 1 is historical; superseding it with 3 would move the
    // authoritative generation from 5 back to 3.
    CHECK_EQ(f.g.supersede_generation(DependencyNodeId(1), DependencyNodeGeneration(1),
                                      DependencyNodeGeneration(3), f.src, f.boot, f.epoch),
             Outcome::REJECT_STALE_NODE_GENERATION);
    CHECK_EQ(f.g.current_authority(DependencyNodeId(1)).value(), 5u);
    // Superseding with a newer generation than the current one moves forward.
    CHECK_EQ(f.g.supersede_generation(DependencyNodeId(1), DependencyNodeGeneration(1),
                                      DependencyNodeGeneration(7), f.src, f.boot, f.epoch),
             Outcome::ACCEPTED);
    CHECK_EQ(f.g.current_authority(DependencyNodeId(1)).value(), 7u);
  }

  // --- Re-activating an edge must not close a cycle -------------------------
  {
    tf::Fixture f;
    CHECK_EQ(f.declare(1, NodeKind::MODEL, "a"), Outcome::ACCEPTED);
    CHECK_EQ(f.declare(2, NodeKind::ARTIFACT, "b"), Outcome::ACCEPTED);
    CHECK_EQ(f.add(10, 1, 2, 1), Outcome::ACCEPTED);                 // B depends on A
    CHECK_EQ(f.g.set_edge_active(DependencyEdgeId(10), false, f.src, f.boot, f.epoch), Outcome::ACCEPTED);
    CHECK_EQ(f.add(11, 2, 1, 1), Outcome::ACCEPTED);                 // A depends on B (A->B inactive)
    CHECK_EQ(f.g.set_edge_active(DependencyEdgeId(10), true, f.src, f.boot, f.epoch), Outcome::REJECT_CYCLE);
    CHECK(f.g.acyclic());
    CHECK(!f.g.edge(DependencyEdgeId(10))->active);
  }

  return summary("regression");
}
