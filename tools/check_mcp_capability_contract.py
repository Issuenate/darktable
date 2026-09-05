#!/usr/bin/env python3
"""Check that the MCP capability tools honour data/capabilities/capability.schema.json.

The schema is published as the wire contract, so a response that does not
satisfy it is a bug in one or the other. This validates without depending on a
jsonschema package: the checks are the ones the schema actually asserts --
declared properties, required properties, and additionalProperties.
"""

from __future__ import annotations

import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "data/capabilities/capability.schema.json"


def _call(binary: pathlib.Path, name: str, arguments: dict) -> dict:
    request = "\n".join([
        json.dumps({"jsonrpc": "2.0", "id": 1, "method": "initialize",
                    "params": {"protocolVersion": "2024-11-05", "capabilities": {},
                               "clientInfo": {"name": "contract-check", "version": "1"}}}),
        json.dumps({"jsonrpc": "2.0", "id": 2, "method": "tools/call",
                    "params": {"name": name, "arguments": arguments}}),
    ]) + "\n"
    done = subprocess.run([str(binary)], input=request, capture_output=True,
                          text=True, timeout=60)
    if done.returncode != 0:
        raise RuntimeError(f"{binary} exited {done.returncode}: {done.stderr[-400:]}")
    reply = json.loads(done.stdout.strip().split("\n")[-1])
    result = reply["result"]
    if result.get("isError"):
        raise RuntimeError(f"{name} returned an error: {result['content'][0]['text']}")
    return json.loads(result["content"][0]["text"])


def _violations(obj: dict, schema: dict, where: str) -> list[str]:
    declared = set(schema.get("properties", {}))
    problems = [f"{where}: missing required property '{key}'"
                for key in schema.get("required", []) if key not in obj]
    if schema.get("additionalProperties") is False:
        problems += [f"{where}: undeclared property '{key}'"
                     for key in sorted(set(obj) - declared)]
    return problems


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: check_mcp_capability_contract.py <path to darktable-mcp>")
        return 2
    binary = pathlib.Path(sys.argv[1])
    if not binary.exists():
        print(f"darktable-mcp not built at {binary}; nothing to check")
        return 0

    schema = json.loads(SCHEMA.read_text(encoding="utf-8"))
    problems: list[str] = []

    described = _call(binary, "capabilities_describe", {"id": "library.rate"})
    problems += _violations(described, schema, "capabilities_describe")

    found = _call(binary, "capabilities_search", {"query": "make brighter", "limit": 5})
    if not found.get("capabilities"):
        problems.append("capabilities_search: returned nothing for a phrase that should match")
    for index, entry in enumerate(found.get("capabilities", [])):
        if "score" not in entry or "capability" not in entry:
            problems.append(f"capabilities_search[{index}]: expected score and capability")
            continue
        problems += _violations(entry["capability"], schema,
                                f"capabilities_search[{index}].capability")

    # the limit must be respected: it is the only bound on a caller-supplied size
    capped = _call(binary, "capabilities_search", {"limit": 2})
    if len(capped.get("capabilities", [])) > 2:
        problems.append("capabilities_search: returned more results than the requested limit")

    if problems:
        for problem in problems:
            print(problem)
        return 1
    print("MCP capability tools satisfy capability.schema.json")
    return 0


if __name__ == "__main__":
    sys.exit(main())
