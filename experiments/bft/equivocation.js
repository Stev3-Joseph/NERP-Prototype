const Node = require("../../consensus/bft/bftnode");
const { makeEquivocator } = require("./attacks");

// NERP validators across three regions
const validators = {
    A: new Node("Region1-Validator-A"),
    B: new Node("Region1-Validator-B"),
    C: new Node("Region2-Validator-C"),
    D: new Node("Region3-Validator-D")
};

const auditEvent = {
    eventId: "AUD-021",
    action: "INTELLIGENCE_ACCESS",
    responderId: "R-104",
    incidentId: "INC-001"
};

console.log("Equivocation attack - National Audit Log");
console.log("Region 3 Validator D is compromised.");
console.log("D sends conflicting audit outcomes to the other validators.");

makeEquivocator(validators.D, {
    A: { ...auditEvent, result: "SUCCESS" },
    B: { ...auditEvent, result: "FAILURE" },
    C: { ...auditEvent, result: "UNAUTHORISED" }
});

console.log("\nMessage received by Region 1 Validator A:");
console.log(validators.D.voteForPeer("A"));

console.log("\nMessage received by Region 1 Validator B:");
console.log(validators.D.voteForPeer("B"));

console.log("\nMessage received by Region 2 Validator C:");
console.log(validators.D.voteForPeer("C"));