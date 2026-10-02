# Component maintenance workflow

Each component has exactly one live work queue: `doc/BACKLOG.md`. It includes
roadmap proposals, reproducible defects and missing qualification. Do not
create competing ROADMAP, PLAN or KNOWN-ISSUES trackers. Design documents own
contracts; user guides own supported behaviour; dated qualification records
own exact results.

1. Record a finding under a stable component-prefixed ID. State its type
   (defect, improvement or qualification), status, observed behaviour, evidence,
   affected target and acceptance criteria. State unknowns without inventing
   a cause. Search existing entries before adding another.
2. Use Open, In progress, Blocked or Done. Open does not authorise work. Mark
   In progress only when the session's agreed scope includes implementation;
   name a concrete prerequisite when marking Blocked.
3. Make authorised changes directly in the owning component's `src/` and
   maintained build recipes. A new upstream input is reconciled through normal
   Git commits with attribution in UPSTREAM.md; frozen archives do not change.
4. Build the component and run affected checks. Preserve failure controls and
   separate host, object/link, disk-image, guest and release qualification.
5. Close an item only after its acceptance is met. Record the commands, result
   and relevant source/target identities, and update current user/design docs.
   Keep the item ID for traceability; Git retains previous source checkpoints.

For this reorganisation, functional defect repairs are outside scope. Record
existing defects and continue equivalent migration wherever possible. Repair
regressions introduced by moving paths or replacing patch-based preparation.
Do not delete a failing check or waive an acceptance requirement to make a
migration pass. No commit, push, publication or external action follows from
this workflow without explicit session authority.
