#!/usr/bin/env python3
"""Independent black-box checks. Runtime/security implementation is entirely C.
Uses only Python's standard library; changes are confined to a temporary directory.
"""
import copy
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
EXE = pathlib.Path(sys.argv[1]).resolve()
checks = []

def run(*args, code=0):
    p = subprocess.run([str(EXE), *map(str, args)], capture_output=True, text=True)
    assert p.returncode == code, (args, p.returncode, p.stdout, p.stderr)
    return p

def load(p):
    return json.loads(pathlib.Path(p).read_text())

def check(name, condition):
    assert condition, name
    checks.append({"test": name, "passed": True})

with tempfile.TemporaryDirectory(prefix="nerp-cd-test-") as temp:
    temp = pathlib.Path(temp)
    data = temp / "data"
    shutil.copytree(ROOT.parent / "datastore", data, copy_function=shutil.copyfile)
    cfg = temp / "config.json"
    shutil.copyfile(ROOT / "config.json", cfg)
    original = {p.name: p.read_bytes() for p in data.glob("*.json")}
    out = temp / "run"
    run("demo", data, cfg, out)
    cases = load(out / "policy_scenarios.json")
    check("16 policy experiments match expected outcomes", len(cases) == 16 and all(c["passed"] for c in cases))
    check("D4.1 isolates confidentiality denial", cases[0]["ACM"] and cases[0]["scope"] and not cases[0]["BLP"] and cases[0]["Biba"])
    check("D4.2 isolates integrity denial", cases[1]["ACM"] and cases[1]["BLP"] and not cases[1]["Biba"])
    check("D4.3 permits assigned incident", cases[2]["allowed"])
    check("BLP no write down and Biba no write up exercised", not cases[3]["BLP"] and cases[4]["BLP"] and not cases[4]["Biba"])
    cls = load(out / "classifications.json")
    id_fields = {
        "incidents": "incidentId",
        "responders": "responderId",
        "intelligence": "reportId",
        "dispatch": "dispatchId",
        "audit": "eventId",
    }
    expected = sorted(
        (dataset, row[id_field])
        for dataset, id_field in id_fields.items()
        for row in load(data / f"{dataset}.json")
    )
    actual = sorted((row["dataset"], row["id"]) for row in cls)
    check("each input record classified exactly once", actual == expected)
    c = {(r["dataset"], r["id"]): r for r in cls}
    check("secret incident sensitivity propagates into dispatch", c["incidents", "INC-010"]["classification"] == c["dispatch", "D010"]["classification"] == "SECRET")
    check("TOP SECRET intelligence sensitivity propagates into audit", c["audit", "AUD-010"]["classification"] == "TOP SECRET")
    check("record classification differs from personnel clearance", c["responders", "R004"]["classification"] == "OFFICIAL")
    an = load(out / "analytics_view.json")
    check("analytics excludes secret incidents and all raw intelligence", len(an["incidents"]) == len(an["dispatch"]) == 18 and an["intelligence"] == [])
    check("allowlists remove names, contacts, locations and free text", all(set(r) == {"incidentToken", "assignedAgency", "priority", "casualtyBand", "month"} for r in an["incidents"]))
    check("pseudonymous joins remain useful", {r["incidentToken"] for r in an["incidents"]} == {r["incidentToken"] for r in an["dispatch"]})
    research = load(out / "research_view.json")
    check("research has aggregate cells >= k and remains restricted", len(research["groups"]) == 3 and all(r["incidentCount"] >= 3 for r in research["groups"]) and research["classification"] == "OFFICIAL")
    op = load(out / "operational_view.json")
    check("operational view is scoped to paramedic R002", [r["incidentId"] for r in op["incidents"]] == ["INC-002"] and [r["dispatchId"] for r in op["dispatch"]] == ["D002"] and [r["responderId"] for r in op["responders"]] == ["R002"])
    check("three authenticated encryption checks pass", all(t["passed"] for t in load(out / "crypto_tests.json")))
    rt = load(out / "retention_tests.json")
    check("four managed-copy lifecycle checks pass", all(rt[k] for k in rt if k != "scope"))
    check("legal hold has priority over expired retention", any(r["stem"] == "incidents--INC-006" and r["nextState"] == "LEGAL_HOLD" for r in load(out / "vault_demo" / "retention_events.json")))
    check("fixture input files are unchanged", all(p.read_bytes() == original[p.name] for p in data.glob("*.json")))
    deny = json.loads(run("access", data, cfg, "R002", "intelligence", "INT-001", "read", code=3).stdout)
    check("denied reads return no record data", "data" not in deny)
    allow = json.loads(run("access", data, cfg, "R002", "incidents", "INC-002", "read").stdout)
    check("permitted reads are mediated by the policy engine", allow["allowed"] and allow["data"]["incidentId"] == "INC-002")
    allow = json.loads(run("access", data, cfg, "R001", "incidents", "INC-001", "write", "OFFICIAL").stdout)
    check("fresh lower-label session handles legitimate blind write", allow["allowed"] and "data" not in allow)
    run("retention", cfg, out / "vault_demo", "2028-02-30", code=2)
    check("invalid retention date rejected", True)
    run("retention", cfg, out / "vault_demo", "2026-11-03", code=2)
    check("retention time cannot roll backwards", True)
    run("demo", data, cfg, out, code=2)
    check("existing output is not overwritten", True)
    invpath = out / "vault_demo" / "inventory.json"
    inventory = load(invpath)
    inventory["records"][0]["classification"] = "PUBLIC"
    invpath.write_text(json.dumps(inventory))
    run("retention", cfg, out / "vault_demo", "2029-01-01", code=2)
    check("modified lifecycle metadata fails HMAC verification", True)
    # A malicious extra field must not survive the allowlist transformer.
    modified = load(data / "incidents.json")
    modified[1]["newSensitiveField"] = "DO_NOT_EXPORT"
    (data / "incidents.json").write_text(json.dumps(modified))
    newout = temp / "extra_field_run"
    run("demo", data, cfg, newout)
    check("unexpected fields cannot leak into generated views", "DO_NOT_EXPORT" not in (newout / "analytics_view.json").read_text() and "DO_NOT_EXPORT" not in (newout / "research_view.json").read_text())
    suppressed_cfg = temp / "suppressed.json"
    config_data = load(cfg)
    config_data["research_k"] = 8
    suppressed_cfg.write_text(json.dumps(config_data))
    suppressed_out = temp / "suppressed_run"
    run("demo", data, suppressed_cfg, suppressed_out)
    check("small research cells are suppressed", load(suppressed_out / "research_view.json")["groups"] == [])
    undated_cfg = temp / "undated.json"
    config_data = load(cfg)
    config_data["fixture_ingestion_date"] = ""
    undated_cfg.write_text(json.dumps(config_data))
    undated_vault = temp / "undated_vault"
    run("vault-init", data, undated_cfg, undated_vault)
    run("retention", undated_cfg, undated_vault, "2028-11-05")
    check("undated records cannot silently expire", sum(r["nextState"] == "UNKNOWN_DATE_HOLD" for r in load(undated_vault / "retention_events.json")) == 40)
    cipherpath = newout / "vault_demo" / "ciphertext" / "incidents--INC-006.json"
    envelope = load(cipherpath)
    envelope["ciphertext"] = ("0" if envelope["ciphertext"][0] != "0" else "1") + envelope["ciphertext"][1:]
    cipherpath.write_text(json.dumps(envelope))
    run("vault-read", newout / "vault_demo", "incidents--INC-006", code=3)
    check("tampered managed ciphertext returns no plaintext", True)
    # Classification escalation and ambiguity are tested with changed inputs.
    intel = load(data / "intelligence.json")
    intel[0]["classificationHint"] = "UNRECOGNISED"
    (data / "intelligence.json").write_text(json.dumps(intel))
    d = json.loads(run("access", data, cfg, "R020", "intelligence", "INT-001", "read", code=3).stdout)
    check("unknown classification is quarantined even for cleared users", d["quarantine"] and not d["allowed"])
    intel[0]["classificationHint"] = "OFFICIAL"
    (data / "intelligence.json").write_text(json.dumps(intel))
    d = json.loads(run("access", data, cfg, "R020", "intelligence", "INT-001", "read", code=3).stdout)
    check("content-hint conflict escalates and requires review", d["quarantine"])
    (data / "incidents.json").write_text('[{"incidentId":"X","incidentId":"Y"}]')
    run("access", data, cfg, "R002", "incidents", "INC-002", "read", code=2)
    check("duplicate JSON keys are rejected", True)
    for bad in ['[', '{"x": "\\uD800"}', '[01]', '[1,]', '{}garbage', '[1e999]', '{"x":"\\u0000"}']:
        (data / "incidents.json").write_text(bad)
        run("access", data, cfg, "R002", "incidents", "INC-002", "read", code=2)
    check("malformed JSON, NUL, nonfinite numbers and unpaired surrogates rejected", True)

print(json.dumps({"status": "PASS", "checks": len(checks), "results": checks}, indent=2))
