const Node = require("../../consensus/bft/bftnode");
const { stealKey, forgeVote } = require("./attacks");

const validators = {
    A: new Node("Region1-Validator-A"),
    B: new Node("Region1-Validator-B"),
    C: new Node("Region2-Validator-C"),
    D: new Node("Region3-Validator-D")
};

const maliciousAuditEvent = {
    eventId: "AUD-022",
    action: "INCIDENT_RECORD_MODIFIED",
    responderId: "R-104",
    incidentId: "INC-001",
    result: "AUTHORISED"
};

console.log("Key compromise attack - National Audit Log");
console.log("Attacker targets Region 3 Validator D.");
console.log("D's validator key is stolen and used to forge an audit message.");

const stolenSecret = stealKey(validators.D);

const forgedVote = forgeVote(
    validators.D.id,
    stolenSecret,
    maliciousAuditEvent
);

console.log("\nForged audit message:");
console.log(forgedVote);