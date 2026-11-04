#!/usr/bin/env python3
"""Optional document builder; not used by the C runtime. Requires reportlab."""
import collections
import html
import json
import pathlib
import re

from reportlab.lib import colors
from reportlab.lib.enums import TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (SimpleDocTemplate, Paragraph, Spacer, Table,
                               TableStyle, PageBreak, KeepTogether, Preformatted)

ROOT = pathlib.Path(__file__).resolve().parents[1]
DOCS, EVID = ROOT / "docs", ROOT / "evidence"
pdfmetrics.registerFont(TTFont("DV", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("DV-Bold", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"))
pdfmetrics.registerFontFamily("DV", normal="DV", bold="DV-Bold", italic="DV", boldItalic="DV-Bold")
NAVY, BLUE, LIGHT = colors.HexColor("#152c43"), colors.HexColor("#176d88"), colors.HexColor("#edf4f7")
styles = getSampleStyleSheet()
styles.add(ParagraphStyle("BodyDV", fontName="DV", fontSize=9.35, leading=13.4, spaceAfter=8, textColor=NAVY))
styles.add(ParagraphStyle("SmallDV", parent=styles["BodyDV"], fontSize=8.0, leading=11.8, spaceAfter=7))
styles.add(ParagraphStyle("TitleDV", fontName="DV-Bold", fontSize=23, leading=29, textColor=NAVY, spaceAfter=14))
styles.add(ParagraphStyle("HeadingDV", fontName="DV-Bold", fontSize=13.3, leading=18, textColor=BLUE, spaceBefore=14, spaceAfter=8, keepWithNext=True))
styles.add(ParagraphStyle("SubDV", fontName="DV-Bold", fontSize=10, leading=15, textColor=NAVY, spaceBefore=8, spaceAfter=7, keepWithNext=True))
styles.add(ParagraphStyle("CellDV", fontName="DV", fontSize=7.7, leading=10.8, textColor=NAVY))
styles.add(ParagraphStyle("CellHeadDV", parent=styles["CellDV"], fontName="DV-Bold", textColor=colors.white))

def read(name):
    return json.loads((EVID / name).read_text())

def text(s):
    # Add break opportunities to long URLs without changing the visible address.
    s = html.escape(str(s))
    s = re.sub(r"`([^`]+)`", r'<font name="DV-Bold">\1</font>', s)
    s = s.replace("https://", "https://<wbr/>").replace("/", "/<wbr/>")
    return s

def p(s, small=False):
    return Paragraph(text(s), styles["SmallDV" if small else "BodyDV"])

def table(headers, rows, widths):
    data = [[Paragraph(text(c), styles["CellHeadDV"]) for c in headers]]
    data += [[Paragraph(text(c), styles["CellDV"]) for c in row] for row in rows]
    t = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT")
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), NAVY),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, LIGHT]),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 7),
        ("RIGHTPADDING", (0, 0), (-1, -1), 7),
        ("TOPPADDING", (0, 0), (-1, -1), 7),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
        ("LINEBELOW", (0, 0), (-1, 0), 0.7, BLUE),
        ("LINEBELOW", (0, 1), (-1, -1), 0.25, colors.HexColor("#dbe4e9")),
    ]))
    return t

def frame(canvas, doc):
    canvas.saveState()
    width, height = A4
    canvas.setStrokeColor(BLUE)
    canvas.setLineWidth(1.1)
    canvas.line(48, height - 39, width - 48, height - 39)
    canvas.setFillColor(NAVY)
    canvas.setFont("DV-Bold", 8)
    canvas.drawString(48, height - 29, "NERP  /  INFORMATION GOVERNANCE + POLICY ENGINE")
    canvas.setFont("DV", 7.2)
    canvas.drawString(48, 26, "COMSM0122  |  C/D contribution  |  Integration review draft")
    canvas.drawRightString(width - 48, 26, str(doc.page))
    canvas.restoreState()

def build(path, story):
    doc = SimpleDocTemplate(str(path), pagesize=A4, leftMargin=48, rightMargin=48,
                            topMargin=57, bottomMargin=44, title=path.stem.replace("_", " "),
                            author="NERP C/D prototype", pageCompression=1)
    doc.build(story, onFirstPage=frame, onLaterPages=frame)

def report():
    source = (DOCS / "Technical_Report_CD.md").read_text()
    story, lines = [], source.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue
        if line.startswith("# "):
            story.append(Paragraph(text(line[2:]), styles["TitleDV"]))
        elif line.startswith("## "):
            if line == "## References":
                story.append(PageBreak())
            story.append(Paragraph(text(line[3:]), styles["HeadingDV"]))
        elif line.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                row = [c.strip() for c in lines[i].strip().strip("|").split("|")]
                if not all(re.fullmatch(r"[-:]+", c) for c in row):
                    rows.append(row)
                i += 1
            story.append(table(rows[0], rows[1:], [65, 153, 147, 134]))
            story.append(Spacer(1, 9))
            continue
        else:
            para = [line]
            while i + 1 < len(lines) and lines[i + 1].strip() and not lines[i + 1].startswith(("#", "|")):
                i += 1
                para.append(lines[i])
            story.append(p(" ".join(para), small=line.startswith("[")))
        i += 1
    build(DOCS / "Technical_Report_CD.pdf", story)
    counts = {}
    for heading, section in re.findall(r"## ([CD]\d[^\n]*)\n(.*?)(?=\n## |\Z)", source, re.S):
        code = heading.split()[0]
        maximum = 300 if code in {"C3", "D4"} else 250
        count = len(section.split())
        assert count <= maximum, (heading, count)
        counts[code] = {"words": count, "maximum": maximum, "note": "Whitespace count including table text; final institutional count may differ."}
    (EVID / "report_word_counts.json").write_text(json.dumps(counts, indent=2))

def portfolio():
    story, md = [], ["# NERP C/D Security Assurance Portfolio\n"]

    def page(title, subtitle):
        if story:
            story.append(PageBreak())
        story.append(Paragraph(text(title), styles["TitleDV"]))
        story.append(p(subtitle, small=True))
        md.extend(["\n## " + title, subtitle + "\n"])

    def para(s):
        story.append(p(s))
        md.append(s + "\n")

    def tab(headers, rows, widths):
        story.append(table(headers, rows, widths))
        story.append(Spacer(1, 10))
        md.extend(["| " + " | ".join(headers) + " |", "| " + " | ".join("---" for _ in headers) + " |"])
        md.extend("| " + " | ".join(map(str, row)) + " |" for row in rows)
        md.append("")

    page("Security assurance evidence", "Parts C and D only | Prepared 2 November 2026 | Reproducible C11 prototype")
    para("This portfolio records executed results for the supplied five-dataset fixture. It supports the C/D report; it does not establish that the group's architecture, authentication or consensus components have been implemented or tested.")
    tab(["Evidence", "Claim supported", "Primary artefact"], [
        ["C01", "All source records receive justified labels", "classifications.json; classification_counts.json"],
        ["C02", "Ambiguous/adversarial input fails safely", "integration_tests.json"],
        ["C03", "Purpose-specific views minimise disclosure", "operational_view.json; analytics_view.json; research_view.json"],
        ["C04", "Managed-copy retention and holds are enforced", "retention_*_checkpoint.json; retention_tests.json"],
        ["D01", "Explicit role permissions exist", "access_matrix.json; config.json"],
        ["D02", "Assignment conflicts do not silently grant access", "data_quality_warnings.json"],
        ["D03", "ACM, BLP and Biba jointly decide", "policy_scenarios.json; operational_decisions.json"],
        ["D04", "Executable checks cover expected and adverse inputs", "integration_tests.json; sanitizer_tests.json; source_sha256.json"],
    ], [48, 205, 246])
    para("Observed: 100 labelled records; 16 expected policy outcomes; three authenticated-encryption checks; four lifecycle checks; 33 black-box checks. The black-box suite validates some of the preceding experiments: these counts are not additive independent tests.")
    para("Evidence is machine-readable JSON and actual terminal output. No screenshot, real deployment result or external group contribution has been invented. Generated secrets are deliberately excluded from the handover; rerunning creates new keys.")

    page("C01 / C02  Classification", "Schema, observed distribution and adversarial handling")
    conf = read("classification_counts.json")
    tab(["Level", "Records", "Interpretation"], [
        ["PUBLIC", conf["PUBLIC"], "Application release label; no raw fixture is public"],
        ["OFFICIAL", conf["OFFICIAL"], "Personal / ordinary operational information; restricted handling"],
        ["CONFIDENTIAL", conf["CONFIDENTIAL"], "Supplied-data compatibility level; not an extra current GSCP tier"],
        ["SECRET", conf["SECRET"], "Terrorism response and serious intelligence"],
        ["TOP SECRET", conf["TOP SECRET"], "Critical national-security intelligence and linked audit events"],
    ], [96, 55, 348])
    labels = read("classifications.json")
    selected = ["INC-002", "INC-010", "R004", "INT-001", "INT-003", "INT-014", "D010", "AUD-010"]
    tab(["Record", "Confidentiality", "Integrity", "Why this example matters"], [
        [r["id"], r["classification"], r["integrity"], {"INC-002":"PII is protected without inventing medical diagnoses", "INC-010":"Terrorism incident content raises sensitivity", "R004":"Personnel clearance is not object classification", "INT-001":"Hint and threat indicators agree", "INT-003":"Explicit unverified fixture, not supplied provenance", "INT-014":"Explicit simulated endorsement, not keyword-derived trust", "D010":"Inherits incident classification", "AUD-010":"Inherits TOP SECRET referenced intelligence"}[r["id"]]]
        for r in labels if r["id"] in selected
    ], [61, 89, 94, 255])
    para("Unknown intelligence hints and content-hint conflicts quarantine records. Missing reference targets also quarantine. Duplicate keys, malformed JSON and unsafe identifiers are rejected. No claim of classifier accuracy is made without ground-truth labels.")

    page("C03  Purpose-specific views", "Actual field transformations and release boundaries")
    tab(["Source", "Operational projection", "Analytics / research handling"], [
        ["Incidents", "Assigned event: exact location, priority, casualties; caller contacts omitted", "18 eligible records; month, agency, priority, casualty band, HMAC token. No precise location or incident free text"],
        ["Responders", "Own assignment/role/station for frontline subject", "Role counts only; no names, stations, individual IDs or clearances"],
        ["Intelligence", "Only policy-permitted threat/region/summary fields", "All raw intelligence excluded from derived views"],
        ["Dispatch", "Associated dispatch, unit and response times", "18 eligible rows, HMAC join token, status and valid response duration; no unit"],
        ["Audit", "Only permitted oversight records", "Counts for eligible action types, no actor/resource/time trace"],
    ], [73, 198, 228])
    op = read("operational_view.json")
    para("R002's operational projection contains incidents INC-002, dispatch D002, own responder R002 and three permitted OFFICIAL intelligence records. No audit records pass. Each attempted read has a decision trace in operational_decisions.json.")
    research = read("research_view.json")
    tab(["Released research group", "Eligible incidents", "Casualties rounded to 5"], [[g["agency"], g["incidentCount"], g["casualtiesRoundedTo5"]] for g in research["groups"]], [220, 130, 149])
    para("Cells smaller than k=3 are suppressed. A changed-configuration test with k=8 suppresses all three cells. Research retains OFFICIAL / UNVERIFIED and RESTRICTED_REVIEW_REQUIRED. It contains no record IDs, free text, precise times or places.")
    para("HMAC pseudonyms preserve permitted joins but remain pseudonymous. Aggregate output remains vulnerable to external linkage and repeated-query differencing. Trusted transformation and human release approval are explicit trust dependencies, not an unrecorded BLP exception.")

    page("C04  Lifecycle enforcement", "Encrypted managed copies, simulated clocks, legal hold and disposal scope")
    cfg = json.loads((ROOT / "config.json").read_text())
    tab(["Classification", "Active days", "Total days", "Policy status"], [[k, v["active_days"], v["total_days"], "Coursework choice; not a legal duration"] for k, v in cfg["retention"].items()], [112, 72, 72, 243])
    para("Audit total retention has a 2555-day floor. Missing dates cannot silently expire; without the configured ingestion-date assumption, 40 undated records enter UNKNOWN_DATE_HOLD. Future dates and backwards lifecycle clocks are rejected or held as appropriate. INC-006 has a configured legal hold.")
    archive = collections.Counter(r["nextState"] for r in read("retention_archive_checkpoint.json"))
    expiry = collections.Counter(r["nextState"] for r in read("retention_expiry_checkpoint.json"))
    tab(["Experimental clock", "Observed lifecycle counts"], [["2027-01-03", "; ".join(f"{k}: {v}" for k,v in sorted(archive.items()))], ["2028-11-05", "; ".join(f"{k}: {v}" for k,v in sorted(expiry.items()))]], [122, 377])
    rt = read("retention_tests.json")
    tab(["Executed lifecycle check", "Result"], [[k, "PASS" if v else "FAIL"] for k,v in rt.items() if k!="scope"], [425, 74])
    para("Each managed record has a random AES-256-GCM key; its labels/date/identity are bound as authenticated data. The signed inventory protects lifecycle metadata. Archived is a logical state over encrypted storage. At expiry the live key file is overwritten, fsynced and unlinked; ciphertext can remain but the tested record no longer decrypts.")
    para("Boundary: source fixtures and previous exports are untouched. SSD wear levelling, snapshots, replicas, key backups, crash recovery and production KMS sanitisation are not solved by this demonstration. NIST SP 800-88 Rev. 2 informs the limitation; no certified media purge is claimed.")

    page("D01  Access Control Matrix", "Coarse role grants, followed by scope, confidentiality, integrity and quarantine gates")
    roles = cfg["roles"]
    tab(["Role", "Inc.", "Resp.", "Intel.", "Disp.", "Audit", "Scope"], [[role]+[v["grants"][k] or "-" for k in ["incidents","responders","intelligence","dispatch","audit"]]+["Assigned + agency; own personnel" if v["scope"]=="assigned" else ("Global (provisional)" if role=="Regional Commander" else "Global")] for role,v in roles.items()], [108, 35, 42, 37, 42, 45, 190])
    para("r = read; w = blind-write authorisation. A grant is necessary, never sufficient. No human role has audit write permission. A protected audit append service belongs to the platform architecture. Dispatcher DISPATCHER01 is an explicitly synthetic profile because the supplied personnel dataset has no dispatcher.")
    para("Frontline incident/dispatch access requires both matching currentAssignment and matching agency. Intelligence remains subject to both mandatory models. The fixtures do not provide compartments or an authoritative region map; those restrictions need group integration.")
    tab(["Decision rule", "Check"], [["Valid context", "Known subject, resource and action; session <= clearance"], ["Role permission", "Action exists in the Access Control Matrix"], ["Scope", "Assignment and agency, or explicitly configured scope"], ["BLP", "read: S >= O; blind write: S <= O (confidentiality)"], ["Biba", "read: S <= O; blind write: S >= O (integrity)"], ["Review status", "Quarantined objects are denied"], ["Final", "Allow only when every gate passes"]], [122, 377])

    page("D02  Data quality and scope", "Source anomalies are reported rather than silently rewritten")
    warnings = read("data_quality_warnings.json")
    tab(["Responder", "Assigned incident", "Finding / consequence"], [[w["responder"],w["assignment"],w["finding"]] for w in warnings], [85, 105, 309])
    para("Seven mismatches are present in the original source fixture. Example: R007 is a paramedic assigned to INC-007, but the incident's assignedAgency is Fire. Assignment alone would grant access across the configured agency boundary. The selected policy denies until command review authorises or corrects the relationship.")
    para("This is a conservative prototype choice, not proof that a real multi-agency assignment is invalid. Major incidents may legitimately require joint access. The architecture owner must decide how explicit joint-response authorisations are represented; a silent global allow would discard least privilege.")
    para("Further missing provenance: personnel and intelligence lack record timestamps; dispatch has times without dates and inherits the incident date. The assumed ingestion date is recorded per managed copy. Object integrity is configured separately; the data do not establish that dispatch or audit ingestion is trustworthy.")

    page("D03  Required policy conflicts", "Three task-required scenarios plus both mandatory write restrictions")
    cases = read("policy_scenarios.json")
    tab(["Case", "Request", "ACM", "BLP", "Biba", "Final"], [[i+1, f'{c["subject"]} {c["operation"]} {c["resource"]}', "Allow" if c["ACM"] else "Deny", "Allow" if c["BLP"] else "Deny", "Allow" if c["Biba"] else "Deny", "Allow" if c["allowed"] else "Deny"] for i,c in enumerate(cases[:5])], [35, 204, 63, 63, 65, 69])
    for i in range(5):
        c=cases[i]
        para(f'{i+1}. {c["scenario"]}: {c["reason"]}. Expected outcome matched: {c["passed"]}.')
    para("Cases 1-3 satisfy D4 directly. Case 4 demonstrates No Write Down. Case 5 demonstrates No Write Up. Scope is satisfied in these examples so the mandatory-policy conflict can be assessed directly. Raw decision artefacts retain every gate independently.")
    para("INT-003's low integrity and INT-014's endorsement are controlled evaluation assumptions. No code assigns trust merely because a report calls itself high confidence. The source data are unchanged.")

    page("D03  Additional adverse cases", "Remaining policy experiments: all outcomes matched the configured policy")
    tab(["Case", "Scenario", "Final", "Observed reason"], [[i+1,c["scenario"],"Allow" if c["allowed"] else "Deny",c["reason"]] for i,c in enumerate(cases) if i>=5], [34, 217, 51, 197])
    para("Reads only return record data after all gates pass. Unknown identities/actions/resources fail closed. A fresh OFFICIAL session is modelled separately from R001's default SECRET context. The prototype does not prove cross-session noninterference; production must enforce isolation and trusted current labels.")
    para("The CLI's write command returns a blind-write decision only. It does not persist operational edits. A real update route must check read and write as necessary, validate fields, preserve protected labels and apply the change transactionally.")

    page("D04  Verification record", "Executed black-box suite and build environment")
    env = read("verification_environment.json")
    tab(["Verification", "Observed result"], [["Compiler",env["compiler"]],["Crypto library",env["openssl"]],["Strict build",env["strict_build"]],["Normal black-box suite","33 checks PASS"],["Instrumented suite","33 checks PASS under AddressSanitizer + UndefinedBehaviorSanitizer"],["LeakSanitizer",env["leak_sanitizer"]]], [115,384])
    para("Test coverage includes classification and reference propagation; privacy allowlists and unexpected fields; HMAC joins; research suppression; current-session limits; matrix and mandatory-policy conflicts; scope anomalies; metadata/ciphertext integrity; legal holds and missing dates; calendar validation and malformed JSON. The raw JSON results enumerate each check.")
    para("Actual demo terminal output:")
    for line in (EVID / "demo_log.txt").read_text().splitlines():
        if line.startswith("Results:"):
            line="Results: fresh output directory (local path omitted)"
        story.append(p(line,small=True));md.append("    "+line)
    para("Reproduce from the project root: make; make test; ./nerp demo data config.json run. Choose a fresh run directory. make sanitize repeats the suite with instrumentation. Denied CLI requests intentionally exit with status 3.")
    para("Integrity of the evidence package: source_sha256.json records full SHA-256 values for implementation, headers, tests, configuration and original datasets. report_word_counts.json records each report section's conservative whitespace word count. No generated encryption/HMAC key is included in the handover.")

    page("Assurance and integration limits", "A bounded claim supported by evidence, not a claim of national deployment readiness")
    tab(["Claim", "Evidence", "Remaining condition"], [["Classification boundaries are applied","C01-C02","Trusted provenance, protected rules and steward review"],["Views reduce disclosure","C03","Release approval, linkage review and query-budget controls"],["Managed-copy expiry and holds work","C04","Production KMS, replicas, exports, backups and failure recovery"],["Policy gates combine correctly for tested inputs","D01-D04","Authenticated identities, protected labels and complete service mediation"],["Assignment inconsistencies do not silently grant access","D02-D03","Command-approved joint-response and regional rules"],["Implementation survives tested adverse input","D04","Broader testing; no concurrency/distributed or formal correctness claim"]], [147,68,284])
    para("The C/D modules are suitable as an integration prototype under the stated assumptions. Strict Biba can block useful field-to-dispatch workflows; a trusted endorsement path must be separately engineered. A trusted sanitisation path must not become an ordinary user bypass. The group's audit consensus, authentication, architecture and E-part governance decisions remain outside these tests.")
    para("Group handover: agree the classification lattice, integrity assumptions, regional/incident scope, dates, retention days, service identities, protected write handlers and evidence numbering. Then rerun and revise the report together. The current report is deliberately labelled as an integration-review draft.")
    para("Source basis: Cabinet Office GSCP (2024); Bell and LaPadula (1976); Biba (1977, bibliographic record); ICO storage-limitation and anonymisation guidance; NIST SP 800-88 Rev. 2 (2025); OpenSSL EVP documentation. Full links appear in the accompanying report. No retention duration is attributed to legislation.")
    (DOCS / "Security_Assurance_Portfolio_CD.md").write_text("\n".join(md))
    build(DOCS / "Security_Assurance_Portfolio_CD.pdf", story)

if __name__ == "__main__":
    report()
    portfolio()
    print("Built report and C/D assurance portfolio.")
