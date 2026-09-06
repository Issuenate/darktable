# Agent tool architecture

## Scope

The Essentials interface, deterministic Find Anything search, future language
model providers, and external agents share one capability boundary. UI labels,
processing-module parameter blobs, XMP internals, SQL statements, and catalog
memory are not public agent interfaces.

The first implemented slice is the Library / Choose workflow. It provides the
shared registry, offline discovery, safe navigation, a focused selection
inspector, read-only MCP discovery, and versioned wire contracts. The complete
plan executor is intentionally not implemented in this slice.

## Capability registry

Descriptors live in `src/common/capabilities.c` and use stable, non-localized
IDs. A descriptor owns its version, friendly copy, search phrases, help topic,
contexts, selection policy, typed arguments, side-effect class, lifecycle
requirements, navigation target, and agent exposure.

Essentials UI actions must use `DT_ESSENTIALS_ACTION("stable.id")`. The
`tools/check_capability_registry.py` test rejects references that do not have a
descriptor. UI text may be translated without changing an ID or a stored plan.

The current agent exposure levels are:

- `internal_only`: never advertised
- `discovery`: discoverable but not callable
- `read_only`: safe inspection or navigation
- `previewable`: an agent may request a typed plan and preview
- `executable`: eligible for commit after the executor enforces policy

No capability is promoted to `executable` merely because a legacy darktable
function can perform the operation.

## Safe action lifecycle

All future prompting and agent entry points use this sequence:

```text
request
-> capability discovery
-> typed plan
-> validation
-> preview
-> confirmation
-> transactional execution
-> verification
-> execution receipt and undo
```

The contracts in `data/capabilities` are the versioned boundary:

- `capability.schema.json`
- `plan.schema.json`
- `preview.schema.json`
- `permission.schema.json`
- `execution-receipt.schema.json`
- `error.schema.json`

A preview must set `writes_performed` to false. It must not write history,
sidecars, metadata, the catalog, or output files. Plans bind to a selection and
history `state_hash`; a commit with a different state fails with `stale_plan`.
Commit requests require an idempotency key. A repeated key returns the original
receipt and never repeats edits or output. Reversible steps execute in one undo
group and roll back when any plan step or verification fails.

## GUI ownership and catalog safety

The GUI owns live execution while it owns the catalog. A standalone MCP
process must not write to a catalog locked by the GUI. A future live bridge
must submit plans to the GUI executor or to an authenticated local broker that
serializes access through that executor.

The existing headless MCP tools remain for compatibility. Raw module
introspection stays in that developer-oriented namespace. The read-only
`capabilities_search` and `capabilities_describe` tools are the first shared
registry consumers and do not grant mutation authority.

## Provider boundary

A future provider adapter may convert natural language into a typed plan only.
It cannot execute capabilities. Local and cloud providers use the same plan
schema and executor. Cloud requests receive no pixels, filenames, EXIF,
locations, or catalog data without a request-specific permission record.
Credentials belong in platform-secure storage; prompt history is session-only
by default.

## Deferred work

- implement plan validation, state hashes, preview isolation, and receipts
- implement the GUI-owned transactional executor and rollback verification
- add safe organization and export plan builders
- make the Essentials editor use semantic adjustment facades
- expose job progress, cancellation, and undo capabilities
- add provider-neutral local and cloud model adapters
- design the authenticated live-agent broker and session permissions
- decide whether a companion app is warranted after the contracts stabilize
- add parity tests proving GUI, prompted, and MCP plans produce identical
  history, XMP, and pixel output

