const fs = require("fs");
const path = require("path");

class CPGM {
    constructor() {
        this.quarantined = new Set();
        this.auditFile = path.join(__dirname, "..", "..", "datastore", "audit.json");
    }

    checkAcceptor(acceptor, trustedState) {
        const stateMatches =
            acceptor.highestPromised === trustedState.highestPromised &&
            JSON.stringify(acceptor.acceptedProposal) ===
            JSON.stringify(trustedState.acceptedProposal);

        if (!stateMatches) {
            console.log(`[CPGM ALERT] State mismatch: ${acceptor.id}`);

            this.quarantined.add(acceptor.id);

            this.writeAudit(acceptor.id);

            console.log(`[CPGM ACTION] ${acceptor.id} quarantined.`);
            return false;
        }

        console.log(`[CPGM] ${acceptor.id} state verified.`);
        return true;
    }

    canParticipate(acceptor) {
        return !this.quarantined.has(acceptor.id);
    }

    writeAudit(acceptorId) {
        let records = [];

        if (fs.existsSync(this.auditFile)) {
            records = JSON.parse(fs.readFileSync(this.auditFile, "utf8"));
        }

        const nextNumber = records.reduce((max, record) => {
            const number = Number(
                (record.eventId || "").replace("AUD-", "")
            );
            return Math.max(max, number || 0);
        }, 0) + 1;

        records.push({
            eventId: `AUD-${String(nextNumber).padStart(3, "0")}`,
            actor: "CPGM Security Monitor",
            action: "PAXOS ACCEPTOR QUARANTINED",
            resource: acceptorId,
            timestamp: new Date().toISOString()
        });

        fs.writeFileSync(
            this.auditFile,
            JSON.stringify(records, null, 4)
        );

        console.log("[CPGM AUDIT] Violation recorded.");
    }
}

module.exports = CPGM;