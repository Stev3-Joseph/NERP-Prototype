const Acceptor = require("../../consensus/paxos/acceptor");

console.log("\n==============================");
console.log("NERP RESOURCE ALLOCATION");
console.log("ATTACK: ACCEPTOR STATE LOSS");
console.log("==============================");

// Region 1 acceptor
const a1 = new Acceptor("Region1-A1");

console.log("\nRegion 1 Acceptor A1 accepts a resource allocation proposal.");

a1.acceptedProposal = {
    number: 101,
    value: "Resource allocation: INC-001"
};

a1.highestPromised = 101;

console.log("\nBefore state loss:");
console.log(a1);

/* Simulate loss of persistent state */
console.log("\nAcceptor A1 experiences disk failure.");
console.log("Previously stored Paxos state is lost.");

a1.acceptedProposal = null;
a1.highestPromised = 0;

console.log("\nAfter state loss:");
console.log(a1);

console.log("\nWARNING:");
console.log("Acceptor has forgotten its previous proposal.");
console.log("Paxos safety may be at risk if it resumes without recovery.");