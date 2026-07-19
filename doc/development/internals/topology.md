---
title: "Topology Internals"
date: 2026-06-26
weight: 110
geekdocHidden: false
---

The topology extension has SQL-facing documentation in the user manual. This
page is for implementation notes that matter when changing the C topology
engine or its PostgreSQL wrappers.

## Edge End Stars

An edge end star is the collection of directed edge ends incident to one
topology node. The current implementation lives in
`liblwgeom/topo/lwt_edgeend_star.h` and
`liblwgeom/topo/lwt_edgeend_star.c`, with individual edge ends represented by
`liblwgeom/topo/lwt_edgeend.h`.

`lwt_edgeEndStar_addEdge()` adds the outgoing end when the edge starts at the
star node, the incoming end when the edge ends there, or both ends for a closed
edge incident to the same node. Each edge end records its edge pointer,
direction, and azimuth.

The star is sorted lazily by azimuth. `lwt_edgeEndStar_getNextCW()` and
`lwt_edgeEndStar_getNextCCW()` ensure sorting, find the requested directed edge
end, and return the neighboring edge end around the node, wrapping at either
end of the sorted array.

This structure is an internal navigation helper for topology algorithms that
need adjacent edge-end order around a node. User-facing topology behavior
belongs in the manual; the old Trac `EdgeStar` page was only a pointer to this
header and should not remain as a parallel source page.
